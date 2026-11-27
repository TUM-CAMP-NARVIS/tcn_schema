"""Emit fastddsgen-ready IDL for ROS 2 interfaces, and expand services and actions into their derived types.

The IDL follows rosidl_adapter's msg -> idl conversion (jazzy): the same type mapping, `<Name>_Constants` modules,
`@default`, and `structure_needs_at_least_one_member` for empty messages. Differences, all decided in tcn_schema 0.11
(decisions §2 E2, §5 R5): every struct is `@final`; every file has an include guard; fixed arrays are member
declarators (`double k[9];`) instead of typedefs; comments are not carried over. The derived service and action
types follow rosidl_parser/definition.py (jazzy).
"""
from typing import List

from .rosmsg import ActionSpec, Field, MessageSpec, RosType, ServiceSpec

IDL_PRIMITIVE = {"bool": "boolean", "byte": "octet", "char": "uint8", "int8": "int8", "uint8": "uint8",
                 "int16": "int16", "uint16": "uint16", "int32": "int32", "uint32": "uint32", "int64": "int64",
                 "uint64": "uint64", "float32": "float", "float64": "double", "string": "string",
                 "wstring": "wstring"}

# IDL 4.2 keywords (7.2.4) and the fastddsgen 3.0.0 grammar's additional ones; a field named like one of them is
# written as the escaped identifier `_<name>` (IDL 4.2 7.2.3), which is the same identifier.
IDL_KEYWORDS = {
    "abstract", "any", "alias", "attribute", "bitfield", "bitmask", "bitset", "boolean", "case", "char", "component",
    "connector", "const", "consumes", "context", "custom", "default", "double", "exception", "emits", "enum",
    "eventtype", "factory", "false", "finder", "fixed", "float", "getraises", "getter", "home", "import", "in",
    "inout", "interface", "local", "long", "manages", "map", "mirrorport", "module", "multiple", "native", "object",
    "octet", "oneway", "out", "primarykey", "private", "port", "porttype", "provides", "public", "publishes",
    "raises", "readonly", "setraises", "setter", "sequence", "short", "string", "struct", "supports", "switch",
    "true", "truncatable", "typedef", "typeid", "typename", "typeprefix", "unsigned", "union", "uses", "valuebase",
    "valuetype", "void", "wchar", "wstring", "int8", "uint8", "int16", "int32", "int64", "uint16", "uint32",
    "uint64", "annotation",
}

UUID = RosType("unique_identifier_msgs", "UUID")
TIME = RosType("builtin_interfaces", "Time")
EVENT_INFO = RosType("service_msgs", "ServiceEventInfo")


def _ref(pkg: str, name: str, kind: str, array=None) -> RosType:
    return RosType(pkg, name, array, None, kind)


def _event(pkg: str, base: str, kind: str) -> MessageSpec:
    return MessageSpec(pkg, base + "_Event", [
        Field("info", EVENT_INFO),
        Field("request", _ref(pkg, base + "_Request", kind, ("bounded", 1))),
        Field("response", _ref(pkg, base + "_Response", kind, ("bounded", 1)))], [], kind)


def expand_service(spec: ServiceSpec) -> List[MessageSpec]:
    """Foo_Request, Foo_Response, Foo_Event (rosidl_parser Service)."""
    return [spec.request, spec.response, _event(spec.pkg, spec.name, "srv")]


def expand_action(spec: ActionSpec) -> List[MessageSpec]:
    """The ten structs of an action, in rosidl_parser Action order."""
    p, n = spec.pkg, spec.name
    goal = _ref(p, n + "_Goal", "action")
    result = _ref(p, n + "_Result", "action")
    feedback = _ref(p, n + "_Feedback", "action")
    send, get = n + "_SendGoal", n + "_GetResult"
    return [
        spec.goal, spec.result, spec.feedback,
        MessageSpec(p, send + "_Request", [Field("goal_id", UUID), Field("goal", goal)], [], "action"),
        MessageSpec(p, send + "_Response", [Field("accepted", RosType(None, "bool")), Field("stamp", TIME)], [],
                    "action"),
        _event(p, send, "action"),
        MessageSpec(p, get + "_Request", [Field("goal_id", UUID)], [], "action"),
        MessageSpec(p, get + "_Response", [Field("status", RosType(None, "int8")), Field("result", result)], [],
                    "action"),
        _event(p, get, "action"),
        MessageSpec(p, n + "_FeedbackMessage", [Field("goal_id", UUID), Field("feedback", feedback)], [], "action"),
    ]


def idl_base(t: RosType) -> str:
    if t.pkg is None:
        base = IDL_PRIMITIVE[t.name]
        if t.string_bound is not None:
            base += f"<{t.string_bound}>"
        return base
    return f"{t.pkg}::{t.kind}::{t.name}"


def idl_type(t: RosType) -> str:
    """The member type; a fixed array's dimension goes on the declarator instead (see _declarator)."""
    base = idl_base(t)
    if t.array is None or t.array[0] == "fixed":
        return base
    if t.array[0] == "unbounded":
        return f"sequence<{base}>"
    return f"sequence<{base}, {t.array[1]}>"


def _declarator(f: Field) -> str:
    name = "_" + f.name if f.name.lower() in IDL_KEYWORDS else f.name
    if f.type.array and f.type.array[0] == "fixed":
        return f"{name}[{f.type.array[1]}]"
    return name


def _string_literal(value: str) -> str:
    v = value
    if len(v) >= 2 and v[0] == v[-1] and v[0] in "'\"":
        v = v[1:-1]
    v = v.encode().decode("unicode_escape")
    return '"' + v.replace('"', r'\"') + '"'


def _scalar(prim: str, value: str):
    """A Python value for an element of a ROS default / constant, as rosidl parses it."""
    if prim == "bool":
        return value.strip().lower() in ("true", "1")
    if prim in ("float32", "float64"):
        return float(value)
    if prim in ("string", "wstring"):
        v = value.strip()
        if len(v) >= 2 and v[0] == v[-1] and v[0] in "'\"":
            v = v[1:-1]
        return v
    return int(value, 0)


def _literal(t: RosType, value: str) -> str:
    """rosidl_adapter to_idl_literal."""
    if t.array is not None:
        inner = value.strip()
        if inner.startswith("[") and inner.endswith("]"):
            inner = inner[1:-1]
        items = tuple(_scalar(t.name, x) for x in _split_list(inner))
        content = repr(items).replace("\\", r"\\").replace('"', r'\"')
        return f'"{content}"'
    if t.name == "bool":
        return "TRUE" if _scalar("bool", value) else "FALSE"
    if t.name in ("string", "wstring"):
        return _string_literal(value)
    return value.strip()


def _split_list(s: str) -> List[str]:
    out, cur, q = [], "", None
    for ch in s:
        if q:
            cur += ch
            if ch == q:
                q = None
        elif ch in "'\"":
            q = ch
            cur += ch
        elif ch == ",":
            out.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def _const_type(type_string: str) -> str:
    for s in ("string", "wstring"):
        if type_string.startswith(s + "<="):
            return f"{s}<{type_string[len(s) + 2:]}>"
    return IDL_PRIMITIVE[type_string]


def guard(pkg: str, kind: str, stem: str) -> str:
    return f"{pkg}_{kind}_{stem}_IDL".upper()


def emit_idl(pkg: str, kind: str, stem: str, specs: List[MessageSpec]) -> str:
    defined = {(s.pkg, s.kind, s.name) for s in specs}
    includes = set()
    for s in specs:
        for f in s.fields:
            t = f.type
            if t.pkg is not None and (t.pkg, t.kind, t.name) not in defined:
                includes.add(f"{t.pkg}/{t.kind}/{t.name}.idl")
    g = guard(pkg, kind, stem)
    out = [f"// Generated by tools/ros2 from ROS 2 {pkg}/{kind}/{stem} (see src/dds/ros2/UPSTREAM.lock); do not edit.",
           f"#ifndef {g}", f"#define {g}", ""]
    out += [f'#include "{inc}"' for inc in sorted(includes)]
    if includes:
        out.append("")
    out += [f"module {pkg} {{", f"  module {kind} {{"]
    for s in specs:
        if s.constants:
            out.append(f"    module {s.name}_Constants {{")
            for c in s.constants:
                ct = _const_type(c.type)
                value = _string_literal(c.value) if ct.startswith(("string", "wstring")) else \
                    ("TRUE" if _scalar("bool", c.value) else "FALSE") if ct == "boolean" else c.value.strip()
                out.append(f"      const {ct} {c.name} = {value};")
            out.append("    };")
        out.append(f"    @final struct {s.name} {{")
        if not s.fields:
            out.append("      uint8 structure_needs_at_least_one_member;")
        for f in s.fields:
            if f.default is not None:
                out.append(f"      @default (value={_literal(f.type, f.default)})")
            out.append(f"      {idl_type(f.type)} {_declarator(f)};")
        out.append("    };")
    out += ["  };", "};", "", "#endif", ""]
    return "\n".join(out)
