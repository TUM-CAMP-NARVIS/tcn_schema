"""A recursive-descent parser for the IDL subset used in tcn_schema.

Supported: `module`, `map<K, V>`,, `struct` (and forward declarations), `enum`, `union ... switch(T)`, `typedef`, `const`,
annotations `@name` / `@name(...)` (including multi-line `@verbatim` strings), `sequence<T[, N]>`, `string[<N>]`,
multi-word primitives (`unsigned long long`), fixed arrays `x[N]`, comments, and the preprocessor lines `#include`
(recorded), `#ifndef` / `#define` / `#endif` (ignored).

Names are resolved to fully qualified form by searching the declaring module scope outward, first within the file,
then (in `load_schema`) across all files.
"""
import re
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple

from .model import Const, Enum, IdlFile, Member, Schema, Struct, Typedef, Union

FAMILY_DIRS = {"ros2": "ros2", "custom": "tcn", "tcn": "tcn"}

_PRIMITIVE_WORDS = {"short", "long", "unsigned", "float", "double", "char", "wchar", "boolean", "octet",
                    "int8", "uint8", "int16", "uint16", "int32", "uint32", "int64", "uint64", "string", "wstring",
                    "void", "any"}

_TOKEN = re.compile(r"""
    (?P<ws>\s+)
  | (?P<lcomment>//[^\n]*)
  | (?P<bcomment>/\*.*?\*/)
  | (?P<pp>^[ \t]*\#[^\n]*)
  | (?P<string>"(?:[^"\\]|\\.)*")
  | (?P<number>-?(?:0[xX][0-9a-fA-F]+|\d+(?:\.\d*)?(?:[eE][-+]?\d+)?)[a-zA-Z]*)
  | (?P<name>::?[A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_][A-Za-z0-9_]*)*|[A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_][A-Za-z0-9_]*)*)
  | (?P<punct>[{}()\[\];:<>,=@|&+\-*/~%^])
""", re.VERBOSE | re.DOTALL | re.MULTILINE)


class ParseError(Exception):
    pass


def _tokenize(text: str):
    """Yield (kind, value, start, end); comments and whitespace dropped; #include recorded as ('include', path)."""
    pos = 0
    while pos < len(text):
        m = _TOKEN.match(text, pos)
        if not m:
            line = text.count("\n", 0, pos) + 1
            raise ParseError(f"line {line}: unexpected character {text[pos]!r}")
        kind = m.lastgroup
        value = m.group(kind)
        pos = m.end()
        if kind in ("ws", "lcomment", "bcomment"):
            continue
        if kind == "pp":
            inc = re.match(r'\s*#\s*include\s*[<"]([^>"]+)[>"]', value)
            if inc:
                yield ("include", inc.group(1), m.start(), m.end())
            continue
        yield (kind, value, m.start(), m.end())


class _Parser:
    def __init__(self, text: str):
        self.text = text
        self.includes: List[str] = []
        self.toks = []
        for t in _tokenize(text):
            if t[0] == "include":
                self.includes.append(t[1])
            else:
                self.toks.append(t)
        self.i = 0
        # raw declarations: (kind, fqname, scope, payload)
        self.decls: List[tuple] = []
        # forward declarations (scope + [name]); fastddsgen 3.0.0 stores members of these types as pointers
        self.forwards: List[List[str]] = []
        self.forward_kinds: Dict[str, str] = {}

    # -- token helpers -----------------------------------------------------------------------------------------
    def peek(self, k: int = 0):
        j = self.i + k
        return self.toks[j] if j < len(self.toks) else ("eof", "", len(self.text), len(self.text))

    def next(self):
        t = self.peek()
        self.i += 1
        return t

    def expect(self, value: str):
        t = self.next()
        if t[1] != value:
            line = self.text.count("\n", 0, t[2]) + 1
            raise ParseError(f"line {line}: expected {value!r}, got {t[1]!r}")
        return t

    def accept(self, value: str) -> bool:
        if self.peek()[1] == value:
            self.i += 1
            return True
        return False

    # -- grammar -----------------------------------------------------------------------------------------------
    def parse(self):
        self.definitions([])
        if self.peek()[0] != "eof":
            raise ParseError(f"trailing token {self.peek()[1]!r}")

    def definitions(self, scope: List[str]):
        while self.peek()[0] != "eof" and self.peek()[1] != "}":
            self.definition(scope)

    def annotations(self) -> Dict[str, str]:
        ann: Dict[str, str] = {}
        while self.peek()[1] == "@":
            self.next()
            name = self.next()[1]
            args = ""
            if self.peek()[1] == "(":
                start = self.next()[3]
                depth = 1
                end = start
                while depth:
                    t = self.next()
                    if t[0] == "eof":
                        raise ParseError("unterminated annotation")
                    if t[1] == "(":
                        depth += 1
                    elif t[1] == ")":
                        depth -= 1
                    end = t[2]
                args = self.text[start:end].strip()
            ann[name] = args
        return ann

    def _decl_start(self) -> int:
        """Start of the next declaration's text: its leading comments and annotations belong to it, a comment on the
        line of the previous declaration's end does not."""
        prev_end = self.toks[self.i - 1][3] if self.i > 0 else 0
        nl = self.text.find("\n", prev_end)
        rest = self.text[prev_end:nl if nl >= 0 else len(self.text)]
        if nl >= 0 and (not rest.strip() or rest.strip().startswith("//")):
            pos = nl + 1
        else:
            pos = prev_end
        while pos < len(self.text) and self.text[pos] in " \t\r\n":
            pos += 1
        return pos

    def _decl_end(self) -> int:
        """End of the declaration just parsed: after its `;`, plus a comment on the same line."""
        end = self.toks[self.i - 1][3]
        nl = self.text.find("\n", end)
        rest = self.text[end:nl if nl >= 0 else len(self.text)]
        if rest.strip().startswith("//") or rest.strip().startswith("/*"):
            return end + len(rest.rstrip())
        return end

    def definition(self, scope: List[str]):
        start = self._decl_start()
        before = len(self.decls)
        self._definition(scope)
        # A declaration (not a module, whose inner declarations carry their own spans, nor a forward declaration)
        # was added: record its span.
        if len(self.decls) == before + 1 and len(self.decls[-1]) == 4:
            kind, sc, name, payload = self.decls[-1]
            self.decls[-1] = (kind, sc, name, payload, (start, self._decl_end()))

    def _definition(self, scope: List[str]):
        ann = self.annotations()
        kw = self.peek()[1]
        if kw == "module":
            self.next()
            name = self.next()[1]
            self.expect("{")
            self.definitions(scope + [name])
            self.expect("}")
            self.accept(";")
        elif kw == "struct":
            self.struct(scope, ann)
        elif kw == "enum":
            self.enum(scope, ann)
        elif kw == "union":
            self.union(scope, ann)
        elif kw == "typedef":
            self.next()
            ty = self.type_spec()
            name = self.next()[1]
            dims = self.array_dims()
            self.expect(";")
            self.decls.append(("typedef", scope, name, (ty, dims)))
        elif kw == "const":
            self.next()
            ty = self.type_spec()
            name = self.next()[1]
            self.expect("=")
            start = self.peek()[2]
            end = start
            while self.peek()[1] != ";":
                end = self.next()[3]
            self.expect(";")
            self.decls.append(("const", scope, name, (ty, self.text[start:end].strip())))
        else:
            t = self.peek()
            line = self.text.count("\n", 0, t[2]) + 1
            raise ParseError(f"line {line}: unexpected {t[1]!r}")

    @staticmethod
    def _extensibility(ann: Dict[str, str]) -> str:
        for k in ("final", "appendable", "mutable"):
            if k in ann:
                return k
        if "extensibility" in ann:
            return ann["extensibility"].strip().lower()
        return "default"

    def struct(self, scope, ann):
        self.expect("struct")
        name = self.next()[1]
        if self.accept(";"):
            self.forwards.append(scope + [name])
            self.forward_kinds["::".join(scope + [name])] = "struct"
            return  # forward declaration
        if self.accept(":"):
            self.next()  # base type (unused in this repository)
        self.expect("{")
        members: List[tuple] = []
        while not self.accept("}"):
            members.extend(self.member())
        self.accept(";")
        self.decls.append(("struct", scope, name, (members, self._extensibility(ann), ann)))

    def member(self) -> List[tuple]:
        ann = self.annotations()
        ty = self.type_spec()
        out = []
        while True:
            name = self.next()[1]
            dims = self.array_dims()
            out.append((name, ty, dims, ann))
            if not self.accept(","):
                break
        self.expect(";")
        return out

    def array_dims(self) -> Optional[List[int]]:
        dims = []
        while self.accept("["):
            dims.append(int(self.next()[1], 0))
            self.expect("]")
        return dims or None

    def enum(self, scope, ann):
        self.expect("enum")
        name = self.next()[1]
        self.expect("{")
        values: List[Tuple[str, int]] = []
        nxt = 0
        while not self.accept("}"):
            eann = self.annotations()
            en = self.next()[1]
            if "value" in eann:
                nxt = int(eann["value"], 0)
            values.append((en, nxt))
            nxt += 1
            self.accept(",")
        self.accept(";")
        self.decls.append(("enum", scope, name, (values, self._extensibility(ann), ann)))

    def union(self, scope, ann):
        self.expect("union")
        name = self.next()[1]
        if self.accept(";"):
            self.forwards.append(scope + [name])
            self.forward_kinds["::".join(scope + [name])] = "union"
            return  # forward declaration
        self.expect("switch")
        self.expect("(")
        disc = self.type_spec()
        self.expect(")")
        self.expect("{")
        cases = []
        while not self.accept("}"):
            labels = []
            while self.peek()[1] in ("case", "default"):
                if self.next()[1] == "default":
                    labels.append("default")
                else:
                    start = self.peek()[2]
                    end = start
                    while self.peek()[1] != ":":
                        end = self.next()[3]
                    labels.append(self.text[start:end].strip())
                self.expect(":")
            (m,) = self.member()
            cases.append((labels, m))
        self.accept(";")
        self.decls.append(("union", scope, name, (disc, cases, self._extensibility(ann), ann)))

    def type_spec(self):
        """Return a type expression: ('prim', text) | ('name', raw) | ('seq', elem, bound) | ('str', kind, bound)."""
        t = self.next()
        word = t[1]
        if word == "sequence":
            self.expect("<")
            elem = self.type_spec()
            bound = None
            if self.accept(","):
                bound = self.next()[1]
            self.expect(">")
            return ("seq", elem, bound)
        if word == "map":
            self.expect("<")
            key = self.type_spec()
            self.expect(",")
            val = self.type_spec()
            bound = None
            if self.accept(","):
                bound = self.next()[1]
            self.expect(">")
            return ("map", key, val, bound)
        if word in ("string", "wstring"):
            bound = None
            if self.accept("<"):
                bound = self.next()[1]
                self.expect(">")
            return ("str", word, bound)
        if word in _PRIMITIVE_WORDS:
            words = [word]
            while self.peek()[1] in ("long", "short") and words[-1] in ("unsigned", "long"):
                words.append(self.next()[1])
            if words[-1] == "long" and self.peek()[1] == "double":
                words.append(self.next()[1])
            return ("prim", " ".join(words))
        if t[0] != "name":
            raise ParseError(f"expected a type, got {word!r}")
        return ("name", word)


def _render(ty, resolve) -> str:
    kind = ty[0]
    if kind == "prim":
        return ty[1]
    if kind == "name":
        return resolve(ty[1])
    if kind == "str":
        return f"{ty[1]}<{ty[2]}>" if ty[2] else ty[1]
    if kind == "map":
        k, v = _render(ty[1], resolve), _render(ty[2], resolve)
        return f"map<{k}, {v}, {ty[3]}>" if ty[3] else f"map<{k}, {v}>"
    if kind == "seq":
        inner = _render(ty[1], resolve)
        return f"sequence<{inner}, {ty[2]}>" if ty[2] else f"sequence<{inner}>"
    raise ValueError(ty)


def _make_resolver(scope: List[str], known: Set[str]):
    def resolve(raw: str) -> str:
        if raw.startswith("::"):
            return raw[2:]
        for i in range(len(scope), -1, -1):
            cand = "::".join(scope[:i] + [raw])
            if cand in known:
                return cand
        return raw
    return resolve


def _build(raw_decls, known: Set[str]):
    out = []
    for kind, scope, name, payload, span in raw_decls:
        fq = "::".join(scope + [name])
        res = _make_resolver(scope, known)
        if kind == "struct":
            members, ext, ann = payload
            out.append(Struct(fq, [_member(m, res) for m in members], ext, ann, span))
        elif kind == "enum":
            values, ext, ann = payload
            out.append(Enum(fq, values, ext, ann, span))
        elif kind == "union":
            disc, cases, ext, ann = payload
            out.append(Union(fq, _render(disc, res), [(labels, _member(m, res)) for labels, m in cases], ext, ann,
                             span))
        elif kind == "typedef":
            ty, dims = payload
            out.append(Typedef(fq, _render(ty, res), dims, span))
        elif kind == "const":
            ty, value = payload
            out.append(Const(fq, _render(ty, res), value, span))
    return out


def _member(m, res) -> Member:
    name, ty, dims, ann = m
    if name.startswith("_"):
        name = name[1:]  # IDL 4.2 7.2.3: an escaped identifier is the name without the leading underscore
    ann = dict(ann)
    optional = "optional" in ann
    return Member(name, _render(ty, res), optional, dims, ann)


def _parse_raw(path: Path) -> _Parser:
    p = _Parser(path.read_text(encoding="utf-8"))
    try:
        p.parse()
    except ParseError as e:
        raise ParseError(f"{path}: {e}") from None
    return p


def _file_meta(rel: str):
    parts = rel.split("/")
    return parts[0], (parts[1] if len(parts) > 2 else "msg")


def parse_file(path: Path, rel: str, family: str = "tcn") -> IdlFile:
    """Parse one file; names resolve against this file's own declarations only."""
    p = _parse_raw(path)
    known = {"::".join(d[1] + [d[2]]) for d in p.decls}
    pkg, kind = _file_meta(rel)
    return IdlFile(rel, pkg, kind, p.includes, _build(p.decls, known), family, ["::".join(x) for x in p.forwards],
                   dict(p.forward_kinds))


def load_schema(dds_root: Path) -> Schema:
    """Parse every *.idl under dds_root/{ros2,custom,tcn}; resolve names across all files."""
    raws = []
    family: Dict[str, str] = {}
    for sub, fam in FAMILY_DIRS.items():
        base = dds_root / sub
        if not base.is_dir():
            continue
        for path in sorted(base.rglob("*.idl")):
            rel = path.relative_to(base).as_posix()
            raws.append((rel, fam, _parse_raw(path)))
            family[_file_meta(rel)[0]] = fam
    known = {"::".join(d[1] + [d[2]]) for _, _, p in raws for d in p.decls}
    files = []
    for rel, fam, p in raws:
        pkg, kind = _file_meta(rel)
        files.append(IdlFile(rel, pkg, kind, p.includes, _build(p.decls, known), fam, ["::".join(x) for x in p.forwards],
                             dict(p.forward_kinds)))
    return Schema(files, family)
