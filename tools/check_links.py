"""Report relative Markdown links whose target file does not exist.

Usage: python3 -I tools/check_links.py <root>

Exit 0 when every relative link resolves; otherwise print `file:line: broken link <target>` for each broken one and
exit 1. URLs (`scheme://`, `mailto:`) and pure anchors (`#...`) are ignored; a `path#anchor` link is checked for the
path only. Directories named `build`, `.git`, `.superpowers` and `node_modules` are skipped.
"""
import re
import sys
from pathlib import Path

_LINK = re.compile(r"\[[^\]]*\]\(([^)\s]+)(?:\s+\"[^\"]*\")?\)")
_CODE_SPAN = re.compile(r"`[^`]*`")
_SKIP_DIRS ={"build", ".git", ".superpowers", "node_modules"}


def _is_external(target: str) -> bool:
    return target.startswith("#") or "://" in target or target.startswith("mailto:")


def find_broken_links(root: Path) -> list:
    broken = []
    for md in sorted(root.rglob("*.md")):
        if any(part in _SKIP_DIRS for part in md.relative_to(root).parts):
            continue
        in_fence = False
        for lineno, line in enumerate(md.read_text(encoding="utf-8").splitlines(), start=1):
            if line.lstrip().startswith("```"):
                in_fence = not in_fence
                continue
            if in_fence:
                continue
            for target in _LINK.findall(_CODE_SPAN.sub("", line)):
                if _is_external(target):
                    continue
                path = target.split("#", 1)[0]
                if path and not (md.parent / path).exists():
                    broken.append((md, lineno, target))
    return broken


def main(argv: list) -> int:
    root = Path(argv[1] if len(argv) > 1 else ".")
    broken = find_broken_links(root)
    for md, lineno, target in broken:
        print(f"{md}:{lineno}: broken link {target}")
    return 1 if broken else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
