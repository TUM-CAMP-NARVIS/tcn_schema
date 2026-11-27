import unittest

from tools.ros2.rosmsg import RosType, parse_action, parse_message, parse_service


class RosMsgTest(unittest.TestCase):
    def test_primitives_and_arrays(self):
        m = parse_message("p", "M", "float64[36] covariance\nuint8[] data\nstring<=8 s\nint32[<=4] b\n")
        f = {x.name: x.type for x in m.fields}
        self.assertEqual(f["covariance"].array, ("fixed", 36))
        self.assertEqual(f["data"].array, ("unbounded",))
        self.assertEqual((f["s"].name, f["s"].string_bound), ("string", 8))
        self.assertEqual(f["b"].array, ("bounded", 4))
        self.assertIsNone(f["covariance"].pkg)

    def test_bounded_string_array(self):
        m = parse_message("p", "M", "string<=5[<=3] names\n")
        t = m.fields[0].type
        self.assertEqual((t.name, t.string_bound, t.array), ("string", 5, ("bounded", 3)))

    def test_constants(self):
        m = parse_message("p", "M", 'uint8 SPHERE=1\nstring NAME="x"\n')
        self.assertEqual([(c.name, c.type, c.value) for c in m.constants],
                         [("SPHERE", "uint8", "1"), ("NAME", "string", '"x"')])
        self.assertEqual(m.fields, [])

    def test_defaults(self):
        m = parse_message("p", "M", "float64 w 1.0\nint32[] v [1, 2]\n")
        self.assertEqual([f.default for f in m.fields], ["1.0", "[1, 2]"])

    def test_resolution(self):
        m = parse_message("geometry_msgs", "PoseStamped", "std_msgs/Header header\nPose pose\n")
        self.assertEqual(m.fields[0].type, RosType("std_msgs", "Header"))
        self.assertEqual(m.fields[1].type, RosType("geometry_msgs", "Pose"))

    def test_comments(self):
        m = parse_message("p", "M", "# file comment\n\nint8 X=1 # one\n  # indented\nint32 a # field\n")
        self.assertEqual(m.constants[0].value, "1")
        self.assertEqual([f.name for f in m.fields], ["a"])

    def test_service_split(self):
        s = parse_service("p", "Add", "int64 a\n---\nint64 sum\n")
        self.assertEqual(([f.name for f in s.request.fields], [f.name for f in s.response.fields]), (["a"], ["sum"]))
        self.assertEqual((s.request.name, s.response.name), ("Add_Request", "Add_Response"))

    def test_action_split(self):
        a = parse_action("p", "Do", "int32 g\n---\nint32 r\n---\nint32 f\n")
        self.assertEqual([[f.name for f in x.fields] for x in (a.goal, a.result, a.feedback)], [["g"], ["r"], ["f"]])
        self.assertEqual(a.goal.name, "Do_Goal")


if __name__ == "__main__":
    unittest.main()
