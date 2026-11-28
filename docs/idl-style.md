# IDL style guide

These rules apply to every IDL file in `src/dds/`. They come from the 0.11 decisions
([docs/proposals/0.11-decisions.md](proposals/0.11-decisions.md): O5, E1, E2, E6, E7). The lint checks the machine-checkable rules:

```
python3 -I tools/idl_cli.py lint --dds-root src/dds --baseline tools/idl/lint_baseline.txt
```

The lint also runs in `ctest` as `idl_lint` when `TCN_SCHEMA_BUILD_MSG_TESTS=ON`.
- `tools/idl/lint_baseline.txt` lists the findings that existed when the lint was introduced.
- The lint fails on any new finding, and also on any baseline entry that no longer occurs. So when you fix a finding,
  remove its line (or regenerate the baseline with `--update-baseline`, and check the diff only removes lines).
- The baseline shrinks as the 0.11 plans land, and is empty at 0.11.0. Since 0.11 plan 5 it **is empty**: the lint is
  clean for every package, `sqe_msgs` included, and the baseline file holds only its header. Do not regrow it
  (`test_lint` fails when a baseline entry names `sqe_msgs`).

Two families of packages follow different rules:
- **tcn** (`src/dds/tcn/`): `tcn_types`, `sqe_msgs`, `tcnart_msgs`, `pcpd_msgs` and `conversation_msgs`. The package
  table `cmake/tcn_packages.json` is the authority.
- **ros2:** the copies under `src/dds/ros2/`.

## Rules checked by the lint

**`basename-unique`**: No file name appears twice in one package, across `msg/`, `rpc/` and `action/`.
- Generated headers are named after the file.
- In 0.10, `tcnart_msgs/msg/SpatialRelations.idl` and `tcnart_msgs/rpc/SpatialRelations.idl` produced two
  `SpatialRelations.h`. The installed `rpc` header was a copy of the `msg` one, so the `rpc` types never shipped
  (the file was deleted in 0.11).

**`include-collision`**: No file includes, directly or transitively, a file with the same basename as a file of its
own `<pkg>/<kind>` directory. fastddsgen 3.0.0 writes the code of every included IDL file into the including run's
output directory, so the included file's header overwrites the other one. `tools/ros2` avoids this for ROS services
and actions by naming such a file `<Name>Service.idl` / `<Name>Action.idl` (for example
`control_msgs/action/GripperCommandAction.idl`). The type names are unchanged.

**`file-pascalcase`** (tcn): File names are PascalCase (`PoseTracking.idl`, not `pose_tracking.idl`).

**`include-guard`**: Every file has an include guard named after its path: `<PKG>_<KIND>_<FILE>_IDL`, upper case.
For example, `tcnart_msgs/msg/PoseTracking.idl` has

```
#ifndef TCNART_MSGS_MSG_POSETRACKING_IDL
#define TCNART_MSGS_MSG_POSETRACKING_IDL
...
#endif
```

fastddsgen preprocesses each file, and a type included twice without a guard is a redefinition.

**`extensibility-explicit`** (E1, E2, E7): Every struct, union and enum states its extensibility.
- **tcn:** `@appendable`, so 0.12 can append fields without a wire break; or `@final` for small leaf structs that will
  never change (`Vector3`, `Quaternion`, `RigidTransform`, `SRPoint2D`).
- **ros2:** always `@final`, so the encoding stays byte-compatible with ROS 2 (XCDR1).
- The generator default is not relied on: an explicit annotation is what a reader and every hand-written codec see.

**`enum-first-unknown`** (tcn, E6): The first enumerator of every enum is `…_UNKNOWN` or `…_NONE`.
- A reader decodes an enumerator it does not know as that value.
- A member missing from an older payload also reads as that value (E8), never as a real value.

**`enum-prefix`** (tcn, O5): Every enumerator starts with the enum's name in UPPER_SNAKE_CASE: `enum StreamKind {
STREAM_KIND_UNKNOWN, STREAM_KIND_VIDEO, … }`. This keeps the generated C++ free of clashes, such as
`ERROR_TIMEOUT` with Windows macros (0.11: `conversation_msgs::msg::ErrorCode` is `ERROR_CODE_*`). The 0.11
renames are listed in [migration/0.11-enum-map.md](migration/0.11-enum-map.md).

**`no-enums-module`**: No `module enums { … }`. An enum lives next to the types that use it.

## Rules not (yet) checked by the lint

- **`@optional` instead of placeholders.** Absence is explicit: use `@optional`, not flag pairs (`has_x` + `x`),
  sentinel values (`-1`, NaN, an identity pose) or "empty means absent". Documented exceptions are
  `tcnart_msgs::msg::HumanShape.bone_lengths` (decisions §7) and `tcnart_msgs::msg::Person.bone_lengths`
  (`PersonTracking.idl`): NaN for a bone whose length is not frozen yet. An element of a fixed-length per-bone
  sequence cannot be `@optional`.

  A second named exception is `pcpd_msgs::rpc::ParameterDescriptor`: it keeps its `has_minimum` + `minimum`,
  `has_maximum` + `maximum` and `has_step` + `step` pairs. They predate 0.11 (0.9.0, when XCDR1 could not round-trip
  an `@optional` member), and no 0.11 decision asks for the change, so 0.11 leaves the parameter protocol's
  descriptor as it is. A false flag means the value is meaningless and must be ignored.
- **New members from 0.11.x on** are `@optional`, unless 0, `""` or empty is a true reading of them (E8).
- **Payload last.** In messages that zero-copy encoders write (`VideoStreamMessage`, point clouds, meshes), the
  payload is the last member, so its offset follows from the members before it.
- **Stream messages start with `tcn_types::msg::StreamHeader header`** (0.11, #38 A1), never `std_msgs/Header`. The
  ten stream messages are `VideoStreamMessage`, `Pose6DMessage`, `TargetTrackingMessage`, `EyeTrackingMessage`,
  `HandTrackingMessage`, `ImuMessage`, `PersonTrackingMessage`, `HumanPoseMessage`, `HumanShapeMessage`
  (`tcnart_msgs`) and `MeshBitstreamMessage` (`pcpd_msgs`); a new stream message joins them.
  `TcnartPresenceMessage` is not a stream message and keeps `std_msgs/Header`. `tools/idl/tests/test_stream_header.py`
  checks the list.
- **A stream kind's descriptor part is a `*StreamInfo` case** of the `StreamInfo` union in
  `tcnart_msgs/msg/StreamDescriptor.idl` (`VideoStreamInfo`, `TrackerStreamInfo`, `PersonStreamInfo`, …), switched on
  `StreamKind`. A new kind appends a `StreamKind` enumerator and a case; a new field of a kind is a trailing
  `@optional` member of its `*StreamInfo`. `StreamDescriptorMessage.properties` holds vendor extensions only, never
  something a typed field can say.
- **The joint-set table is the only source.** Joints, bones, parents and rest poses live in
  `tcnart_msgs/msg/joint_sets.yaml`; `JointSet.idl`, `JointSets.md` and the C++, C# and JSON arrays are generated from
  it (`python3 -I tools/idl_cli.py joint-sets --table src/dds/tcn/tcnart_msgs/msg/joint_sets.yaml`). Never edit a
  generated file or copy a row into an IDL comment; `joint-sets --check` and the drift test in
  `tools/idl/tests/test_joint_sets.py` (ctest `idl_tools`) fail on drift.
- **Every top-level message documents its key or topic and its encoding** in a comment above it.
- **ROS 2 types are never edited by hand.** `src/dds/ros2/` is generated from ROS 2 Jazzy by
  `python3 -I tools/ros2_cli.py refresh` (see [src/dds/ros2/UPSTREAM.md](../src/dds/ros2/UPSTREAM.md)). The generated
  files carry their include guards and `@final`, and fields named like IDL keywords are written as escaped
  identifiers (`_map` is the field `map`). What ROS does not cover belongs in a tcn type, which may embed ROS types.
