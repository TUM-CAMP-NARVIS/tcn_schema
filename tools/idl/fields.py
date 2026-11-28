"""Remove struct members, delete declarations, make members @optional (0.11 plan 4: F1, F2, F5).

spec: {"remove": {fq: [member]}, "delete": [fq], "optional": {fq: [member]}}.
A removed member takes its comments with it: the comment-only lines directly above it, its trailing comment, and
comment-only continuation lines below it that are indented further than the member. A deleted declaration goes with
its parser span (leading comments and annotations) and one following blank line.
"""
import re
from typing import Dict, List, Tuple

from .model import Struct


def _lines(text: str) -> List[Tuple[int, int]]:
    out, pos = [], 0
    for line in text.split("\n"):
        out.append((pos, pos + len(line)))
        pos += len(line) + 1
    return out


def _code(line: str) -> str:
    return line.split("//", 1)[0]


def _indent(line: str) -> int:
    return len(line) - len(line.lstrip())


def _member_line(text: str, lines, d: Struct, name: str) -> int:
    pat = re.compile(rf"\b{re.escape(name)}\s*(\[[^\]]*\]\s*)*;\s*$")
    hits = [i for i, (s, e) in enumerate(lines)
            if s >= d.span[0] and e <= d.span[1] and pat.search(_code(text[s:e]))]
    if len(hits) != 1:
        raise ValueError(f"{d.name}.{name}: expected one declaration line, found {len(hits)}")
    return hits[0]


def _is_comment(line: str) -> bool:
    return line.strip().startswith("//")


def apply_fields(text: str, decls, spec: dict) -> str:
    lines = _lines(text)
    by_name = {d.name: d for d in decls}
    cuts: List[Tuple[int, int]] = []
    inserts: List[Tuple[int, str]] = []
    for fq, members in spec.get("remove", {}).items():
        d = by_name.get(fq)
        if d is None:
            continue
        for name in members:
            if name not in [m.name for m in d.members]:
                raise ValueError(f"{fq}: no member {name}")
            i = _member_line(text, lines, d, name)
            ind = _indent(text[lines[i][0]:lines[i][1]])
            first = i
            while first - 1 >= 0 and _is_comment(text[lines[first - 1][0]:lines[first - 1][1]]) \
                    and lines[first - 1][0] > d.span[0]:
                first -= 1
            last = i
            while last + 1 < len(lines):
                nxt = text[lines[last + 1][0]:lines[last + 1][1]]
                if _is_comment(nxt) and _indent(nxt) > ind:
                    last += 1
                else:
                    break
            # Between two blank lines: take the following one too, so no double blank line is left.
            if first > 0 and last + 1 < len(lines) and not text[lines[first - 1][0]:lines[first - 1][1]].strip() \
                    and not text[lines[last + 1][0]:lines[last + 1][1]].strip():
                last += 1
            cuts.append((lines[first][0], lines[last][1] + 1))
    for fq in spec.get("delete", []):
        d = by_name.get(fq)
        if d is None:
            continue
        s, e = d.span
        line_start = text.rfind("\n", 0, s) + 1
        if not text[line_start:s].strip():
            s = line_start
        end = text.find("\n", e)
        end = len(text) if end < 0 else end + 1
        if text[end:].startswith("\n"):          # one following blank line
            end += 1
        elif text[end:].lstrip(" ").startswith("\n"):
            end = text.index("\n", end) + 1
        cuts.append((s, end))
    for fq, members in spec.get("optional", {}).items():
        d = by_name.get(fq)
        if d is None:
            continue
        for name in members:
            m = next((m for m in d.members if m.name == name), None)
            if m is None:
                raise ValueError(f"{fq}: no member {name}")
            if m.optional:
                continue
            i = _member_line(text, lines, d, name)
            s, e = lines[i]
            inserts.append((s + _indent(text[s:e]), "@optional "))
    edits = [(s, e, "") for s, e in cuts] + [(at, at, ins) for at, ins in inserts]
    original = text
    for s, e, new in sorted(edits, reverse=True):
        text = text[:s] + new + text[e:]
    # Adjacent removals each keep their own blank-line context; collapse what they leave, unless the file already
    # used double blank lines (then its formatting is not ours to change).
    if not re.search(r"\n[ \t]*\n[ \t]*\n", original):
        text = re.sub(r"\n[ \t]*\n([ \t]*\n)+", "\n\n", text)
    return text
