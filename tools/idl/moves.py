"""Compare two IDL schemas across a move: every old declaration is either deleted or mapped to a new one with the
same layout (members, enumerators, union cases, typedef and const), with type names mapped through the moves.

Used for 0.11 plan 3: the restructure must not change any field (only names, packages, modules and files).
"""
import json
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, Iterable, List, Set, Tuple

from .model import Const, Enum, Schema, Struct, Typedef, Union

_NAME = re.compile(r"[A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_][A-Za-z0-9_]*)+")


@dataclass
class Report:
    mapped: List[Tuple[str, str]] = field(default_factory=list)   # renamed declarations (old, new)
    deleted: List[str] = field(default_factory=list)
    problems: List[str] = field(default_factory=list)
    relocated: List[Tuple[str, str, str]] = field(default_factory=list)  # same name, new header: (name, old, new)
    reshaped: List[str] = field(default_factory=list)  # layout changed on purpose (expect["reshaped"]), by old name


def _mapper(renames: Dict[str, str]):
    def m(text: str) -> str:
        return _NAME.sub(lambda x: renames.get(x.group(0), x.group(0)), text)
    return m


def _member(x, m):
    # Annotations that can matter on the wire or in generated code; comments (@verbatim) and units do not.
    ann = {k: v for k, v in x.annotations.items() if k not in ("verbatim", "unit", "optional")}  # optional: x.optional
    return (x.name, m(x.type), x.optional, x.array, sorted(ann.items()))


def _layout(d, m):
    if isinstance(d, Struct):
        return ("struct", d.extensibility, [_member(x, m) for x in d.members])
    if isinstance(d, Enum):
        return ("enum", d.extensibility, list(d.enumerators))
    if isinstance(d, Union):
        return ("union", d.extensibility, m(d.discriminator),
                [([m(l) for l in labels], _member(x, m)) for labels, x in d.cases])
    if isinstance(d, Typedef):
        return ("typedef", m(d.type), d.array)
    if isinstance(d, Const):
        return ("const", m(d.type), d.value)
    raise TypeError(d)


def _expected(d, name: str, expect: dict, label_map: Dict[str, str]):
    """The old declaration as it should look after the listed, deliberate changes (0.11 plan 4).

    expect keys: "extensibility" {fq or "*": value; "*" applies only where none was declared},
    "removed_members" {fq: [member]}, "optional" {fq: [member]}, "enums" {fq: [[old or None, new], ...]}.
    """
    import dataclasses
    if not isinstance(d, (Struct, Enum, Union)):
        return d
    ext = expect.get("extensibility", {})
    want = ext.get(name) or (ext.get("*") if d.extensibility == "default" else None) or d.extensibility
    d = dataclasses.replace(d, extensibility=want)
    if isinstance(d, Struct):
        gone, opt = set(expect.get("removed_members", {}).get(name, [])), set(expect.get("optional", {}).get(name, []))
        unknown = (gone | opt) - {x.name for x in d.members}
        if unknown:
            raise ValueError(f"{name}: expect names members it does not have: {sorted(unknown)}")
        d = dataclasses.replace(d, members=[dataclasses.replace(x, optional=x.optional or x.name in opt)
                                            for x in d.members if x.name not in gone])
    elif isinstance(d, Enum) and name in expect.get("enums", {}):
        table = expect["enums"][name]
        if sorted(o for o, _ in table if o is not None) != sorted(n for n, _ in d.enumerators):
            raise ValueError(f"{name}: the rename table must list every old enumerator exactly once")
        d = dataclasses.replace(d, enumerators=[(n, i) for i, (_, n) in enumerate(table)])
    elif isinstance(d, Union):
        d = dataclasses.replace(d, cases=[([label_map.get(l, l) for l in labels], x) for labels, x in d.cases])
    return d


def load_expect(expect: dict = None) -> dict:
    """`expect` with `enums_from` resolved: the "enums" of that JSON file (path as given, relative to the working
    directory, which is the repo root for the CLI) are merged into `expect["enums"]`. An enum named both inline and
    in the file is an error."""
    expect = dict(expect or {})
    path = expect.pop("enums_from", None)
    if path:
        table = json.loads(Path(path).read_text(encoding="utf-8"))["enums"]
        inline = expect.get("enums", {})
        both = sorted(set(table) & set(inline))
        if both:
            raise ValueError(f"enums named both inline and in {path}: {both}")
        expect["enums"] = {**inline, **table}
    return expect


def compare(old: Schema, new: Schema, renames: Dict[str, str], deleted: Set[str],
            new_types: Iterable[str] = (), expect: dict = None) -> Report:
    r = Report()
    m = _mapper(renames)
    od, nd = old.decls(), new.decls()
    expect = load_expect(expect)
    reshaped = set(expect.get("reshaped", []))
    label_map = {o: n for t in expect.get("enums", {}).values() for o, n in t if o is not None}
    reached = set()
    for name in sorted(od):
        if name in deleted:
            r.deleted.append(name)
            continue
        target = renames.get(name, name)
        if target not in nd:
            r.problems.append(f"{name}: not deleted, and its new name {target} does not exist")
            continue
        reached.add(target)
        if target != name:
            r.mapped.append((name, target))
        else:
            oh, nh = header_of(name, old), header_of(target, new)
            if oh and nh and oh != nh:
                r.relocated.append((name, oh, nh))
        if name in reshaped:
            # Not layout-compared; but a member that was meant to go must be gone.
            r.reshaped.append(name)
            if isinstance(nd[target], Struct):
                kept = {x.name for x in nd[target].members} & set(expect.get("removed_members", {}).get(name, []))
                for member in sorted(kept):
                    r.problems.append(f"{target}: reshaped, but the removed member {member} still exists")
            continue
        a, b = _layout(_expected(od[name], name, expect, label_map), m), _layout(nd[target], lambda t: t)
        if a != b:
            r.problems.append(f"{target}: layout differs from {name}: {a} != {b}")
    for name in sorted(set(nd) - reached - set(new_types)):
        r.problems.append(f"{name}: new declaration without an old one")
    for name in sorted(deleted - set(od)):
        r.problems.append(f"{name}: listed as deleted but does not exist")
    return r


def header_of(fq: str, schema: Schema) -> str:
    for f in schema.files:
        if any(d.name == fq for d in f.decls):
            return f.path.rsplit(".", 1)[0] + ".h"
    return ""


def write_reports(r: Report, old: Schema, new: Schema, json_path, md_path, plan: str = "plan 3",
                  spec_name: str = "moves_0_11.json") -> None:
    data = {"moved": [{"old": o, "new": n, "old_header": header_of(o, old), "new_header": header_of(n, new)}
                      for o, n in r.mapped],
            "relocated": [{"name": n, "old_header": oh, "new_header": nh} for n, oh, nh in r.relocated],
            "reshaped": [{"name": n, "header": header_of(n, new)} for n in r.reshaped],
            "deleted": [{"old": o, "old_header": header_of(o, old)} for o in r.deleted]}
    if json_path:
        json_path.write_text(json.dumps(data, indent=1, sort_keys=True) + "\n", encoding="utf-8")
    if md_path:
        lines = ["# tcn_schema 0.11: type map (generated)", "",
                 f"Generated by `python3 -I tools/idl_cli.py moves` (0.11 {plan}, from `tools/idl/{spec_name}`). Every",
                 "type that moved, with its old and",
                 "new fully qualified name (C++ namespace and wire type name) and header; every type that kept its name",
                 "but moved to another header; then every deleted type.",
                 "Fields are unchanged by these moves (checked by the same tool), except in the types listed under Reshaped.",
                 "", "## Moved", "", "| Old | New | Old header | New header |", "|---|---|---|---|"]
        lines += [f"| `{x['old']}` | `{x['new']}` | `{x['old_header']}` | `{x['new_header']}` |" for x in data["moved"]]
        lines += ["", "## Same name, new header", "",
                  "These types keep their name (C++ namespace and wire type name) but are declared in another file.",
                  "", "| Type | Old header | New header |", "|---|---|---|"]
        lines += [f"| `{x['name']}` | `{x['old_header']}` | `{x['new_header']}` |" for x in data["relocated"]]
        lines += ["", "## Reshaped", "",
                  "These types keep their name but their members changed on purpose (not layout-compared; the",
                  "migration log describes each).", "", "| Type | Header |", "|---|---|"]
        lines += [f"| `{x['name']}` | `{x['header']}` |" for x in data["reshaped"]]
        lines += ["", "## Deleted", "", "| Type | Header (0.10) |", "|---|---|"]
        lines += [f"| `{x['old']}` | `{x['old_header']}` |" for x in data["deleted"]]
        md_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
