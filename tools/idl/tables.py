"""types.json and enums.json: machine-readable tables of every IDL declaration (decisions O7)."""
import json
import re
from pathlib import Path

from .model import Const, Enum, Member, Schema, Struct, Typedef, Union


def _member(m: Member) -> dict:
    return {"name": m.name, "type": m.type, "optional": m.optional, "array": m.array}


def _entry(decl, f) -> dict:
    base = {"name": decl.name, "package": f.package, "family": f.family, "file": f.path}
    if isinstance(decl, (Struct, Union)):
        # The type is forward-declared in its file (see _mark_pointer_equality for what that does in fastddsgen 3.0.0).
        base["forward_declared"] = decl.name in f.forward_decls
    if isinstance(decl, Struct):
        return {**base, "kind": "struct", "extensibility": decl.extensibility,
                "members": [_member(m) for m in decl.members]}
    if isinstance(decl, Union):
        return {**base, "kind": "union", "extensibility": decl.extensibility, "discriminator": decl.discriminator,
                "cases": [{"labels": labels, "member": _member(m)} for labels, m in decl.cases]}
    if isinstance(decl, Enum):
        return {**base, "kind": "enum", "extensibility": decl.extensibility,
                "enumerators": [{"name": n, "value": v} for n, v in decl.enumerators]}
    if isinstance(decl, Typedef):
        return {**base, "kind": "typedef", "type": decl.type, "array": decl.array}
    if isinstance(decl, Const):
        return {**base, "kind": "const", "type": decl.type, "value": decl.value}
    raise TypeError(decl)


_NAMED = re.compile(r"[A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_][A-Za-z0-9_]*)+")


def _members(t: dict) -> list:
    return t.get("members") or [c["member"] for c in t.get("cases", [])]


def _mark_pointer_equality(types: list) -> None:
    """fastddsgen 3.0.0 stores a member whose type is a forward-declared union as a raw pointer, and the generated
    operator== compares the pointers. `pointer_members` names those members; `eq_by_pointer` marks every struct or
    union whose operator== is therefore wrong, directly or through a member, sequence element, map value or union
    case of such a type."""
    by_name = {t["name"]: t for t in types}
    fwd_unions = {t["name"] for t in types if t["kind"] == "union" and t.get("forward_declared")}
    aggregates = [t for t in types if t["kind"] in ("struct", "union")]
    for t in aggregates:
        t["pointer_members"] = [m["name"] for m in _members(t) if m["type"] in fwd_unions]
        t["eq_by_pointer"] = bool(t["pointer_members"])
    changed = True
    while changed:
        changed = False
        for t in aggregates:
            if t["eq_by_pointer"]:
                continue
            refs = {n for m in _members(t) for n in _NAMED.findall(m["type"])}
            if any(by_name.get(n, {}).get("eq_by_pointer") for n in refs):
                t["eq_by_pointer"] = changed = True


def build_types(schema: Schema, version: str) -> dict:
    types = [_entry(d, f) for f in schema.files for d in f.decls]
    _mark_pointer_equality(types)
    return {"schema_version": version, "types": types}


def build_enums(schema: Schema, version: str) -> dict:
    return {"schema_version": version,
            "enums": [_entry(d, f) for f in schema.files for d in f.decls if isinstance(d, Enum)]}


def write_tables(schema: Schema, version: str, out: Path) -> None:
    out.mkdir(parents=True, exist_ok=True)
    for name, data in (("types.json", build_types(schema, version)), ("enums.json", build_enums(schema, version))):
        (out / name).write_text(json.dumps(data, indent=1, sort_keys=True) + "\n", encoding="utf-8")
