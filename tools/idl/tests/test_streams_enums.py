"""0.11 plan 6: the new and changed enums of Global Constraints, exactly (Review Focus 1: a mirror that keeps old
ordinals decodes plausible wrong values). Ordinal = position; every enum is @appendable with UNKNOWN (or NONE) at
0 (E6). Each task adds the rows of the enums it lands."""
import unittest
from pathlib import Path

from tools.idl.model import Enum
from tools.idl.parser import load_schema

REPO = Path(__file__).resolve().parents[3]
DDS = REPO / "src" / "dds"


def _rows(prefix, *rest):
    """The table's notation: the prefix is written out on the first enumerator only."""
    return [f"{prefix}_UNKNOWN"] + [f"{prefix}_{x}" for x in rest]


ENUMS = {
    # Task 2 (generated from joint_sets.yaml).
    "tcnart_msgs::msg::JointSet": _rows("JOINT_SET", "COCO17", "HALPE26", "TCN_LITE", "TCN_BODY", "SMPL_24",
                                        "SMPLX_55", "SMPLH_52"),
    # Task 3 (HumanPose.idl, PersonTracking.idl).
    "tcnart_msgs::msg::PersonState": _rows("PERSON_STATE", "TRACKED", "COASTING", "LOST"),
    "tcnart_msgs::msg::PersonTrackState": _rows("PERSON_TRACK_STATE", "TENTATIVE", "CONFIRMED", "PREDICTED"),
    "tcnart_msgs::msg::JointSource": _rows("JOINT_SOURCE", "OBSERVED", "DERIVED", "DEFAULTED"),
    "tcnart_msgs::msg::SmplModel": _rows("SMPL_MODEL", "SMPL", "SMPLH", "SMPLX"),
    "tcnart_msgs::msg::SmplGender": _rows("SMPL_GENDER", "NEUTRAL", "FEMALE", "MALE"),
    "tcnart_msgs::msg::PersonSmoothing": _rows("PERSON_SMOOTHING", "OFF", "ONE_EURO"),
}


class StreamsEnumsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.decls = load_schema(DDS).decls()

    def test_enumerators_exact(self):
        for name, want in ENUMS.items():
            with self.subTest(name):
                e = self.decls.get(name)
                self.assertIsInstance(e, Enum, f"{name} is missing")
                self.assertEqual(e.enumerators, [(n, i) for i, n in enumerate(want)])

    def test_appendable(self):
        for name in ENUMS:
            with self.subTest(name):
                self.assertEqual(self.decls[name].extensibility, "appendable")


if __name__ == "__main__":
    unittest.main()
