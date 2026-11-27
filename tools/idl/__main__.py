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

    args = ap.parse_args(argv)
    if args.cmd == "tables":
        from .tables import write_tables
        write_tables(load_schema(args.dds_root), args.version, args.out)
        return 0
    return 2


if __name__ == "__main__":
    import sys
    sys.exit(main(sys.argv[1:]))
