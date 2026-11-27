"""Subcommands of the IDL tools. Use tools/idl_cli.py to run them under `python3 -I`."""
import argparse
from pathlib import Path

from .parser import load_schema


def main(argv) -> int:
    ap = argparse.ArgumentParser(prog="idl_cli.py")
    sub = ap.add_subparsers(dest="cmd", required=True)

    t = sub.add_parser("tables", help="write types.json and enums.json")
    t.add_argument("--dds-root", type=Path, required=True)
    t.add_argument("--version", required=True)
    t.add_argument("--out", type=Path, required=True)

    li = sub.add_parser("lint", help="check the IDL style rules against a baseline")
    li.add_argument("--dds-root", type=Path, required=True)
    li.add_argument("--baseline", type=Path, required=True)
    li.add_argument("--update-baseline", action="store_true")

    args = ap.parse_args(argv)
    if args.cmd == "tables":
        from .tables import write_tables
        write_tables(load_schema(args.dds_root), args.version, args.out)
        return 0
    if args.cmd == "lint":
        return _lint(args)
    return 2


def _lint(args) -> int:
    from .lint import compare_baseline, format_baseline, lint, read_baseline
    from .parser import FAMILY_DIRS
    schema = load_schema(args.dds_root)
    texts = {}
    for f in schema.files:
        for sub, fam in FAMILY_DIRS.items():
            p = args.dds_root / sub / f.path
            if fam == f.family and p.is_file():
                texts[f.path] = p.read_text(encoding="utf-8")
                break
    findings = lint(schema, texts)
    if args.update_baseline:
        args.baseline.write_text(format_baseline(findings), encoding="utf-8")
        print(f"baseline: {len({f.key() for f in findings})} entries")
        return 0
    baseline = read_baseline(args.baseline.read_text(encoding="utf-8")) if args.baseline.exists() else set()
    new, stale = compare_baseline(findings, baseline)
    for f in new:
        print(f"{f.file}:{f.line}: {f.rule}: {f.message}")
    for k in stale:
        print(f"stale baseline entry (fixed? remove it): {k.replace(chr(9), ' | ')}")
    return 1 if new or stale else 0


if __name__ == "__main__":
    import sys
    sys.exit(main(sys.argv[1:]))
