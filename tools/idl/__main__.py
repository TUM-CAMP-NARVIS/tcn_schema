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
    mv.add_argument("--plan", default="plan 3", help="provenance for the generated markdown, e.g. 'plan 5'")

    an = sub.add_parser("annotate", help="add explicit @appendable/@final to every tcn struct, union and enum (E1, E7)")
    an.add_argument("--dds-root", type=Path, required=True)
    an.add_argument("--final", default="", help="comma-separated fully qualified names to declare @final")

    en = sub.add_parser("enums", help="apply an enum rename table and write the enum map (0.11 plan 4)")
    en.add_argument("--dds-root", type=Path, required=True)
    en.add_argument("--table", type=Path, required=True)
    en.add_argument("--markdown", type=Path, required=True)

    fl = sub.add_parser("fields", help="remove members, delete types, add @optional (0.11 plan 4)")
    fl.add_argument("--dds-root", type=Path, required=True)
    fl.add_argument("--spec", type=Path, required=True)

    args = ap.parse_args(argv)
    if args.cmd == "fields":
        import json
        from .fields import apply_fields
        spec = json.loads(args.spec.read_text(encoding="utf-8"))
        schema = load_schema(args.dds_root)
        named = set(spec.get("remove", {})) | set(spec.get("delete", [])) | set(spec.get("optional", {}))
        missing = sorted(named - set(schema.decls()))
        if missing:
            raise SystemExit(f"not in the schema: {missing}")
        for f in schema.files:
            if f.family != "tcn":
                continue
            p = args.dds_root / "tcn" / f.path
            text = p.read_text(encoding="utf-8")
            new = apply_fields(text, f.decls, spec)
            if new != text:
                p.write_text(new, encoding="utf-8")
        return 0
    if args.cmd == "enums":
        import json
        from .enums import migration_rows, write_markdown
        spec = json.loads(args.table.read_text(encoding="utf-8"))
        table = spec["enums"]
        schema = load_schema(args.dds_root)
        decls = schema.decls()
        missing = sorted(set(table) - set(decls))
        if missing:
            raise SystemExit(f"not in the schema: {missing}")
        rows = {fq: migration_rows(decls[fq], t) for fq, t in table.items()}
        from .enums import apply_enums
        for f in schema.files:
            if f.family != "tcn":
                continue
            p = args.dds_root / "tcn" / f.path
            text = p.read_text(encoding="utf-8")
            new = apply_enums(text, f.decls, table)
            if new != text:
                p.write_text(new, encoding="utf-8")
        write_markdown(rows, args.markdown, spec["plan"], args.table.name)
        print(f"{len(table)} enums renamed")
        return 0
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
        write_reports(report, old, new, args.json, args.markdown, plan=args.plan, spec_name=args.spec.name)
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
