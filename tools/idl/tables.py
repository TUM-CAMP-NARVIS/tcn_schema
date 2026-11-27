"""types.json and enums.json: machine-readable tables of every IDL declaration (decisions O7)."""
import json
from pathlib import Path

from .model import Const, Enum, Member, Schema, Struct, Typedef, Union


def _member(m: Member) -> dict:
    return {"name": m.name, "type": m.type, "optional": m.optional, "array": m.array}


def _entry(decl, f) -> dict:
    base = {"name": decl.name, "package": f.package, "family": f.family, "file": f.path}
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


def build_types(schema: Schema, version: str) -> dict:
    return {"schema_version": version,
            "types": [_entry(d, f) for f in schema.files for d in f.decls]}


def build_enums(schema: Schema, version: str) -> dict:
    return {"schema_version": version,
            "enums": [_entry(d, f) for f in schema.files for d in f.decls if isinstance(d, Enum)]}


def write_tables(schema: Schema, version: str, out: Path) -> None:
    out.mkdir(parents=True, exist_ok=True)
    for name, data in (("types.json", build_types(schema, version)), ("enums.json", build_enums(schema, version))):
        (out / name).write_text(json.dumps(data, indent=1, sort_keys=True) + "\n", encoding="utf-8")
