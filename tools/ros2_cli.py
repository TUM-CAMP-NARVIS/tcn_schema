"""Command-line entry for tools/ros2; runs under `python3 -I` (which puts no directory on sys.path).

    python3 -I tools/ros2_cli.py refresh [--update-lock] [--hash-distros kilted,lyrical] [--cache DIR]
    python3 -I tools/ros2_cli.py check   [--cache DIR]
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.ros2.refresh import main  # noqa: E402

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
