"""Insert explicit extensibility annotations (0.11 plan 4, E1/E7).

Every struct, union and enum without `@final`/`@appendable`/`@mutable` gets `@appendable ` (or `@final ` when its
fully qualified name is in `final`) directly before its keyword. Comments and other annotations stay where they are.
"""
import re
from typing import Iterable, Set

from .model import Enum, Struct, Union

_KW = {Struct: "struct", Union: "union", Enum: "enum"}


def _keyword_offset(text: str, start: int, end: int, kw: str, short: str) -> int:
    pat = re.compile(rf"\b{kw}\s+{re.escape(short)}\b")
    pos = start
    while True:
        m = pat.search(text, pos, end)
        if m is None:
            raise ValueError(f"{kw} {short}: keyword not found in its span")
        line_start = text.rfind("\n", 0, m.start()) + 1
        if "//" not in text[line_start:m.start()]:
            return m.start()
        pos = m.end()


def annotate(text: str, decls: Iterable, final: Set[str]) -> str:
    edits = []
    for d in decls:
        kw = _KW.get(type(d))
        if kw is None or d.extensibility != "default":
            continue
        at = _keyword_offset(text, d.span[0], d.span[1], kw, d.name.rsplit("::", 1)[-1])
        edits.append((at, "@final " if d.name in final else "@appendable "))
    for at, ins in sorted(edits, reverse=True):
        text = text[:at] + ins + text[at:]
    return text
