"""The joint-set table (decisions §7 HumanPose): load, check, and the generated outputs that must match it."""
import copy
import dataclasses
import re
import unittest
from pathlib import Path

from tools.idl import joint_sets as js
from tools.idl.model import Enum
from tools.idl.parser import load_schema

REPO = Path(__file__).resolve().parents[3]
TABLE = REPO / "src" / "dds" / "tcn" / "tcnart_msgs" / "msg" / "joint_sets.yaml"

ENUM_ORDER = ["JOINT_SET_UNKNOWN", "JOINT_SET_COCO17", "JOINT_SET_HALPE26", "JOINT_SET_TCN_LITE",
              "JOINT_SET_TCN_BODY", "JOINT_SET_SMPL_24", "JOINT_SET_SMPLX_55", "JOINT_SET_SMPLH_52"]


def _set(t, set_id):
    return next(s for s in t.sets if s.id == set_id)


def _names(s):
    return [j.name for j in s.joints]


class JointSetTableTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = TABLE.read_text(encoding="utf-8")
        cls.table = js.load_table(cls.text)

    def fresh(self):
        return copy.deepcopy(self.table)

    def test_counts(self):
        want = {"COCO17": (17, 16), "HALPE26": (26, 26), "TCN_LITE": (7, 2), "TCN_BODY": (20, 19),
                "SMPL_24": (24, 0), "SMPLX_55": (55, 0), "SMPLH_52": (52, 0)}
        self.assertEqual({s.id: (len(s.joints), len(s.bones)) for s in self.table.sets}, want)

    def test_hand_expansion(self):
        x = _set(self.table, "SMPLX_55")
        self.assertEqual((x.joints[25].name, x.joints[25].parent), ("left_index1", "left_wrist"))
        self.assertEqual((x.joints[26].name, x.joints[26].parent), ("left_index2", "left_index1"))
        self.assertEqual(x.joints[54].name, "right_thumb3")
        h = _set(self.table, "SMPLH_52")
        self.assertEqual(h.joints[22].name, "left_index1")
        self.assertEqual(h.joints[51].name, "right_thumb3")

    def test_range_expansion(self):
        coco, halpe = _set(self.table, "COCO17"), _set(self.table, "HALPE26")
        self.assertEqual(_names(halpe)[:17], _names(coco))
        self.assertEqual(halpe.joints[17].name, "head_top")
        self.assertEqual([(b.frm, b.to) for b in halpe.bones[:16]], [(b.frm, b.to) for b in coco.bones])
        self.assertEqual(_names(_set(self.table, "SMPLX_55"))[:22], _names(_set(self.table, "SMPL_24"))[:22])

    def test_defaults_and_extra_fields(self):
        body = _set(self.table, "TCN_BODY")
        self.assertEqual((body.kind, body.root, body.rest_pose, body.replaces), ("tree", "pelvis", "t_pose",
                                                                                 ["tcnart_body_v1"]))
        self.assertEqual(body.bones[5].rest_dir, (1.0, 0.0, 0.0))
        self.assertIn("UNVERIFIED", _set(self.table, "SMPL_24").verified)
        self.assertTrue(_set(self.table, "SMPLX_55").pose_excludes_root)
        self.assertEqual(_set(self.table, "COCO17").root, {"midpoint": ["left_hip", "right_hip"]})
        self.assertEqual(_set(self.table, "TCN_LITE").joints[1].source, "DERIVED")

    def test_real_table_checks_clean(self):
        self.assertEqual(js.check_table(self.table), [])
        self.assertEqual(js.check_table(self.table, previous=self.fresh()), [])

    def assertOneError(self, t, set_id, previous=None):
        errors = js.check_table(t, previous)
        self.assertEqual(len(errors), 1, errors)
        self.assertIn(set_id, errors[0])

    def test_duplicate_name(self):
        t = self.fresh()
        s = _set(t, "TCN_BODY")
        s.joints[19] = dataclasses.replace(s.joints[19], name="left_toe")
        s.bones[18] = dataclasses.replace(s.bones[18], to="left_toe")
        errors = js.check_table(t)
        self.assertTrue(errors and all("TCN_BODY" in e for e in errors), errors)
        self.assertTrue(any("duplicate" in e for e in errors), errors)
        t = self.fresh()
        s = _set(t, "HALPE26")  # appended to the extending set, so COCO17 stays its prefix
        s.joints.append(js.Joint("nose", None, None, None))
        self.assertOneError(t, "HALPE26")

    def test_parent_cycle(self):
        t = self.fresh()
        s = _set(t, "TCN_BODY")
        s.joints[1] = dataclasses.replace(s.joints[1], parent="chest")  # spine <-> chest
        self.assertOneError(t, "TCN_BODY")

    def test_rest_dir_not_unit(self):
        t = self.fresh()
        s = _set(t, "TCN_BODY")
        s.bones[0] = dataclasses.replace(s.bones[0], rest_dir=(0.0, 2.0, 0.0))
        self.assertOneError(t, "TCN_BODY")

    def test_ordinal_gap(self):
        t = self.fresh()
        s = _set(t, "SMPLH_52")
        s.ordinal = 9
        self.assertOneError(t, "SMPLH_52")

    def test_extends_not_a_prefix(self):
        t = self.fresh()
        s = _set(t, "HALPE26")
        s.joints[3] = js.Joint("left_ear_tip", None, None, None)
        s.bones = [dataclasses.replace(b, to="left_ear_tip") if b.to == "left_ear" else b for b in s.bones]
        self.assertOneError(t, "HALPE26")

    def test_changed_set_needs_new_version(self):
        previous = self.fresh()
        s = _set(previous, "TCN_BODY")
        s.joints[18] = dataclasses.replace(s.joints[18], name="left_foot")
        s.bones[14] = dataclasses.replace(s.bones[14], to="left_foot")
        self.assertOneError(self.fresh(), "TCN_BODY", previous=previous)
        t = self.fresh()
        _set(t, "TCN_BODY").version = 2
        self.assertEqual(js.check_table(t, previous), [])

    def test_other_rules(self):
        t = self.fresh()
        _set(t, "TCN_LITE").joints[0] = dataclasses.replace(_set(t, "TCN_LITE").joints[0], source="GUESSED")
        self.assertOneError(t, "TCN_LITE")
        t = self.fresh()
        _set(t, "TCN_BODY").bones[3] = dataclasses.replace(_set(t, "TCN_BODY").bones[3], frm="chest")
        self.assertOneError(t, "TCN_BODY")
        t = self.fresh()
        _set(t, "TCN_BODY").bones[3] = dataclasses.replace(_set(t, "TCN_BODY").bones[3], rest_dir=None)
        self.assertOneError(t, "TCN_BODY")
        t = self.fresh()
        _set(t, "TCN_BODY").joints[1] = dataclasses.replace(_set(t, "TCN_BODY").joints[1], parent="ghost")
        self.assertOneError(t, "TCN_BODY")
        t = self.fresh()
        s = _set(t, "HALPE26")
        s.bones.append(js.Bone("nose", "ghost", None))
        self.assertOneError(t, "HALPE26")
        t = self.fresh()
        s = _set(t, "SMPLX_55")
        s.joints += [js.Joint(f"extra{i}", "jaw", None, None) for i in range(9)]  # 64 joints: pose would hold 63
        self.assertOneError(t, "SMPLX_55")
        t = self.fresh()
        s = _set(t, "COCO17")
        s.joints += [js.Joint(f"extra{i}", None, None, None) for i in range(48)]  # 65 joints
        errors = js.check_table(t)
        self.assertTrue(errors and any("64" in e and "COCO17" in e for e in errors), errors)

    def test_bone_bound(self):
        # HumanShape.bone_lengths holds at most 64: one per bone of the set's bone list.
        t = self.fresh()
        s = _set(t, "HALPE26")
        s.bones += [js.Bone("nose", "left_eye", None) for _ in range(65 - len(s.bones))]  # 65 bones
        errors = js.check_table(t)
        self.assertEqual(len(errors), 1, errors)
        self.assertIn("HALPE26", errors[0])
        self.assertIn("65 bones", errors[0])
        self.assertIn("HumanShape.bone_lengths", errors[0])
        t = self.fresh()
        s = _set(t, "HALPE26")
        s.bones += [js.Bone("nose", "left_eye", None) for _ in range(64 - len(s.bones))]  # 64 bones: fits
        self.assertEqual(js.check_table(t), [])

    def test_markdown_states_person_tracking_bound(self):
        md = js.render_markdown(self.table)
        self.assertIn("`PersonTrackingMessage` carries only a set with at most 32 joints and 32 bones", md)

    def test_load_rejects_unknown_keys_and_bad_shorthands(self):
        with self.assertRaises(ValueError):
            js.load_table(self.text.replace("    purpose: the joint order of SmplParams.pose for SMPL-X; no model data\n",
                                            "    porpose: x\n"))
        with self.assertRaises(ValueError):
            js.load_table(self.text.replace('{range: "0-21", same_as: SMPL_24}', '{range: "0-21", same_as: NOPE}'))
        with self.assertRaises(ValueError):
            js.load_table(self.text.replace('{range: "0-16", same_as: COCO17}', '{range: "0-17", same_as: COCO17}'))


class JointSetOutputsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.table = js.load_table(TABLE.read_text(encoding="utf-8"))

    def test_idl_enum_order(self):
        idl = js.render_idl(self.table)
        self.assertEqual(re.findall(r"\b(JOINT_SET_[A-Z0-9_]+)\b", idl.split("enum JointSet", 1)[1]), ENUM_ORDER)
        self.assertIn("@appendable enum JointSet", idl)
        self.assertIn("#ifndef TCNART_MSGS_MSG_JOINTSET_IDL", idl)

    def test_generated_enum_matches_yaml(self):
        e = load_schema(REPO / "src" / "dds").decls()["tcnart_msgs::msg::JointSet"]
        self.assertIsInstance(e, Enum)
        self.assertEqual(e.extensibility, "appendable")
        by_ordinal = {s.ordinal: s.id for s in self.table.sets}
        self.assertEqual(e.enumerators[0], ("JOINT_SET_UNKNOWN", 0))
        self.assertEqual(len(e.enumerators), len(by_ordinal) + 1)
        for name, value in e.enumerators[1:]:
            self.assertEqual(name, "JOINT_SET_" + by_ordinal[value])

    def test_outputs_match_renderers(self):
        self.assertEqual(set(js.OUTPUTS), {
            "src/dds/tcn/tcnart_msgs/msg/JointSet.idl", "src/dds/tcn/tcnart_msgs/msg/JointSets.md",
            "src/joint_sets/include/tcn/joint_sets.h", "src/joint_sets/cs/JointSets.cs",
            "src/joint_sets/joint_sets.json"})
        for rel, render in js.OUTPUTS.items():
            with self.subTest(rel=rel):
                self.assertEqual((REPO / rel).read_text(encoding="utf-8"), render(self.table),
                                 "regenerate: python3 -I tools/idl_cli.py joint-sets --table " +
                                 str(TABLE.relative_to(REPO)))

    def test_json(self):
        import json
        doc = json.loads(js.render_json(self.table))
        body = next(s for s in doc["joint_sets"] if s["id"] == "TCN_BODY")
        self.assertEqual((body["enum"], body["ordinal"], body["version"]), ("JOINT_SET_TCN_BODY", 4, 1))
        self.assertEqual(body["joints"][3]["parent_index"], 2)
        self.assertEqual(body["bones"][5], {"from": "neck", "to": "left_shoulder", "from_index": 3, "to_index": 6,
                                            "rest_dir": [1.0, 0.0, 0.0]})
        coco = next(s for s in doc["joint_sets"] if s["id"] == "COCO17")
        self.assertIsNone(coco["bones"][0]["rest_dir"])

    def test_markdown_mentions(self):
        md = js.render_markdown(self.table)
        for needle in ("tcnart_body_v1", "JOINT_SET_TCN_BODY", "NaN", "UNVERIFIED", "parent-relative",
                       "rest-pose-relative", "left_index1", "joint_sets.yaml"):
            self.assertIn(needle, md)

    def test_cpp_and_csharp(self):
        h = js.render_cpp(self.table)
        self.assertIn("enum class JointSet : std::int32_t", h)
        self.assertIn("Unknown = 0, Coco17 = 1, Halpe26 = 2, TcnLite = 3, TcnBody = 4, Smpl24 = 5, Smplx55 = 6, "
                      "Smplh52 = 7", h)
        self.assertIn("namespace tcn::joint_sets", h)
        cs = js.render_csharp(self.table)
        self.assertIn("namespace Tcn.Schema", cs)
        self.assertIn("public static class JointSets", cs)


class JointSetCliTest(unittest.TestCase):
    def run_cli(self, *args):
        import subprocess
        import sys
        return subprocess.run([sys.executable, "-I", str(REPO / "tools" / "idl_cli.py"), "joint-sets", *args],
                              capture_output=True, text=True)

    def test_write_then_check(self):
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            r = self.run_cli("--table", str(TABLE), "--root", tmp, "--check")
            self.assertEqual(r.returncode, 1)
            for rel in js.OUTPUTS:
                self.assertIn(f"differs: {rel}", r.stdout)
            self.assertFalse(any(Path(tmp).iterdir()), "--check writes nothing")
            self.assertEqual(self.run_cli("--table", str(TABLE), "--root", tmp).returncode, 0)
            r = self.run_cli("--table", str(TABLE), "--root", tmp, "--check")
            self.assertEqual((r.returncode, r.stdout), (0, ""))

    def test_table_errors_fail(self):
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            bad = Path(tmp) / "bad.yaml"
            bad.write_text(TABLE.read_text(encoding="utf-8").replace("ordinal: 7", "ordinal: 9"), encoding="utf-8")
            r = self.run_cli("--table", str(bad), "--root", tmp, "--check")
            self.assertEqual(r.returncode, 1)
            self.assertIn("SMPLH_52", r.stdout)


if __name__ == "__main__":
    unittest.main()
