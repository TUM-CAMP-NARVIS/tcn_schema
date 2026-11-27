import subprocess
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent


class RepoHygieneTest(unittest.TestCase):
    def test_no_bytecode_tracked(self):
        files = subprocess.run(["git", "ls-files"], cwd=REPO, capture_output=True, text=True, check=True).stdout
        self.assertEqual([f for f in files.splitlines() if f.endswith(".pyc") or "__pycache__/" in f], [])

    def test_bytecode_ignored(self):
        r = subprocess.run(["git", "check-ignore", "-q", "tools/idl/__pycache__/x.cpython-312.pyc"], cwd=REPO)
        self.assertEqual(r.returncode, 0)


if __name__ == "__main__":
    unittest.main()
