"""Package dependencies as the IDL `#include`s state them."""
from typing import Dict, Set

from .model import Schema


def include_deps(schema: Schema) -> Dict[str, Set[str]]:
    """Map each package to the other packages its IDL files include."""
    deps: Dict[str, Set[str]] = {f.package: set() for f in schema.files}
    for f in schema.files:
        for inc in f.includes:
            pkg = inc.split("/", 1)[0]
            if pkg != f.package:
                deps[f.package].add(pkg)
    return deps
