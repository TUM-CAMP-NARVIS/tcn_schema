"""Read the parts of a rosdistro distribution.yaml that tools/ros2 needs (no PyYAML).

For each repository: the packages it releases, its source URL and branch, and its release version. A repository
without a `release: packages:` list releases one package named like the repository.
"""
from typing import Dict


def parse_distribution(text: str) -> Dict[str, dict]:
    repos: Dict[str, dict] = {}
    repo = section = None
    in_packages = False
    in_repositories = False
    for raw in text.splitlines():
        if not raw.strip() or raw.lstrip().startswith("#"):
            continue
        indent = len(raw) - len(raw.lstrip(" "))
        line = raw.strip()
        if indent == 0:
            in_repositories = line == "repositories:"
            repo = None
            continue
        if not in_repositories:
            continue
        if indent == 2 and line.endswith(":"):
            repo = line[:-1]
            repos[repo] = {"packages": [], "source_url": None, "source_version": None, "release_version": None}
            section, in_packages = None, False
        elif indent == 4 and repo:
            section = line[:-1] if line.endswith(":") else None
            in_packages = False
        elif indent in (6, 8) and repo and in_packages and line.startswith("- "):
            repos[repo]["packages"].append(line[2:].strip())
        elif indent == 6 and repo:
            in_packages = section == "release" and line == "packages:"
            key, _, value = line.partition(":")
            value = value.strip()
            if section == "source" and key == "url":
                repos[repo]["source_url"] = value
            elif section == "source" and key == "version":
                repos[repo]["source_version"] = value
            elif section == "release" and key == "version":
                repos[repo]["release_version"] = value
    for name, r in repos.items():
        if not r["packages"]:
            r["packages"] = [name]
    return repos


def package_index(repos: Dict[str, dict]) -> Dict[str, str]:
    """package -> repository name"""
    return {p: name for name, r in repos.items() for p in r["packages"]}
