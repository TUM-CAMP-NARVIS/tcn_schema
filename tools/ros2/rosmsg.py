"""Parser for ROS 2 interface definitions (.msg / .srv / .action), following rosidl_adapter/parser.py (jazzy).

Only what the IDL emitter and the type hashing need: fields (type, name, default), constants, and the request /
response / goal / result / feedback split. Comments are dropped.
"""
from dataclasses import dataclass, field
from typing import List, Optional, Tuple

PRIMITIVES = {"bool", "byte", "char", "float32", "float64", "int8", "uint8", "int16", "uint16", "int32", "uint32",
              "int64", "uint64", "string", "wstring"}
# rosidl keeps these for compatibility and maps them to builtin_interfaces.
_COMPAT = {"time": ("builtin_interfaces", "Time"), "duration": ("builtin_interfaces", "Duration")}
SEPARATOR = "---"


@dataclass(frozen=True)
class RosType:
    pkg: Optional[str]            # None for primitives
    name: str                     # primitive name, or message name
    array: Optional[Tuple] = None  # None | ("fixed", N) | ("bounded", N) | ("unbounded",)
    string_bound: Optional[int] = None


@dataclass
class Field:
    name: str
    type: RosType
    default: Optional[str] = None


@dataclass
class Constant:
    name: str
    type: str
    value: str


@dataclass
class MessageSpec:
    pkg: str
    name: str
    fields: List[Field] = field(default_factory=list)
    constants: List[Constant] = field(default_factory=list)


@dataclass
class ServiceSpec:
    pkg: str
    name: str
    request: MessageSpec
    response: MessageSpec


@dataclass
class ActionSpec:
    pkg: str
    name: str
    goal: MessageSpec
    result: MessageSpec
    feedback: MessageSpec


class RosParseError(Exception):
    pass


def parse_type(type_string: str, context_pkg: str) -> RosType:
    array = None
    base = type_string
    if type_string.endswith("]"):
        base, _, spec = type_string[:-1].partition("[")
        if spec == "":
            array = ("unbounded",)
        elif spec.startswith("<="):
            array = ("bounded", int(spec[2:]))
        else:
            array = ("fixed", int(spec))
    string_bound = None
    for s in ("string", "wstring"):
        if base.startswith(s + "<="):
            string_bound = int(base[len(s) + 2:])
            base = s
    if base in PRIMITIVES:
        return RosType(None, base, array, string_bound)
    if base in _COMPAT:
        pkg, name = _COMPAT[base]
        return RosType(pkg, name, array)
    parts = base.split("/")
    if len(parts) == 2:
        return RosType(parts[0], parts[1], array)
    if len(parts) == 1:
        return RosType(context_pkg, base, array)
    raise RosParseError(f"invalid type {type_string!r}")


def parse_message(pkg: str, name: str, text: str) -> MessageSpec:
    spec = MessageSpec(pkg, name)
    for raw in text.replace("\t", " ").splitlines():
        line = raw.rstrip()
        if not line:
            continue
        idx = line.find("#")
        if idx >= 0:
            line = line[:idx].rstrip()
        if not line.strip():
            continue
        type_string, _, rest = line.strip().partition(" ")
        rest = rest.lstrip()
        if not rest:
            raise RosParseError(f"{pkg}/{name}: invalid line {raw!r}")
        if "=" in rest:
            cname, _, value = rest.partition("=")
            spec.constants.append(Constant(cname.rstrip(), type_string, value.strip()))
        else:
            fname, _, default = rest.partition(" ")
            default = default.strip() or None
            spec.fields.append(Field(fname, parse_type(type_string, pkg), default))
    return spec


def _split(text: str, n: int, what: str) -> List[str]:
    parts, cur = [], []
    for line in text.splitlines():
        if line.strip() == SEPARATOR:
            parts.append("\n".join(cur))
            cur = []
        else:
            cur.append(line)
    parts.append("\n".join(cur))
    if len(parts) != n:
        raise RosParseError(f"{what}: expected {n - 1} '---' separators, found {len(parts) - 1}")
    return parts


def parse_service(pkg: str, name: str, text: str) -> ServiceSpec:
    req, resp = _split(text, 2, f"{pkg}/{name}")
    return ServiceSpec(pkg, name, parse_message(pkg, name + "_Request", req),
                       parse_message(pkg, name + "_Response", resp))


def parse_action(pkg: str, name: str, text: str) -> ActionSpec:
    goal, result, feedback = _split(text, 3, f"{pkg}/{name}")
    return ActionSpec(pkg, name, parse_message(pkg, name + "_Goal", goal),
                      parse_message(pkg, name + "_Result", result), parse_message(pkg, name + "_Feedback", feedback))
