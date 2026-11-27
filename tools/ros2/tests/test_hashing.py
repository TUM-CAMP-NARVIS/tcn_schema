import json
import unittest
from pathlib import Path

from tools.ros2.hashing import hash_all
from tools.ros2.tree import load_sources

FIX = Path(__file__).resolve().parent / "fixtures"


class HashingTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.official = json.loads((FIX / "official_hashes.json").read_text())["hashes"]
        cls.ours = hash_all(load_sources(FIX / "jazzy"))

    def test_string_known_hash(self):
        # rmw_zenoh docs/design.md example
        self.assertEqual(self.ours["std_msgs/msg/String"],
                         "RIHS01_df668c740482bbd48fb39d76a70dfd4bd59db1288021743503259e948f6b1a18")

    def test_official_hashes(self):
        mismatches = {k: (v, self.ours.get(k)) for k, v in self.official.items() if self.ours.get(k) != v}
        self.assertEqual(mismatches, {}, f"{len(mismatches)} of {len(self.official)} differ")

    def test_covers_services_and_actions(self):
        for name in ("std_srvs/srv/SetBool", "std_srvs/srv/SetBool_Event", "tf2_msgs/action/LookupTransform",
                     "tf2_msgs/action/LookupTransform_SendGoal", "tf2_msgs/action/LookupTransform_FeedbackMessage"):
            self.assertIn(name, self.ours)


if __name__ == "__main__":
    unittest.main()
