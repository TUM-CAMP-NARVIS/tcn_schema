"""ROS 2 type descriptions and RIHS01 type hashes.

A port of rosidl_generator_type_description/__init__.py (ros2/rosidl, branch jazzy), Copyright Open Source Robotics
Foundation, Inc., licensed under the Apache License 2.0. It works from tools/ros2's parsed .msg/.srv/.action
specifications instead of rosidl_parser's IDL model; the serialized description and the hash are the same, which
tools/ros2/tests/test_hashing.py checks against every hash shipped in the Jazzy binary packages of 11 packages.
"""
import hashlib
import json
from typing import Dict

from .rosmsg import MessageSpec, RosType
from .tree import Tree

RIHS01_PREFIX = "RIHS01_"

# type_description_interfaces/msg/FieldType.msg
_BASE_ID = {"NESTED_TYPE": 1, "INT8": 2, "UINT8": 3, "INT16": 4, "UINT16": 5, "INT32": 6, "UINT32": 7, "INT64": 8,
            "UINT64": 9, "FLOAT": 10, "DOUBLE": 11, "LONG_DOUBLE": 12, "CHAR": 13, "WCHAR": 14, "BOOLEAN": 15,
            "BYTE": 16, "STRING": 17, "WSTRING": 18, "FIXED_STRING": 19, "FIXED_WSTRING": 20,
            "BOUNDED_STRING": 21, "BOUNDED_WSTRING": 22}
_SUFFIX_OFFSET = {None: 0, "fixed": 48, "bounded": 96, "unbounded": 144}
# .msg primitive -> FieldType base name; rosidl maps msg `char` to IDL uint8 and `byte` to octet.
_PRIMITIVE = {"bool": "BOOLEAN", "byte": "BYTE", "char": "UINT8", "float32": "FLOAT", "float64": "DOUBLE",
              "int8": "INT8", "uint8": "UINT8", "int16": "INT16", "uint16": "UINT16", "int32": "INT32",
              "uint32": "UINT32", "int64": "INT64", "uint64": "UINT64", "string": "STRING", "wstring": "WSTRING"}


def _type_id(t: RosType) -> int:
    if t.pkg is not None:
        base = "NESTED_TYPE"
    else:
        base = _PRIMITIVE[t.name]
        if t.string_bound is not None:
            base = "BOUNDED_" + base
    return _BASE_ID[base] + _SUFFIX_OFFSET[t.array[0] if t.array else None]


def _field_type(t: RosType) -> dict:
    return {
        "type_id": _type_id(t),
        "capacity": t.array[1] if t.array and t.array[0] in ("fixed", "bounded") else 0,
        "string_capacity": t.string_bound or 0,
        "nested_type_name": f"{t.pkg}/{t.kind}/{t.name}" if t.pkg is not None else "",
    }


def _field(name: str, t: RosType) -> dict:
    return {"name": name, "type": _field_type(t), "default_value": ""}


def _struct(type_name: str, spec: MessageSpec) -> dict:
    fields = [_field(f.name, f.type) for f in spec.fields]
    if not fields:  # rosidl's IDL gives an empty message this placeholder member, and hashes it
        fields = [_field("structure_needs_at_least_one_member", RosType(None, "uint8"))]
    return {"type_name": type_name, "fields": fields}


def _generic(type_name: str, members) -> dict:
    return {"type_name": type_name, "fields": [_field(n, RosType(p, nm, None, None, k)) for n, (p, k, nm) in members]}


def individual_descriptions(tree: Tree) -> Dict[str, dict]:
    out = {name: _struct(name, spec) for name, spec in tree.structs().items()}

    def service(pkg, kind, base):
        out[f"{pkg}/{kind}/{base}"] = _generic(f"{pkg}/{kind}/{base}", [
            ("request_message", (pkg, kind, base + "_Request")),
            ("response_message", (pkg, kind, base + "_Response")),
            ("event_message", (pkg, kind, base + "_Event"))])

    for s in tree.srvs.values():
        service(s.pkg, "srv", s.name)
    for a in tree.actions.values():
        p, n = a.pkg, a.name
        out[f"{p}/action/{n}"] = _generic(f"{p}/action/{n}", [
            ("goal", (p, "action", n + "_Goal")), ("result", (p, "action", n + "_Result")),
            ("feedback", (p, "action", n + "_Feedback")),
            ("send_goal_service", (p, "action", n + "_SendGoal")),
            ("get_result_service", (p, "action", n + "_GetResult")),
            ("feedback_message", (p, "action", n + "_FeedbackMessage"))])
        service(p, "action", n + "_SendGoal")
        service(p, "action", n + "_GetResult")
    return out


def full_description(type_name: str, type_map: Dict[str, dict]) -> dict:
    """rosidl extract_full_type_description: the type and every type it references, sorted by name."""
    refs, queue = set(), [f["type"]["nested_type_name"] for f in type_map[type_name]["fields"]
                          if f["type"]["nested_type_name"]]
    while queue:
        t = queue.pop()
        if t not in refs:
            refs.add(t)
            queue.extend(f["type"]["nested_type_name"] for f in type_map[t]["fields"]
                         if f["type"]["nested_type_name"])
    return {"type_description": type_map[type_name],
            "referenced_type_descriptions": [type_map[t] for t in sorted(refs)]}


def type_hash(description: dict) -> str:
    """rosidl calculate_type_hash: drop default values, dump with the upstream separators, SHA-256."""
    d = json.loads(json.dumps(description))
    for td in [d["type_description"]] + d["referenced_type_descriptions"]:
        for f in td["fields"]:
            del f["default_value"]
    text = json.dumps(d, skipkeys=False, ensure_ascii=True, check_circular=True, allow_nan=False, indent=None,
                      separators=(", ", ": "), sort_keys=False)
    return RIHS01_PREFIX + hashlib.sha256(text.encode("utf-8")).hexdigest()


def hash_all(tree: Tree) -> Dict[str, str]:
    """RIHS01 hash of every type in the tree (messages, services, actions and all their parts)."""
    type_map = individual_descriptions(tree)
    return {name: type_hash(full_description(name, type_map)) for name in sorted(type_map)}
