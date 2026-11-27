"""Apply a declarative move of IDL declarations between files, packages and modules (0.11 plan 3).

Each declaration is moved as its original text (leading comments and annotations included, see parser spans). In
the moved text, references to moved types are rewritten to their new fully qualified names, and unqualified
references to a type that now lives in another module are qualified. Every new file gets an include guard, the
`#include`s of the files it references, and the original file's top comment. The result is the new source of truth;
this script and its move list (tools/idl/moves_0_11.json) stay in the repository as the record of the move.

    python3 -I tools/idl_cli.py restructure --dds-root src/dds --spec tools/idl/moves_0_11.json
"""
import re
import shutil
from pathlib import Path, PurePosixPath
from typing import Dict, List, Set, Tuple

from .model import Enum, Struct, Union
from .parser import FAMILY_DIRS, load_schema

_CODE_OR_SKIP = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:[^"\\]|\\.)*"', re.S)
_QUALIFIED = re.compile(r"(?<![\w:])(?:::)?([A-Za-z_]\w*(?:::[A-Za-z_]\w*)+)")
_NAMES = re.compile(r"[A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_][A-Za-z0-9_]*)+")
_PP = re.compile(r"^\s*#\s*(ifndef|define|endif|include)\b.*$", re.M)


class SpecError(Exception):
    pass


def _code_sub(text: str, fn) -> str:
    """Apply fn to the code parts of text (not to comments or string literals)."""
    out, pos = [], 0
    for m in _CODE_OR_SKIP.finditer(text):
        out.append(fn(text[pos:m.start()]))
        out.append(m.group(0))
        pos = m.end()
    out.append(fn(text[pos:]))
    return "".join(out)


def _refs(decl) -> Set[str]:
    """Fully qualified names a declaration's members (or union discriminator) refer to."""
    types = []
    if isinstance(decl, Struct):
        types = [m.type for m in decl.members]
    elif isinstance(decl, Union):
        types = [decl.discriminator] + [m.type for _, m in decl.cases]
    return {n for t in types for n in _NAMES.findall(t)}


def _short(fq: str) -> str:
    return fq.rsplit("::", 1)[-1]


def _module(fq: str) -> str:
    return fq.rsplit("::", 1)[0]


def _preamble(text: str) -> str:
    head = text[:re.search(r"^\s*module\b", text, re.M).start()] if re.search(r"^\s*module\b", text, re.M) else ""
    return _PP.sub("", head).strip()


def guard(rel: str) -> str:
    p = PurePosixPath(rel)
    return "_".join(list(p.parts[:-1]) + [p.stem, "idl"]).upper()


def restructure(old_root: Path, spec: dict, new_root: Path) -> Dict[str, str]:
    source, target = spec["source"], spec["target"]
    schema = load_schema(old_root)
    family_dir = {fam: sub for sub, fam in FAMILY_DIRS.items() if (old_root / sub).is_dir()}
    texts, file_of, decl_of = {}, {}, {}
    for f in schema.files:
        sub = source if (old_root / source / f.path).is_file() and f.family == "tcn" else family_dir[f.family]
        texts[f.path] = (old_root / sub / f.path).read_text(encoding="utf-8")
        for d in f.decls:
            file_of[d.name] = f
            decl_of[d.name] = d
    source_decls = {d.name for f in schema.files if (old_root / source / f.path).is_file() for d in f.decls}

    placed: Dict[str, Tuple[str, str]] = {}  # old fq -> (new path, new module)
    for entry in spec["files"]:
        for name in entry["decls"]:
            if name not in source_decls:
                raise SpecError(f"{entry['path']}: {name} is not a declaration of {source}/")
            if name in placed:
                raise SpecError(f"{name} is placed twice ({placed[name][0]}, {entry['path']})")
            placed[name] = (entry["path"], entry["module"])
    deleted = set(spec.get("deleted", []))
    for name in deleted:
        if name not in source_decls:
            raise SpecError(f"deleted {name} is not a declaration of {source}/")
        if name in placed:
            raise SpecError(f"{name} is both placed and deleted")
    missing = sorted(source_decls - set(placed) - deleted)
    if missing:
        raise SpecError(f"neither placed nor deleted: {', '.join(missing)}")

    renames = {old: f"{mod}::{_short(old)}" for old, (_, mod) in placed.items()}
    path_of_new = {renames[old]: path for old, (path, _) in placed.items()}
    for name, d in decl_of.items():  # types that do not move (ROS) keep their file
        if name not in source_decls:
            path_of_new.setdefault(name, file_of[name].path)

    out_root = new_root / target
    if out_root.exists():
        shutil.rmtree(out_root)
    packages: Dict[str, Dict[str, List[str]]] = {}
    for entry in spec["files"]:
        path, module = entry["path"], entry["module"]
        names = entry["decls"]
        index = {n: i for i, n in enumerate(names)}
        includes: Set[str] = set()
        chunks = []
        for i, name in enumerate(names):
            d = decl_of[name]
            refs = _refs(d) - {name}
            for r in refs:
                if r in deleted:
                    raise SpecError(f"{name} refers to deleted {r}")
                if r in index and index[r] > i and r not in file_of[r].forward_kinds:
                    raise SpecError(f"{path}: {name} uses {r}, which comes later in the same file")
                new_r = renames.get(r, r)
                if path_of_new.get(new_r) and path_of_new[new_r] != path:
                    includes.add(path_of_new[new_r])
            qualify = {_short(r): renames[r] for r in refs if r in renames and _module(renames[r]) != module}
            text = texts[file_of[name].path]
            start, end = d.span
            line_start = text.rfind("\n", 0, start) + 1
            indent = text[line_start:start] if not text[line_start:start].strip() else ""
            body = indent + text[start:end]

            def fix(code, qualify=qualify):
                code = _QUALIFIED.sub(lambda m: renames.get(m.group(1), m.group(1))
                                      if m.group(1) in renames else m.group(0), code)
                for short, fq in qualify.items():
                    code = re.sub(rf"(?<![\w:]){re.escape(short)}(?![\w:])", fq, code)
                return code
            chunks.append(_code_sub(body, fix))
        sources = {file_of[n].path for n in names}
        pre_from = entry.get("preamble_from") or (sources.pop() if len(sources) == 1 else None)
        preamble = _preamble(texts[pre_from]) if pre_from else ""
        forwards = []
        for n in names:
            f = file_of[n]
            if n in f.forward_kinds:
                forwards.append(f"    {f.forward_kinds[n]} {_short(n)};")
        outer, inner = module.split("::")
        g = guard(path)
        parts = ([preamble, ""] if preamble else []) + [f"#ifndef {g}", f"#define {g}", ""]
        if includes:
            parts += [f'#include "{inc}"' for inc in sorted(includes)] + [""]
        parts += [f"module {outer} {{", f"  module {inner} {{", ""]
        if forwards:
            parts += forwards + [""]
        parts += ["\n\n".join(chunks), "", "  };", "};", "", "#endif", ""]
        dest = out_root / path
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_text("\n".join(parts), encoding="utf-8")
        pkg, kind = PurePosixPath(path).parts[0], PurePosixPath(path).parts[1]
        packages.setdefault(pkg, {}).setdefault(kind, []).append("/".join(PurePosixPath(path).parts[1:]))

    extras: Dict[str, List[str]] = {}
    for old_rel, new_rel in spec.get("extra_files", []):
        dest = out_root / new_rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(old_root / source / old_rel, dest)
        extras.setdefault(PurePosixPath(new_rel).parts[0], []).append("/".join(PurePosixPath(new_rel).parts[1:]))

    for pkg, kinds in packages.items():
        lines = ["tcn_add_idl_package(", f"        NAME {pkg}"]
        for kind in ("msg", "srv", "rpc", "action"):
            if kinds.get(kind):
                lines.append(f"        {kind.upper()}")
                lines += [f'            "{f}"' for f in kinds[kind]]
        if extras.get(pkg):
            lines.append("        EXTRA_INSTALL " + " ".join(f'"{f}"' for f in extras[pkg]))
        lines.append("        )")
        (out_root / pkg / "CMakeLists.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
    return renames
