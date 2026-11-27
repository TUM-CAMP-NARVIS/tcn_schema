import json
import tempfile
import unittest
from pathlib import Path

from tools.ros2.refresh import cache_dir, check_tree, generate
from tools.ros2.rosdistro import parse_distribution
from tools.ros2.tiers import DEFAULT_TIERS, TIERS

FIX = Path(__file__).resolve().parent / "fixtures"
SOURCES = {p.name: p for p in (FIX / "jazzy").iterdir() if p.is_dir()}
TIER_OF = {"std_msgs": "base", "builtin_interfaces": "base", "geometry_msgs": "base", "sensor_msgs": "base",
           "std_srvs": "core", "service_msgs": "core", "unique_identifier_msgs": "core", "tf2_msgs": "core",
           "action_msgs": "core", "nav_msgs": "core", "visualization_msgs": "core"}

DIST = """%YAML 1.1
---
repositories:
  common_interfaces:
    doc:
      type: git
      url: https://github.com/ros2/common_interfaces.git
      version: jazzy
    release:
      packages:
      - geometry_msgs
      - std_msgs
      tags:
        release: release/jazzy/{package}/{version}
      url: https://github.com/ros2-gbp/common_interfaces-release.git
      version: 5.3.8-1
    source:
      type: git
      url: https://github.com/ros2/common_interfaces.git
      version: jazzy
    status: maintained
  control_msgs:
    release:
      url: https://github.com/ros2-gbp/control_msgs-release.git
      version: 5.11.0-1
    source:
      type: git
      url: https://github.com/ros-controls/control_msgs.git
      version: jazzy
type: distribution
version: 2
"""


class TiersTest(unittest.TestCase):
    def test_tiers(self):
        self.assertEqual(DEFAULT_TIERS, ["base", "core"])
        self.assertEqual(TIERS["base"], ["builtin_interfaces", "std_msgs", "geometry_msgs", "sensor_msgs"])
        self.assertIn("type_description_interfaces", TIERS["core"])
        self.assertEqual(TIERS["navigation"], ["nav2_msgs", "geographic_msgs"])


class RosdistroTest(unittest.TestCase):
    def test_parse(self):
        repos = parse_distribution(DIST)
        self.assertEqual(repos["common_interfaces"]["packages"], ["geometry_msgs", "std_msgs"])
        self.assertEqual(repos["common_interfaces"]["source_url"], "https://github.com/ros2/common_interfaces.git")
        self.assertEqual(repos["common_interfaces"]["release_version"], "5.3.8-1")
        self.assertEqual(repos["control_msgs"]["packages"], ["control_msgs"])  # single-package repository


class CacheTest(unittest.TestCase):
    def test_cache_dir_per_commit(self):
        # Several distros pin the same repository at different commits; each checkout needs its own directory, or
        # a later checkout replaces the sources an earlier one handed out.
        a = cache_dir(Path("/c"), "https://github.com/ros2/common_interfaces.git", "a941f14b")
        b = cache_dir(Path("/c"), "https://github.com/ros2/common_interfaces.git", "ea081d8a")
        self.assertNotEqual(a, b)
        self.assertEqual(a.parent, Path("/c"))


class GenerateTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.out = Path(self.tmp.name) / "ros2"
        generate(SOURCES, TIER_OF, self.out, {"jazzy": None})

    def tearDown(self):
        self.tmp.cleanup()

    def test_generate_fixture_tree(self):
        idl = (self.out / "std_msgs" / "msg" / "String.idl").read_text()
        self.assertIn("#ifndef STD_MSGS_MSG_STRING_IDL", idl)
        self.assertIn("@final struct String {", idl)
        srv = (self.out / "std_srvs" / "srv" / "SetBool.idl").read_text()
        self.assertIn("@final struct SetBool_Event {", srv)
        cm = (self.out / "std_srvs" / "CMakeLists.txt").read_text()
        self.assertIn("tcn_add_idl_package(", cm)
        self.assertIn("NAME std_srvs", cm)
        self.assertIn('"srv/SetBool.idl"', cm)
        self.assertTrue((self.out / "builtin_interfaces" / "msg" / "Time.idl").is_file())

    def test_types_json(self):
        types = json.loads((self.out / "ros2_types.json").read_text())
        e = next(t for t in types["types"] if t["ros_name"] == "std_msgs/msg/String")
        self.assertEqual(e["dds_name"], "std_msgs::msg::dds_::String_")
        self.assertEqual(e["cpp_name"], "std_msgs::msg::String")
        self.assertEqual((e["kind"], e["package"], e["tier"]), ("msg", "std_msgs", "base"))
        self.assertEqual(e["hashes"]["jazzy"],
                         "RIHS01_df668c740482bbd48fb39d76a70dfd4bd59db1288021743503259e948f6b1a18")
        kinds = {t["ros_name"]: t["kind"] for t in types["types"]}
        self.assertEqual(kinds["std_srvs/srv/SetBool"], "srv")
        self.assertEqual(kinds["std_srvs/srv/SetBool_Request"], "srv_part")
        self.assertEqual(kinds["tf2_msgs/action/LookupTransform"], "action")
        self.assertEqual(kinds["tf2_msgs/action/LookupTransform_SendGoal"], "action_part")
        self.assertEqual(types["distro"], "jazzy")

    def test_check_detects_drift(self):
        other = Path(self.tmp.name) / "again"
        generate(SOURCES, TIER_OF, other, {"jazzy": None})
        self.assertEqual(check_tree(self.out, other), [])
        p = other / "std_msgs" / "msg" / "String.idl"
        p.write_text(p.read_text() + "// edited\n")
        self.assertEqual(check_tree(self.out, other), ["std_msgs/msg/String.idl"])


if __name__ == "__main__":
    unittest.main()
