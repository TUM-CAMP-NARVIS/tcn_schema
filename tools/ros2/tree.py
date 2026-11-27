"""Load a tree of ROS 2 interface sources: <root>/<pkg>/{msg,srv,action}/<Name>.{msg,srv,action}."""
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, Iterable, List

from .emit import expand_action, expand_service
from .rosmsg import ActionSpec, MessageSpec, ServiceSpec, parse_action, parse_message, parse_service


@dataclass
class Tree:
    msgs: Dict[str, MessageSpec] = field(default_factory=dict)      # "pkg/msg/Name"
    srvs: Dict[str, ServiceSpec] = field(default_factory=dict)      # "pkg/srv/Name"
    actions: Dict[str, ActionSpec] = field(default_factory=dict)    # "pkg/action/Name"

    def packages(self) -> List[str]:
        return sorted({k.split("/", 1)[0] for d in (self.msgs, self.srvs, self.actions) for k in d})

    def structs(self) -> Dict[str, MessageSpec]:
        """Every struct, derived service and action types included: "pkg/kind/Name" -> spec."""
        out = dict(self.msgs)
        for s in self.srvs.values():
            for m in expand_service(s):
                out[f"{m.pkg}/srv/{m.name}"] = m
        for a in self.actions.values():
            for m in expand_action(a):
                out[f"{m.pkg}/action/{m.name}"] = m
        return out


def load_package(tree: Tree, pkg: str, pkg_dir: Path) -> None:
    for kind, parse, target in (("msg", parse_message, tree.msgs), ("srv", parse_service, tree.srvs),
                                ("action", parse_action, tree.actions)):
        for f in sorted((pkg_dir / kind).glob(f"*.{kind}")):
            target[f"{pkg}/{kind}/{f.stem}"] = parse(pkg, f.stem, f.read_text(encoding="utf-8"))


def load_sources(root: Path, packages: Iterable[str] = None) -> Tree:
    tree = Tree()
    for pkg_dir in sorted(p for p in root.iterdir() if p.is_dir()):
        if packages is None or pkg_dir.name in packages:
            load_package(tree, pkg_dir.name, pkg_dir)
    return tree
