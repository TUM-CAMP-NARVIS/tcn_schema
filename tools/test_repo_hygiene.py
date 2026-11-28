import re
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

    def test_versions_agree(self):
        cmake = (REPO / "CMakeLists.txt").read_text()
        cmake_version = re.search(r"project\(tcn_schema VERSION (\S+)", cmake).group(1)
        label = re.search(r'set\(TCN_SCHEMA_VERSION_LABEL "([^"]+)"', cmake).group(1)
        conan = re.search(r'version = "([^"]+)"', (REPO / "conanfile.py").read_text()).group(1)
        self.assertEqual(cmake_version, "0.11.0")
        self.assertEqual(conan, label)
        self.assertTrue(conan.startswith(cmake_version))
        self.assertIn('--version "${TCN_SCHEMA_VERSION_LABEL}"', cmake)

    def test_protocol_floor_break_number(self):
        # The number stays 99, provisional until 0.11.0 (controller ruling): "set at 0.11.0 to the engine's B-number".
        h = (REPO / "src/sqe/include/tcn/sqe/protocol.h").read_text()
        self.assertRegex(h, r"#define TCN_SQE_PROTOCOL_BREAK 99\b")
        self.assertIn("set at 0.11.0 to the engine's B-number", h)

    def test_type_map_streams_installed(self):
        self.assertIn("type_map_0_11_streams.json", (REPO / "CMakeLists.txt").read_text())
        self.assertTrue((REPO / "src/dds/tcn/type_map_0_11_streams.json").is_file())


if __name__ == "__main__":
    unittest.main()
