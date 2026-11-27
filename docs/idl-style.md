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
- The baseline shrinks as the 0.11 plans land, and is empty at 0.11.0.

Two families of packages follow different rules:
- **tcn:** `tcnart_msgs`, `pcpd_msgs`, `conversation_msgs`, and from 0.11 `tcn_types` and `sqe_msgs`.
- **ros2:** the copies under `src/dds/ros2/`.

## Rules checked by the lint

**`basename-unique`**: No file name appears twice in one package, across `msg/`, `rpc/` and `action/`.
- Generated headers are named after the file.
- In 0.10, `tcnart_msgs/msg/SpatialRelations.idl` and `tcnart_msgs/rpc/SpatialRelations.idl` produced two
  `SpatialRelations.h`. The installed `rpc` header was a copy of the `msg` one, so the `rpc` types never shipped.

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

**`enum-prefix`** (tcn, O5): Every enumerator starts with the enum's name in UPPER_SNAKE_CASE: `enum SensorType {
SENSOR_TYPE_NONE, SENSOR_TYPE_VIDEO_COLOR, … }`. This keeps the generated C++ free of clashes, such as
`ERROR_TIMEOUT` with Windows macros.

**`no-enums-module`**: No `module enums { … }`. An enum lives next to the types that use it.

## Rules not (yet) checked by the lint

- **`@optional` instead of placeholders.** Absence is explicit: use `@optional`, not flag pairs (`has_x` + `x`),
  sentinel values (`-1`, NaN, an identity pose) or "empty means absent". A documented exception is
  `HumanShape.bone_lengths` (decisions §7).
- **New members from 0.11.x on** are `@optional`, unless 0, `""` or empty is a true reading of them (E8).
- **Payload last.** In messages that zero-copy encoders write (`VideoStreamMessage`, point clouds, meshes), the
  payload is the last member, so its offset follows from the members before it.
- **Every top-level message documents its key or topic and its encoding** in a comment above it.
- **ROS 2 types are never edited by hand.** They are regenerated from upstream (from 0.11, by `tools/ros2/refresh.py`).
  What ROS does not cover belongs in a tcn type, which may embed ROS types.
