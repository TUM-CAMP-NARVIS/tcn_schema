"""IDL lint: the style rules of docs/idl-style.md (decisions O5, E1, E2, E6, E7).

A baseline file lists accepted findings as `rule<TAB>file<TAB>symbol` (no line numbers, so unrelated edits do not
churn it). The lint fails on a finding not in the baseline and on a baseline entry that no longer occurs: the
baseline only shrinks.
"""
import re
from dataclasses import dataclass
from pathlib import PurePosixPath
from typing import Dict, Iterable, List, Set, Tuple

from .model import Enum, Schema, Struct, Union


@dataclass(frozen=True)
class Finding:
    rule: str
    file: str
    line: int
    symbol: str
    message: str

    def key(self) -> str:
        return f"{self.rule}\t{self.file}\t{self.symbol}"


def _upper_snake(name: str) -> str:
    s = re.sub(r"(?<=[a-z0-9])(?=[A-Z])", "_", name)
    s = re.sub(r"(?<=[A-Z])(?=[A-Z][a-z])", "_", s)
    return s.upper()


def _line_of(text: str, pattern: str) -> int:
    m = re.search(pattern, text)
    return text.count("\n", 0, m.start()) + 1 if m else 1


def expected_guard(rel: str) -> str:
    p = PurePosixPath(rel)
    parts = list(p.parts[:-1]) + [p.stem, "idl"]
    return "_".join(parts).upper()


def lint(schema: Schema, files_text: Dict[str, str]) -> List[Finding]:
    out: List[Finding] = []

    # basename-unique: one finding per duplicate basename within a package, on every file after the first.
    seen: Dict[Tuple[str, str], str] = {}
    for f in sorted(schema.files, key=lambda f: f.path):
        key = (f.package, PurePosixPath(f.path).name)
        if key in seen:
            out.append(Finding("basename-unique", f.path, 1, key[1],
                               f"{key[1]} also exists as {seen[key]}"))
        else:
            seen[key] = f.path

    for f in schema.files:
        text = files_text.get(f.path, "")
        stem = PurePosixPath(f.path).stem
        tcn = f.family == "tcn"

        if tcn and not re.fullmatch(r"[A-Z][A-Za-z0-9]*", stem):
            out.append(Finding("file-pascalcase", f.path, 1, stem, "file name is not PascalCase"))

        guard = expected_guard(f.path)
        if not (re.search(rf"^\s*#\s*ifndef\s+{guard}\b", text, re.M)
                and re.search(rf"^\s*#\s*define\s+{guard}\b", text, re.M)):
            out.append(Finding("include-guard", f.path, 1, guard, f"missing include guard {guard}"))

        if re.search(r"\bmodule\s+enums\b", text):
            out.append(Finding("no-enums-module", f.path, _line_of(text, r"\bmodule\s+enums\b"), "enums",
                               "module enums is not used; put enums next to their types"))

        for d in f.decls:
            if not isinstance(d, (Struct, Union, Enum)):
                continue
            short = d.name.rsplit("::", 1)[-1]
            kw = {Struct: "struct", Union: "union", Enum: "enum"}[type(d)]
            line = _line_of(text, rf"\b{kw}\s+{re.escape(short)}\b")

            want = ("appendable", "final") if tcn else ("final",)
            if d.extensibility not in want:
                out.append(Finding("extensibility-explicit", f.path, line, d.name,
                                   f"{kw} {short} must be declared {' or '.join('@' + w for w in want)}"))

            if tcn and isinstance(d, Enum) and d.enumerators:
                first = d.enumerators[0][0]
                if not (first.endswith("_UNKNOWN") or first.endswith("_NONE")):
                    out.append(Finding("enum-first-unknown", f.path, line, d.name,
                                       f"first enumerator {first} is not *_UNKNOWN / *_NONE"))
                prefix = _upper_snake(short) + "_"
                bad = [n for n, _ in d.enumerators if not n.startswith(prefix)]
                if bad:
                    out.append(Finding("enum-prefix", f.path, line, d.name,
                                       f"enumerators without prefix {prefix}: {', '.join(bad)}"))
    return out


def compare_baseline(findings: Iterable[Finding], baseline: Set[str]):
    """Return (new findings not in the baseline, stale baseline keys that no longer occur)."""
    findings = list(findings)
    keys = {f.key() for f in findings}
    new = [f for f in findings if f.key() not in baseline]
    stale = sorted(baseline - keys)
    return new, stale


def read_baseline(text: str) -> Set[str]:
    return {line for line in text.splitlines() if line and not line.startswith("#")}


def format_baseline(findings: Iterable[Finding]) -> str:
    header = ("# IDL lint baseline: accepted findings as rule<TAB>file<TAB>symbol. This list only shrinks.\n"
              "# Regenerate with: python3 -I tools/idl_cli.py lint --dds-root src/dds "
              "--baseline tools/idl/lint_baseline.txt --update-baseline\n")
    return header + "".join(k + "\n" for k in sorted({f.key() for f in findings}))
