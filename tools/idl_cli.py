"""Command-line entry for the IDL tools; runs under `python3 -I` (which puts no directory on sys.path).

    python3 -I tools/idl_cli.py tables    --dds-root src/dds --version 0.11.0 --out build/share
    python3 -I tools/idl_cli.py lint      --dds-root src/dds --baseline tools/idl/lint_baseline.txt
    python3 -I tools/idl_cli.py roundtrip --types types.json --package tcnart_msgs --out roundtrip_tcnart_msgs.cpp
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.idl.__main__ import main  # noqa: E402

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
