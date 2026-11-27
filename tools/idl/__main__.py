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

    rt = sub.add_parser("roundtrip", help="generate the C++ round-trip test of one package")
    rt.add_argument("--types", type=Path, required=True)
    rt.add_argument("--package", required=True)
    rt.add_argument("--out", type=Path, required=True)

    rs = sub.add_parser("restructure", help="apply a move list (0.11 plan 3) to <dds-root>/<source>")
    rs.add_argument("--dds-root", type=Path, required=True)
    rs.add_argument("--spec", type=Path, required=True)

    mv = sub.add_parser("moves", help="check a move: every old type deleted or mapped with the same layout")
    mv.add_argument("--old", type=Path, required=True, help="dds root before the move")
    mv.add_argument("--new", type=Path, required=True, help="dds root after the move")
    mv.add_argument("--spec", type=Path, required=True)
    mv.add_argument("--json", type=Path)
    mv.add_argument("--markdown", type=Path)

    an = sub.add_parser("annotate", help="add explicit @appendable/@final to every tcn struct, union and enum (E1, E7)")
    an.add_argument("--dds-root", type=Path, required=True)
    an.add_argument("--final", default="", help="comma-separated fully qualified names to declare @final")

    args = ap.parse_args(argv)
    if args.cmd == "annotate":
        from .annotate import annotate
        final = {x for x in args.final.split(",") if x}
        changed = 0
        for f in load_schema(args.dds_root).files:
            if f.family != "tcn":
                continue
            p = args.dds_root / "tcn" / f.path
            text = p.read_text(encoding="utf-8")
            new = annotate(text, f.decls, final)
            if new != text:
                p.write_text(new, encoding="utf-8")
                changed += 1
        print(f"{changed} files annotated")
        return 0
    if args.cmd == "tables":
        from .tables import write_tables
        write_tables(load_schema(args.dds_root), args.version, args.out)
        return 0
    if args.cmd in ("restructure", "moves"):
        import json
        spec = json.loads(args.spec.read_text(encoding="utf-8"))
        if args.cmd == "restructure":
            from .restructure import restructure
            restructure(args.dds_root, spec, args.dds_root)
            return 0
        from .moves import compare, write_reports
        renames = {o: f"{e['module']}::{o.rsplit('::', 1)[1]}" for e in spec.get("files", []) for o in e["decls"]}
        old, new = load_schema(args.old), load_schema(args.new)
        report = compare(old, new, renames, set(spec.get("deleted", [])), spec.get("new_types", []),
                         spec.get("expect"))
        for p in report.problems:
            print(p)
        write_reports(report, old, new, args.json, args.markdown)
        print(f"{len(report.mapped)} moved, {len(report.deleted)} deleted, {len(report.problems)} problems")
        return 1 if report.problems else 0
    if args.cmd == "roundtrip":
        from .roundtrip import write
        write(args.types, args.package, args.out)
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
