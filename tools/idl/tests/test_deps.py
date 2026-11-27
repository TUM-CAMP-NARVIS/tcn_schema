import json
import unittest
from pathlib import Path

from tools.idl.deps import include_deps
from tools.idl.parser import load_schema

REPO = Path(__file__).resolve().parents[3]


class PackageTableTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.deps = include_deps(load_schema(REPO / "src" / "dds"))
        cls.table = json.loads((REPO / "cmake" / "tcn_packages.json").read_text())["packages"]

    def test_include_deps_known_cases(self):
        self.assertEqual(self.deps["std_msgs"], {"builtin_interfaces"})
        self.assertEqual(self.deps["tcnart_msgs"], {"builtin_interfaces", "std_msgs"})
        self.assertEqual(self.deps["conversation_msgs"], set())
        self.assertEqual(self.deps["pcpd_msgs"], {"std_msgs", "tcnart_msgs"})

    def test_table_matches_includes(self):
        idl_entries = [p for p in self.table if p["family"] in ("ros2", "tcn")]
        self.assertEqual({p["name"] for p in idl_entries}, set(self.deps))
        for p in idl_entries:
            self.assertEqual(set(p["deps"]), self.deps[p["name"]], p["name"])

    def test_table_is_topologically_ordered(self):
        seen = set()
        for p in self.table:
            self.assertTrue(set(p["deps"]) <= seen, f"{p['name']} listed before {set(p['deps']) - seen}")
            seen.add(p["name"])

    def test_table_tiers(self):
        from tools.ros2.tiers import TIERS
        tier_of = {p: t for t, ps in TIERS.items() for p in ps}
        ros = [p for p in self.table if p["family"] == "ros2"]
        self.assertEqual({p["name"] for p in ros}, set(tier_of))
        for p in ros:
            self.assertEqual(p["tier"], tier_of[p["name"]], p["name"])
        allowed = {"base": {"base"}, "core": {"base", "core"}}
        by_name = {p["name"]: p for p in self.table}
        for p in ros:
            if p["tier"] in allowed:
                for d in p["deps"]:
                    self.assertIn(by_name[d]["tier"], allowed[p["tier"]], f"{p['name']} -> {d}")
        for p in self.table:
            if p["family"] == "tcn":
                self.assertEqual(p["tier"], "tcn")
                for d in p["deps"]:
                    self.assertIn(by_name[d]["tier"], {"base", "tcn"}, f"{p['name']} -> {d}")

    def test_dirs_exist(self):
        for p in self.table:
            self.assertTrue((REPO / p["dir"]).is_dir(), p["dir"])


if __name__ == "__main__":
    unittest.main()
