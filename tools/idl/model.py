"""Data model of the parsed IDL (the subset tcn_schema uses).

All declaration names are fully qualified (`pkg::msg::Name`). Member and typedef types are rendered as IDL text with
named types fully qualified, e.g. `sequence<tcnart_msgs::msg::Target>`, `string<8>`, `unsigned long`.
"""
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple, Union as _U


@dataclass
class Member:
    name: str
    type: str
    optional: bool = False
    array: Optional[List[int]] = None
    annotations: Dict[str, str] = field(default_factory=dict)


@dataclass
class Struct:
    name: str
    members: List[Member]
    extensibility: str = "default"  # final | appendable | mutable | default
    annotations: Dict[str, str] = field(default_factory=dict)


@dataclass
class Enum:
    name: str
    enumerators: List[Tuple[str, int]]
    extensibility: str = "default"
    annotations: Dict[str, str] = field(default_factory=dict)


@dataclass
class Union:
    name: str
    discriminator: str
    cases: List[Tuple[List[str], Member]]  # labels are enumerator names / literals, or "default"
    extensibility: str = "default"
    annotations: Dict[str, str] = field(default_factory=dict)


@dataclass
class Typedef:
    name: str
    type: str
    array: Optional[List[int]] = None


@dataclass
class Const:
    name: str
    type: str
    value: str


Decl = _U[Struct, Enum, Union, Typedef, Const]


@dataclass
class IdlFile:
    path: str        # relative to src/dds/<family dir>/, e.g. "tcnart_msgs/msg/Types.idl"
    package: str     # "tcnart_msgs"
    kind: str        # "msg" | "rpc" | "action"
    includes: List[str]
    decls: List[Decl]
    family: str = "tcn"   # "ros2" | "tcn"


@dataclass
class Schema:
    files: List[IdlFile]
    family: Dict[str, str]  # package -> "ros2" | "tcn"

    def decls(self) -> Dict[str, Decl]:
        return {d.name: d for f in self.files for d in f.decls}
