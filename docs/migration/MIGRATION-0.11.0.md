# Migration to tcn_schema 0.11.0 (running log)

**Status: in preparation.** This is the log of every 0.11 change that a consumer has to follow. It is updated with each
0.11 pull request, so it is always current on `main`. When `0.11.0` is released, it becomes that release's migration
note.

- **What was decided and why:** [docs/proposals/0.11-decisions.md](../proposals/0.11-decisions.md). Where this log and
  the decisions differ, the decisions win; please report the difference.
- **Order of work:** six plans (Plan 4 was split, see its section), each one or more PRs (decisions §10). Plan 1's plan file is
  [docs/superpowers/plans/2026-11-27-0.11-plan-1-layout-and-tooling.md](../superpowers/plans/2026-11-27-0.11-plan-1-layout-and-tooling.md).

| Plan | Content | Wire change | Status |
|---|---|---|---|
| 1 | Layout and tooling: docs, out-of-tree generation, package table, `types.json`/`enums.json`, lint, round-trip tests | **none** | merged (#41) |
| 2 | ROS 2 tree regenerated from Jazzy, `@final`, service/action types, `ros2_types.json`, tiers | ROS types only (`Range.variance`) | merged (#42) |
| 3 | Moves and deletions: `sqe_msgs`, `tcn_types`, parameter RPC → `pcpd_msgs`, `custom/` → `tcn/`, deleted types | type names, removals | merged (#43) |
| 4 | Encoding (XCDR2, `tcn_schema=0.11`), enum renames outside `sqe_msgs`, field removals F1, F2, F5 | **yes** | in review (#44) |
| 5 | The #35 SQE redesign, with F3/F4 and the `sqe_msgs` enums | **yes** | in review (branch `0.11-plan5-sqe`) |
| 6 | #36, #37, #38 changes, HumanPose and joint sets; `0.11.0-rc1` | **yes** | planned |

**How to use this log.** Find your section in §2. Each item says which plan brought it, what changes for you and what
to do. Items marked *(wire)* need the lockstep cut-over; all others can be done ahead of it.

## 1. Changes so far, for everyone

### Plan 1: layout and tooling (no wire change)

- **Documentation moved.**
  - The 0.11 proposals and the decisions are in `docs/proposals/`.
  - Design notes are in `docs/design/`. The contract-library design is now
    `docs/design/2026-08-31-sqe-contract-library-design.md`.
  - Migration notes are in `docs/migration/`, with an index in the root [MIGRATION.md](../../MIGRATION.md). Links to
    `MIGRATION-x.y.z.md` in the repository root no longer resolve; use `docs/migration/MIGRATION-x.y.z.md`.
- **Generated code is no longer written into the source tree.**
  - fastddsgen now writes to `<build>/generated/<pkg>/<msg|rpc|action>/` instead of `<repo>/include/`.
  - **The installed package is unchanged:** headers stay at `include/<pkg>/<kind>/<File>.h`, so conan consumers
    change nothing.
  - **Only a build that uses the source tree directly** (for example `add_subdirectory(tcn_schema)`, or include paths
    into a tcn_schema checkout) must use the `tcn_schema_<pkg>` targets, which carry the right include directory,
    instead of `<repo>/include`.
  - Delete a stale `<repo>/include/` left from older builds. It stays in `.gitignore` until 0.12, so it cannot be
    committed by accident.
- **`tf2_msgs` LookupTransform moved from `msg/` to `action/`:** `include/tf2_msgs/action/LookupTransform.h` and
  `share/tcn_schema/dds/tf2_msgs/action/LookupTransform.idl`. This fixes the install path of an action type. None of
  the five consumers uses it.
- **New: `share/tcn_schema/types.json` and `share/tcn_schema/enums.json`** (decisions O7).
  - They list every IDL declaration with its fully qualified name, package, family (`ros2`/`tcn`), file,
    extensibility, members (type, `@optional`, array dims), union discriminator and cases, and enumerator ordinals.
  - They are generated from the IDL at build time by `tools/idl_cli.py tables`.
  - **Every team that keeps a hand-written mirror of types or enum ordinals** should start generating it from these
    files, or checking it against them in CI:
    - the engine's `messages.rs` registry;
    - the web controller's `packages/sqe/src/schemas.ts`;
    - IVA's `tcn-wire`;
    - tcnart's Unreal ordinals;
    - HTA's FlatBuffers mirrors.
  - The format is stable from 0.11.0 on. Fields are only added, never removed.
- **Conan components follow the IDL includes.** One table, `cmake/tcn_packages.json`, drives CMake and conan, and is
  installed as `share/tcn_schema/packages.json`. Each component now requires exactly the packages its IDL includes.
  - **Requirements that were dropped,** and that are no longer reachable through another requirement:
    - `geometry_msgs` from `tcnart_msgs`, `pcpd_msgs`, `device_hl2_msgs` and `device_orbbec_msgs`;
    - `std_msgs` from `rosgraph_msgs`.

    **A consumer that requires one of these components and uses types of the dropped package must now require that
    package's component itself** (for example `tcn_schema::geometry_msgs`). None of the five consumers is affected:
    HTA links `tcn_schema::tcn_schema` (everything), and only tcnart's deprecated v2 names components, with
    `geometry_msgs` listed explicitly.
  - `shape_msgs` and `tf2_msgs` no longer require `std_msgs` directly. They still get it through `geometry_msgs`.
  - Requirements were added where the IDL needs them: `builtin_interfaces` for `sensor_msgs`, `tf2_msgs`,
    `statistics_msgs` and `tcnart_msgs`.
  - The CMake config files exported per library (`share/tcn_schema_<pkg>/cmake`) follow the same table.
- **fast-cdr headers are now visible to consumers** (`transitive_headers=True`).
  - Every generated header includes fastcdr headers. Before, a Conan 2 consumer had to require `fast-cdr` itself, as
    HTA does; that is no longer needed, though keeping it does no harm.
- **tcnart:** the shipped `share/tcn_schema/dds/tf2_msgs/action/LookupTransform.idl` now matches the layout of your
  snapshot `v3/third_party/tcn_schema_idl` (`idlgen.py check-package`).
- **New: an IDL style guide and lint** ([docs/idl-style.md](../idl-style.md)): `python3 -I tools/idl_cli.py lint`.
  Today's violations are in a baseline that shrinks with every plan and is empty at 0.11.0. The rules show what 0.11
  IDL looks like:
  - explicit `@appendable`/`@final`;
  - every enum starts with `…_UNKNOWN`/`…_NONE`;
  - enumerators prefixed with the enum name;
  - include guards;
  - PascalCase file names, unique within a package.
- **New: golden byte files** in [test/golden/](../../test/golden/), as space-separated hex. They are the shared
  contract for every hand-written codec (Rust, TypeScript, C#, Python):
  - `std_msgs_Header`;
  - `tcnart_msgs_TargetTrackingMessage`;
  - `sqe_msgs_SRGraph` (named `tcnart_msgs_SRGraph` until Plan 3).

  The values that produced them are in `test/golden/golden_test.cpp`. Since Plan 4 the tcn goldens are XCDR2 and
  there are evolution goldens (see Plan 4). Point your codec tests at these files.
- **New: round-trip tests of every struct and union** (`ctest`: `roundtrip`), generated from `types.json`.
  `tcnart_msgs/rpc/SpatialRelations.idl` is skipped, because its types never existed in C++ (the header collision;
  the file is deleted in Plan 3).
- **Known generator defect, with a workaround for C++ users: `operator==` of the parameter types compares
  pointers.** fastddsgen 3.0.0 stores a member whose type is a *forward-declared union* as a raw pointer. In
  `tcnart_msgs/rpc/Requests.idl` that is `ParameterValue`, so `Parameter::value` and `ParameterDescriptor::minimum`,
  `maximum`, `step` and `default_value` are pointers. The generated `operator==` compares those pointers, and the
  error spreads to every type that contains such a member, directly or through a sequence or union case.
  - **`a == b` can be `false` for equal values** in these 14 types:
    - `tcnart_msgs::rpc::`: `Parameter`, `ParameterValue`, `ParameterList`, `ParameterChanged`,
      `GenericParameterRequest`, `GenericParameterReply`, `ParameterDescriptor`, `ParameterDescriptorList`,
      `ParameterDescriptorReply`, `ParameterSchema`, `ParameterListSchema`, `ParameterSchemaReply`;
    - `pcpd_msgs::rpc::`: `ComponentInfoReply`, `ServiceInfoReply`.
  - **What to do:** compare fields, or the encoded bytes, instead of using `==` on these types.
  - The wire is not affected.
  - `types.json` lists the pointer members per type (`pointer_members`) and marks the 14 types with
    `"eq_by_pointer": true`. `"forward_declared": true` only says that a type is forward-declared in its file.
  - **HTA's and tcnart's parameter services** (and code that compares `ServiceInfoReply` / `ComponentInfoReply`)
    should check that they never use `==` on these types. IVA's Rust mirror is not affected.
- **Building tcn_schema:**
  - it needs Python 3 (standard library only) at build time;
  - it builds in parallel again (the forced serial build is gone);
  - `ctest` works from the build root when `TCN_SCHEMA_BUILD_MSG_TESTS` or `TCN_SCHEMA_BUILD_SQE_TESTS` is on.
  - Changing an IDL file now regenerates every package that includes it, directly or transitively. Before, only the
    file's own package was regenerated, and incremental builds could compile stale code.
  - `conan editable add` works: `package_info()` falls back to the source table.

### Plan 2: ROS 2 interfaces from Jazzy (ROS types only; no tcn type changes on the wire)

- **The ROS 2 tree is generated from ROS 2 Jazzy** by `tools/ros2` and is never edited by hand. Details, tiers and
  how to refresh: [src/dds/ros2/UPSTREAM.md](../../src/dds/ros2/UPSTREAM.md).
  - The packages are pinned to their Jazzy release tags (`src/dds/ros2/UPSTREAM.lock`).
  - Every ROS struct is `@final`, so it stays XCDR1 (`00 01`), byte-compatible with ROS 2.
- **Wire changes of existing ROS types:**
  - `sensor_msgs/msg/Range` gains `float variance` at the end;
  - `sensor_msgs/msg/NavSatStatus` gains the constants `STATUS_UNKNOWN = -2` and `SERVICE_UNKNOWN = 0`, and
    `status` now defaults to `-2` (unknown) instead of `0` (`STATUS_FIX`).

  Everything else that existed keeps its layout, checked type by type. tcn types are unchanged on the wire: the
  golden files did not change.
- **Renamed files and headers** (upstream names, R5). Update `#include`s:
  - `builtin_interfaces/msg/Time_.h` → `builtin_interfaces/msg/Time.h`;
  - `Duration_.h` → `Duration.h`;
  - `std_msgs/msg/String_.h` → `std_msgs/msg/String.h`.

  The same applies to the IDL paths under `share/tcn_schema/dds/`.
- **Fixed-size arrays** are written as member arrays (`double covariance[36]`) instead of typedefs. The C++ type is
  still `std::array<double, 36>` and the wire is identical, but the typedef names (`geometry_msgs::msg::double__36`,
  `sensor_msgs::msg::double__9`, `unique_identifier_msgs::msg::uint8__16`, …) are gone. Use `std::array` directly.
- **New: 29 ROS packages in tiers.**
  - `base` and `core` are built by default.
  - `manipulation` (control_msgs, controller_manager_msgs, moveit_msgs, object_recognition_msgs, octomap_msgs),
    `navigation` (nav2_msgs, geographic_msgs) and `perception` (vision_msgs, pcl_msgs) are built with
    `-DTCN_SCHEMA_ROS2_TIERS=…` or conan `-o tcn_schema/*:ros2_tiers="base;core;manipulation"`.
  - `share/tcn_schema/packages.json` records which components were built.
  - `service_msgs` is in `base` (decisions §5 had it in `core`), because `sensor_msgs` has a service.
- **New: complete service and action types**, as rosidl defines them: `Foo_Request/_Response/_Event`; for actions
  `_Goal`, `_Result`, `_Feedback`, `_SendGoal_*`, `_GetResult_*`, `_FeedbackMessage`.
  - Two `control_msgs` actions are in `GripperCommandAction.idl` and `JointTrajectoryAction.idl`, so their headers
    are `control_msgs/action/GripperCommandAction.h` and `JointTrajectoryAction.h`. Otherwise fastddsgen would
    overwrite them with the same-named message headers. The type names are unchanged.
  - Fields named like IDL keywords (`map`, `object`, `component`) are escaped in the IDL (`_map`). The C++ accessor
    is still `map()`.
- **New: `share/tcn_schema/ros2_types.json`.** For every ROS type it gives the ROS name, the DDS name
  (`std_msgs::msg::dds_::String_`), the C++ name, and the RIHS01 type hash for Jazzy, Kilted and Lyrical. This is
  what a future rmw_zenoh adapter needs. The format is in `src/dds/ros2/UPSTREAM.md`.
- **Generated size constants:** the `…_max_cdr_typesize` values of tcn types that embed `std_msgs/Header` are 8 bytes
  smaller, because `Header` and `Time` are `@final`.
  - fastddsgen 3.0.0 computes sizes above 2³² for a few large ROS types (visualization_msgs `InteractiveMarker*`,
    some moveit_msgs types). Those constants are set to 4294967288, the largest value the generated Fast DDS type
    support can add its header to without overflowing.
  - With `with_dds`, keep the default payload pool policy (`PREALLOCATED_WITH_REALLOC`) for these types.
- **`types.json`: escaped identifiers are listed by their real name.** `conversation_msgs::msg::HealthStatus`'s member
  appears as `component`, not `_component`, as IDL 4.2 defines (the C++ accessor was already `component()`). No
  consumer reads it today.
- **ROS 2 tier selection is checked.** An unknown tier name, a selection without `base`, or a tier without the tiers
  it needs (`manipulation` needs `core`) fails at configure time with a message saying what to add.
- **Building tcn_schema:** parallel generation is now race-free, because each fastddsgen run has its own temp
  directory. Before, concurrent runs could corrupt each other's preprocessed includes.

### Plan 3: moves and deletions (no field changes; type names and packages change)

The complete old → new map, with header paths: **[0.11-type-map.md](0.11-type-map.md)**. It is also installed,
machine-readable, as `share/tcn_schema/type_map_0_11.json` (`moved: [{old, new, old_header, new_header}]`,
`deleted: [...]`); use it to rewrite code and mirrors mechanically. `tools/idl_cli.py moves` checked that every type
that moved has exactly the same fields, enumerators and union cases.

- **New packages and conan components:**
  - `tcn_types`: shared types.
    - `msg/Geometry.idl`: `Vector2`, `Vector3`, `Quaternion`, `RigidTransform`, `Ray`, `ViewPose`, `CoordinateAxis`,
      `CoordinateSystem`;
    - `msg/Ids.idl`: `UUID`;
    - `msg/Tracking.idl`: `MarkerType`, `Marker`, `Target`.
  - `sqe_msgs`: the SQE protocol.
    - `msg/Graph.idl`: `SRNode`, `SREdge`, `SRGraph`, … ;
    - `msg/Transforms.idl`: `SRTransform`, `SRIntrinsics`, … ;
    - `rpc/{Session,Mutations,Streams,Queries,Targets,Operator,Sample}.idl`: the former `SIS.idl`, split by topic.
  - Link `tcn_schema::sqe_msgs` and `tcn_schema::tcn_types` where you use them. `tcn_schema::sqe_contract` now
    requires `sqe_msgs`, `tcnart_msgs` and `tcn_types`.
- **Moved:**
  - `tcnart_msgs::msg::SR*` → `sqe_msgs::msg::SR*`;
  - `tcnart_msgs::rpc::SIS*` (and `SR*` request types) → `sqe_msgs::rpc::*`;
  - `tcnart_msgs::msg::{Vector3, Quaternion, RigidTransform, UUID, Target, Marker, MarkerType, …}` →
    `tcn_types::msg::*`;
  - the parameter RPC (`tcnart_msgs/rpc/Requests.idl`) → `pcpd_msgs/rpc/Parameters.idl`, namespace
    `pcpd_msgs::rpc` (`Parameter`, `ParameterValue`, `GenericParameterRequest/Reply`, `ParameterDescriptor*`,
    `ParameterSet*`, `ParameterChanged`, `RPCResponseStatus`, `NullRequest/Reply`, `BoolReply`, `StringList*`, …);
  - the media enums (`SensorType`, `VideoPixelFormat`, `VideoImageCompression`, `VideoH26xProfile`,
    `AudioAACProfile`) → `tcnart_msgs/msg/MediaTypes.idl`; `Handedness` → `HandTracking.idl`; `XRGazeSpaceType` →
    `EyeTracking.idl` (all still `tcnart_msgs::msg`);
  - `conversation_msgs` files are PascalCase (`msg/Common.idl`, `Control`, `Audio`, `Stt`, `Llm`, `Tts`, `Metrics`,
    `rpc/Discovery.idl`), so the headers are `conversation_msgs/msg/Common.h`, … . The namespaces are unchanged.
  - `tcnart_msgs` has no `rpc/` any more.
- **Headers:** `tcnart_msgs/msg/Types.h`, `tcnart_msgs/msg/SpatialRelations.h`, `tcnart_msgs/rpc/SIS.h` and
  `tcnart_msgs/rpc/Requests.h` are gone. Include the new ones from the type map: its "Moved" table for renamed
  types, and "Same name, new header" for types that kept their name (the media enums, `Handedness`,
  `XRGazeSpaceType`, and every `conversation_msgs` type). Most code includes a message
  header (for example `tcnart_msgs/msg/PoseTracking.h`), which pulls in the new ones itself.
- **Wire type names follow the namespaces:** `application/cdr;sqe_msgs::msg::SRGraph`,
  `pcpd_msgs::rpc::GenericParameterRequest`, … (the contract library's `WireType<T>` already does this). The
  `tcn_schema=` annotation stays `0.10` until Plan 4. **Between this plan and rc1, a build from `main` cannot talk
  to 0.10 peers about moved types.** That is expected, because consumers switch at rc1.
- **Deleted** (decisions §4; listed in the type map):
  - the `device_hl2_msgs` and `device_orbbec_msgs` packages and conan components;
  - `GenericManager`, the legacy `rpc/SpatialRelations` types, `Logging`, `AudioStream`, `XRInput`, `XRRuntime`,
    `Instance`;
  - `Pose6DMapMessage` and `RigidTransformMap(Item)`;
  - `CameraInfoMessage`;
  - the scalar request/reply types (`StringRequest`, `Int32Reply`, … ; `BoolReply` stays);
  - `ParameterSchemaReply`;
  - the pcpd `ServiceType`, `RecorderStatus`, `BufferInfoReply` and `ComponentInfoReply`;
  - the unused math types (`Vector4`, `Matrix*`, `Plane`, `MSGuid`, `BoundingBox`, `Frustum`, `OrientedBox`,
    `Sphere`);
  - the XR and audio enums only the deleted types used.
- **Source tree:** `src/dds/custom/` is now `src/dds/tcn/`, and the IDL install path is
  `share/tcn_schema/dds/<pkg>/…` as before. Every tcn file now has an include guard.

### Plan 4: encoding, enum names, field removals (wire change)

**The roadmap was split here.** The decided "Plan 4" is now three steps, so the SQE types change once, not twice:
Plan 4 (this one) does the encoding for every package and the cleanup outside the SQE; Plan 5 is the #35 SQE redesign
with its field removals (F3, F4) and enum changes; Plan 6 is #36–#38 and rc1.

**Encoding (decisions E1–E9).** Every hand-written codec has to implement this. fast-cdr users set two things and
add one check (the enum range check below).

| | tcn types (`tcn_types`, `sqe_msgs`, `tcnart_msgs`, `pcpd_msgs`, `conversation_msgs`) | ROS 2 types (top level) |
|---|---|---|
| Encoding | XCDR2, `D_CDR2` | XCDR1, `CDR` |
| Encapsulation (LE) | `00 09 00 00` | `00 01 00 00` |
| fast-cdr writer | `Cdr(buf, LITTLE_ENDIANNESS, CdrVersion::XCDRv2)`, then `set_encoding_flag(EncodingAlgorithmFlag::DELIMIT_CDR2)`, then `serialize_encapsulation()` | `CdrVersion::DDS_CDR` |
| fast-cdr reader | pick the version from byte 1 of the encapsulation (`>= 0x06`: `XCDRv2`, else `DDS_CDR`), then `read_encapsulation()` | same |

- **Pitfall: without `set_encoding_flag`** an `XCDRv2` writer emits `00 07` (`PLAIN_CDR2`) and no DHEADERs.
- **The XCDR2 rules a codec needs:**
  - **DHEADER** (uint32, byte length of what follows) before every `@appendable` struct and union, and before
    every sequence or array whose element type is not primitive (strings count as non-primitive; enums count as
    primitive);
  - **no DHEADER** for `@final` types: `tcn_types::msg::{Vector2, Vector3, Quaternion, RigidTransform}`,
    `sqe_msgs::msg::SRPoint2D`, and every ROS type, including a `std_msgs/Header` nested in a tcn message;
  - **maximum alignment 4:** `int64`, `uint64` and `double` align to 4, not 8. This also applies inside a nested ROS
    `Header`, because the whole payload is XCDR2;
  - **`@optional`:** one presence byte (0 or 1), then the value if present;
  - **enums:** int32.
- **Reading (E3, E5, E6, E8):**
  - switch on the encapsulation id;
  - decode into a fresh object;
  - when a struct's DHEADER says more bytes follow than you know members for, skip the rest (a newer writer);
  - when it says fewer, the missing members take their defaults (`0`, `""`, empty, the first enumerator, absent).
  - An unknown enumerator decodes as the enum's first enumerator (`…_UNKNOWN`/`…_NONE`). **fast-cdr does not do this:**
    it casts the int32 to the enum without a range check, so a C++ reader (tcnart, HTA, sqe_client_cpp) holding a
    value appended in 0.11.x has an out-of-range enum. Range-check every enum you `switch` on or index with (the
    enumerators are in `share/tcn_schema/enums.json`) and treat an unknown value as the first enumerator.
- **Annotation (V1):** `application/cdr;<type>;tcn_schema=0.11`. A reader refuses any other major.minor; a 0.10
  payload is refused, not misread. The contract library does this already.
- **E4:** `conversation_msgs` payloads carry the encapsulation header like every other tcn type.
- **Golden files to test a codec against:** `test/golden/*.hex`, with the values in `test/golden/golden_test.cpp`.
  - `std_msgs_Header`: XCDR1;
  - `tcnart_msgs_TargetTrackingMessage`, `sqe_msgs_SRGraph`: XCDR2 with nested `@final` and appendable types;
  - **`evo_v1_Thing` / `evo_v2_Thing`:** one appendable type in two versions (`test/idl/evo/`). V2 appends
    `@optional double c` and `int64 d`. A codec must:
    - decode `evo_v2_Thing` with the V1 layout (skip by DHEADER);
    - decode `evo_v1_Thing` with the V2 layout (`c` absent, `d` = 0).
  - **`evo_v1_Holder` / `evo_v2_Holder`:** `Holder { sequence<Thing> things; int32 tail; }`, two elements. This is
    the test that catches a codec which ignores DHEADERs: decoded with the V1 layout, `evo_v2_Holder` must give the
    second element (`a=8, b="yo"`) and `tail = -1`, and consume every byte. `evo_v1_Holder` with the V2 layout gives
    `c` absent and `d = 0` in both elements and `tail = 42`.

**Enum renames (E6, O5) outside `sqe_msgs`:**
- **The complete old → new map, with values:** **[0.11-enum-map.md](0.11-enum-map.md)** (source:
  `tools/idl/enum_renames_0_11.json`).
- **Every enumerator now starts with its enum's name** (`PixelFormat_NV12` → `VIDEO_PIXEL_FORMAT_NV12`,
  `PVT_STRING` → `PARAMETER_VALUE_TYPE_STRING`, `ERROR_TIMEOUT` → `ERROR_CODE_TIMEOUT`, …).
- **Every enum starts with `…_UNKNOWN` or `…_NONE`.**
- **Values changed where an `…_UNKNOWN` was inserted, or (`VideoH26xProfile`) where `NONE` moved first.** Code that
  writes or compares numbers (TypeScript, Rust, C#, JSON mirrors, databases) must take the new values from the map.
  Unchanged values: `CameraModelType`, `XRGazeSpaceType` and `conversation_msgs::msg::ErrorCode` (renamed only).
- **The default of every enum field with an inserted `…_UNKNOWN` changed.** An unset field now reads `…_UNKNOWN`,
  not the old first enumerator: an unset `RPCResponseStatus` was `SUCCESS` and is now `UNKNOWN`; likewise
  `ParameterAccess` (was read-write-live), `ParameterSetStatus` (applied), `ParameterValueType` (string),
  `VideoPixelFormat` (NV12), `VideoImageCompression` (raw), `CoordinateAxis` (+X), and the five `conversation_msgs`
  enums (`RetrievalPolicy` was auto, `InteractionMode` direct, …). **A producer that relied on the default must now
  set the field explicitly.** This matters for `DeviceContextReply.status`, which readers now gate on instead of
  `is_valid`.
- `sqe_msgs` enums change in Plan 5.

**Fields removed (decisions §4: nobody writes a meaningful value).** Every codec and every hand-written layout
mirror (Rust, TypeScript, C#, FlatBuffers) must drop the member, whether or not anything reads it; a mirror that keeps
it misdecodes every member after it. Code that writes or reads a removed member must go too; the per-consumer notes
below list the sites found by grep.

| Type | Removed |
|---|---|
| `tcnart_msgs::msg::StreamDescriptorMessage` | `calib_topic_schema`, `pose`, `audio_channels`, `aac_profile` (and the enum `AudioAACProfile`) |
| `tcnart_msgs::msg::BufferInfo` | `is_valid`, `is_fixed_size` |
| `pcpd_msgs::msg::MeshBitstreamMessage` | `origin_offset`, `is_compressed` |
| `pcpd_msgs::msg::CameraSensor` | `camera_type` (and `CameraSensorTypesEnum`), `raw_calibration`, `timestamp_offset_ns` |
| `pcpd_msgs::rpc::DeviceContextReply` | `timestamp_offset`, `is_valid` (gate on `status` instead) |
| `tcnart_msgs::msg::TargetTrackingMessage` | `target_origin` |
| `tcn_types::msg::Target` | `quality`, `markers` (and the struct `Marker`; `MarkerType` stays) |
| `tcnart_msgs::msg::HandState` | `tracked`, `aim_valid`, `aim` (an untracked hand is left out) |
| `tcnart_msgs::msg::HandJointPose` | `accuracy` (and `JointPoseAccuracy`) |
| `tcnart_msgs::msg::Person` | `role`, `role_confidence`, `has_root_orientation`, `root_orientation`, `joint_rotations`, `joint_rotation_confidence` |
| `conversation_msgs::msg::AudioOpusFrame` | `packet_loss_concealment_applied` |
| `conversation_msgs::msg::AudioPcmChunk`, `TtsAudioChunk` | `payload_kind`, `shm_ref` (and `PayloadKind`, `ShmRef`) |
| `conversation_msgs::msg::TtsRequest` | `backend`, `voice`, `language`, `speaking_rate`, `pitch`, `volume_gain_db`, `stream_output` |
| `conversation_msgs::msg::LlmRequest` | `system_prompt`, `low_latency_mode` |
| `conversation_msgs::msg::RetrievalOptions` | `preferred_domains`, `excluded_domains` |
| `conversation_msgs::msg::SttPartial` | `stability` |
| `conversation_msgs::msg::StageMetric` | `gpu_utilization_pct` |

- **Made `@optional` (decisions §4):** `EyeTrackingMessage.gaze_space` and `scene_point`.
  - An absent gaze space means LOCAL (the header's frame).
  - `scene_point` is present only when measured, and `EYE_VALID_SCENE_POINT` is set exactly then.
  - tcnart's live producer leaves both absent; HTA's Tobii recordings fill them.
- **Not in this plan:** `PersonTrackingMessage.{up, world_frame}` move into `StreamHeader` with #38 (Plan 6).
- **Tools:**
  - `tools/idl_cli.py annotate`, `enums` and `fields` applied these changes; their inputs are the record.
  - `tools/idl_cli.py moves --spec` with an `expect` block proved that nothing else changed.
- **Packaging:** `cmake --install` / conan package works again for enum-only IDL files; broken since plan 3 (#43).

### Plan 5: the SQE wire redesign, #35 (wire change)

Every type and key of the spatial query engine protocol (`sqe_msgs`, the shared `tcn_types`, the contract library
`tcn::sqe`) changes once, here. Read it with the proposal
[docs/proposals/2026-11-27-0.11-sqe-wire-cleanup-proposal.md](../proposals/2026-11-27-0.11-sqe-wire-cleanup-proposal.md)
(the ids N1, R5, H3 … below are its ids).

**The cut.** What is in this plan and what is not:

| In Plan 5 | Left for Plan 6 |
|---|---|
| Everything in the tables below, including **H3** (the data topic carries `SISRelationOutput`, no attachment) and the marker announcement by id (H5) | **S1** (the stream descriptor is split into a header plus a union per sensor kind) and **N6** (`marker_domain` on the descriptor) |
| `SRMaterialization.marker_domain` and the graph filter `SISGraphFilter.marker_domain` (the graph side of N6) | `StreamDescriptorMessage` is **unchanged** in this plan: `marker_system` is still one string and there is no `marker_domain` field on it |

**Maps, generated by `tools/idl_cli.py` (every row is checked against the IDL):**

| Map | Content | Source |
|---|---|---|
| **[0.11-sqe-type-map.md](0.11-sqe-type-map.md)**, installed as `share/tcn_schema/type_map_0_11_sqe.json` (`moved`, `relocated`, `reshaped`, `deleted`) | Types that moved again, moved header, were reshaped (46 `sqe_msgs` types plus `Target` and `Pose6DMessage`) or were deleted (13) | `tools/idl/sqe_0_11.json` |
| **[0.11-sqe-enum-map.md](0.11-sqe-enum-map.md)** | Every enumerator of the 20 `sqe_msgs` enums with its 0.10 and 0.11 name and value | `tools/idl/enum_renames_0_11_sqe.json` |

- **Apply the plan 3 map first and then the plan 5 map** ([0.11-type-map.md](0.11-type-map.md),
  `type_map_0_11.json`). The plan 3 map is the historical record and is stale for what plan 5 moved or deleted:
  it still says `SRNodeScope` → `sqe_msgs::msg` and `SISTargetKind` → `sqe_msgs::rpc`, and lists as moved some types
  that no longer exist (`SRIntrinsics`, `SISFrameRef`, …). The plan 5 map wins.
- **In the sqe enum map, `SRNodeScope` and `SISTargetKind` appear under their pre-move `sqe_msgs` names** (they
  moved to `tcn_types::msg` in the same plan). The "0.11" columns are the new names and values.
- **Install note.** `cmake --install` of the whole tree failed on the plan 4 base because the generator was told to
  write `…CdrAux` files for enum-only IDL (`tcnart_msgs/msg/MediaTypes.idl`). That is fixed on the plan 4 branch after
  this branch was cut (`5ff0fd7`, with an `install_smoke` test); take it from there.

#### Types: what is new, moved, reshaped and deleted

| | Types |
|---|---|
| **New, `tcn_types::msg`** | `SRFrameRef`, `SRFrameSelector` (frames, `Frames.idl`); `ClockDomain`, `ClockKind` (`Clock.idl`); `SRCovariance6`, `SRCovariance3` (`Geometry.idl`); `PinholeIntrinsics`, `CameraIntrinsics` (union), `CameraIntrinsicsKind`, `SRDistortionModel` (`Camera.idl`); `SISTargetInfo` (`Tracking.idl`) |
| **Moved to `tcn_types::msg`** | `SRNodeScope` (from `sqe_msgs::msg`, to `Frames.idl`), `SISTargetKind` (from `sqe_msgs::rpc`, to `Tracking.idl`) |
| **New, `sqe_msgs::msg`** | `SRRelationRef`, `SRMaterialization`, `SREdgeTiming`, `SREdgeKind`, `SRPose6D`, `SRTranslation3D` |
| **New, `sqe_msgs::rpc`** | `SISStatus`, `SISReason`, `SISReplyHeader`, `SISEmpty` (`rpc/Reply.idl`); `SISEngineCapabilities`, `SISJoinReply`, `SISEngineSummaryReply`; `SISFusionPolicy`, `SISRelationTerms`, `SISRelationState`, `SISParkReason`, `SISRelationStatus`, `SISStreamHandle`, `SISPlanNotification`, `SISRelationOutput`; `SISSampleOutcome`; `SISGraphAnchorKind`, `SISGraphAnchor`, `SISGraphFilter`, `SISGraphSearch`, `SISGraphPath`, `SISGraphQuery`; `SISChangeOriginKind`, `SISChangeOrigin`, `SISExplainSubject`; `SISTargetAliasKey`, `SISTargetFamily`, `SISTargetQuery` |
| **Same name, new header** | `SISFragmentSummary` (`rpc/Operator.idl` → `rpc/Queries.idl`) |
| **Deleted** | `SRNodeTemplate`, `SREdgeTemporalType`, `SRPose6DWithCovariance`, `SRTranslation3DWithCovariance`, `SRIntrinsics`; `SISFrameRef`, `SISMutationStatus`, `SISGraphQueryStatus`, `SISRelationStreamStatus`, `SISRelationStreamHandleKind`, `SISRelationStreamHandle`, `SISRelationSampleStatus`, `SISTargetQueryStatus` |
| **Reshaped** | listed type by type below; the full list with headers is in the type map |

What replaces what:

| 0.10 | 0.11 |
|---|---|
| `SISFrameRef { client, node }` | `tcn_types::msg::SRFrameRef { scope, owner, name, selector }` |
| the five status enums and the `status` field of every reply | `SISReplyHeader { status: SISStatus, reason: SISReason, detail, engine_id, engine_epoch }` as `header`, first member of every reply and notification |
| `SRPose6DWithCovariance`, `SRTranslation3DWithCovariance`, `…_COV` transform cases, "0 or 21 doubles" | `SRPose6D { pose, ? SRCovariance6 covariance }`, `SRTranslation3D { translation, ? SRCovariance3 covariance }` |
| `SRIntrinsics` (`projection` case, `SISPlanOp` PROJECT) | `CameraIntrinsics` (union) holding `PinholeIntrinsics` |
| `SREdgeTemporalType` (6 values) | `SREdgeKind` (`POSE`, `PROJECTION`) plus `SREdgeTiming` (`STATIC`, `STATIC_UNCALIBRATED`, `STATIC_CALIBRATED`, `DYNAMIC`) |
| `SISRelationStreamHandle` union | `SISStreamHandle { handle, data_topic, status_topic, plan_topic }`, an `@optional` member of the start reply |
| `string clock_domain` | `ClockDomain { id, kind }` |

#### Fields: reshaped types

`?` means `@optional` (absent is explicit; there is no sentinel). Every other member keeps its position and type
unless the table says so; a type listed here must be re-decoded as a whole.

**Graph (`sqe_msgs/msg/Graph.idl`, `Transforms.idl`).**

| Type | 0.10 | 0.11 |
|---|---|---|
| `SRNode` | `name` (domain nodes as `@{domain}::{name}`), `node_type`, `scope`, `coord`, `active`, `uuid`, `stream_descriptor_topic`, `pinned`, `node_template`, `materialized_from`, `materialized_id`, `custom_data`, `display_name`, `description` | `name` (always bare), `scope`, **`domain`** (non-empty exactly when scope is DOMAIN), `node_type`, `coord`, `active`, `pinned`, `? stream_descriptor_topic`, `? uuid`, **`? materialization`** (`SRMaterialization`), `display_name`, `description`. Removed: `node_template`, `custom_data`, `materialized_from`, `materialized_id` |
| `SRMaterialization` (new) | – | `template_device` (an `SRFrameRef` whose selector is INSTANCES), `marker_type`, `marker_system`, `marker_id`, `marker_domain` ("" means engine-wide) |
| `SREdge` | `name`, `source`/`target` (bare names), `temporal_type`, `transform`, `weight`, `is_invertible`, `stream_descriptor_topic`, `sequence<SRCalibration>`, `materialized_from`, `derived_from` (strings), `stream_item_id`, `display_name`, `description`, `pinned`, `stream_item_bound` | `name`, `source`/`target` as **`SRFrameRef`**, **`kind`**, **`timing`**, `? transform`, `weight`, `is_invertible`, `? stream_descriptor_topic`, **`? entry_id`**, **`? calibration`**, `pinned`, **`materialized`** (bool), **`derived`** (bool), `derived_from` (`sequence<SRRelationRef>`), `display_name`, `description` |
| `SRCalibration` | `residual` (0 = none), `covariance` (0 or 21 doubles) | `? residual`, `? SRCovariance6 covariance` |
| `SRGraph` | `name`, `origin`, `nodes`, `edges`, `display_name`, `description` | `name` removed (`display_name` serves); the rest unchanged |
| `SRNodeView` / `SREdgeView` | `client` (and `source_client`, `target_client`) | `fragment`; the two endpoint clients are gone (they are in the `SRFrameRef`s) |
| `SRPoint2D` (`@final`) | `u`, `v`, `covariance` | `u`, `v` (still `@final`: 16 bytes, no DHEADER) |
| `SRTransform` (union) | `SRTransformType`: `POSE6D`=0, `TRANSLATION3D`=1, `POSE6D_COV`=2, `TRANSLATION3D_COV`=3, `PROJECTION`=4, `POINT2D`=5 | `POSE6D`=1 (`SRPose6D`), `TRANSLATION3D`=2 (`SRTranslation3D`), `PROJECTION`=3 (`CameraIntrinsics`), `POINT2D`=4. `UNKNOWN`=0; the two `_COV` cases are gone |
| `PinholeIntrinsics` (new, replaces `SRIntrinsics`) | `fx fy cx cy`, `string distortion_model`, `distortion` (≤8), `image_w`, `image_h` | `fx fy cx cy`, `SRDistortionModel distortion_model`, `distortion` (≤16), `? metric_radius`, `? image_width`, `? image_height`. `metric_radius` is the normalized image-plane radius `sqrt(x²+y²)` at `z = 1` within which the model is valid |
| `SRRelationRef` (new) | – | `source`, `target` (both `SRFrameRef`), `name`: an edge is named by its endpoints plus its name |
| `Target` (`tcn_types`) | `pose_covariance`: `sequence<double,21>` | `? SRCovariance6 pose_covariance` |
| `Pose6DMessage` (`tcnart_msgs`) | `pose_covariance`: `sequence<double,21>` | `? SRCovariance6 pose_covariance` |

**Session, mutations, streams, sample (`sqe_msgs/rpc`).**

| Type | 0.10 | 0.11 |
|---|---|---|
| `SISLeaveRequest` | `client` | empty (the client is the key's prefix, N9); encodes as a DHEADER of 0 |
| `SISNodeRemoveRequest` | `name` | `SRFrameRef node` |
| `SISEdgeRemoveRequest` | `name` | `SRRelationRef edge` (a removal is no longer ambiguous) |
| `SISMutationReply` | `status`, `detail`, `engine_id`, `engine_epoch` | `header` |
| join reply | `SISMutationReply` | **`SISJoinReply { header, engine: SISEngineCapabilities }`** |
| `SISEngineInfoReply` | `status`, `detail`, `engine_id`, `engine_epoch`, `engine_version`, `protocol_break`, `schema_version`, `client_scope`, `store`, … | `header`, `engine` (`SISEngineCapabilities { protocol_break, capabilities, client_scope, engine_version, schema_version }`), then `store`, `store_detail`, `fragments_joined`, `fragments_stored`, `relation_streams`, `client_limits`, `operator_limits` |
| `SISEngineSummaryReply` (new) | – | `{ header, engine }`, the answer on `sqe/{engine}/info` |
| `SRStreamTrigger` (union) | `ON_ANY` case: `boolean unused` | `SISEmpty on_any` |
| `SISRelationTerms` (new) | observer, target, trigger, freshness_bias, reoptimise, evaluation repeated in start, sample and explain | one type: `observer`, `target` (`SRFrameRef`), `trigger`, `freshness_bias`, `reoptimise`, `evaluation`, `fusion` (`SISFusionPolicy`: `BEST_PATH`, `FUSED`, `FILTERED`) |
| `SISRelationStreamStartRequest` | `observer`, `target`, `trigger`, `freshness_bias`, `reoptimise`, `evaluation` | `terms` |
| `SISRelationStreamReply` | `status` (`SISRelationStreamStatus`), `handle` (union), `blocked_by`, `detail`, `plan` | `header`, `? handle` (`SISStreamHandle`), `relation` (`SISRelationStatus { state, ? park_reason, detail, blocked_by }`), `? plan` |
| `SISRelationStreamStatusNotification` | `output_topic`, `status`, `detail`, `blocked_by`, `route_epoch` (uint32), `engine_epoch` | `header`, `handle`, `sequence_number`, `? route_epoch` (uint64; absent = never compiled), `relation` |
| `SISPlanNotification` (new) | the plan was published as a bare `SISTranslationPlan` | `{ header, handle, sequence_number, plan }` |
| `SISRelationOutput` (new, H3) | data topic: `TargetTrackingMessage` plus a 13-byte attachment | `{ engine_epoch, route_epoch, sequence_number, clock_domain, total_hops, covariance_hops, targets: TargetTrackingMessage }`, **no attachment** |
| `SISBlockingRelation` | `source`, `target`, `name`, `temporal_type` | `edge` (`SRRelationRef`), `kind`, `timing` |
| `SISPlanInput` | `topic`, `schema`, `device_coord`, `entry_bound`, `entry_id`, `sensor_latency_ns`, `frame_rate` (uint32), `clock_domain` (string), `source`, `buffer_client`, `device`, `target` | `topic`, `schema`, `device_coord`, `? entry_id`, `sensor_latency_ns`, `? frame_rate` (double), `clock_domain` (`ClockDomain`), `source`, `edge` (`SRRelationRef`; replaces the three strings) |
| `SISPlanHop` | `from`, `to`, `edge_name`, `temporal_type`, `derived`, `walked_inverted`, `weight`, `derived_from` (strings), `input` (int32, -1 = none), `covariance` (0 or 21) | `edge` (`SRRelationRef`), `kind`, `timing`, `derived`, `walked_inverted`, `weight`, `derived_from` (`SRRelationRef`s), `? input`, `? SRCovariance6 covariance` |
| `SISPlanOp` (union) | `PROJECT` case: `SRIntrinsics` | `CameraIntrinsics` |
| `SISTranslationPlan` | `plan_version`, `output_topic`, `route_epoch` (uint32), `engine_epoch`, `target_coord`, `instances`, `inputs`, `reference_input` (int32, -1 = none), `correlated_via` (strings), `clock_domain` (string), `path_weight`, `hops`, `route_notes` | `? route_epoch` (uint64; absent for a dry run), `target_coord`, `instances`, `inputs`, `? reference_input`, `correlated_via` (`SRRelationRef`s), `clock_domain` (`ClockDomain`), `path_weight`, `hops`, `route_notes`. Removed: `plan_version`, `output_topic`, `engine_epoch` |
| `SISRelationSampleRequest` | `observer`, `target`, `freshness_bias`, `instant`, `stamp_ns`, `wait_ms`, `include_plan` | `terms`, `instant`, `? stamp_ns` (present exactly when `instant` is AT), `? wait_ms`, `include_plan` |
| `SISRelationSampleReply` | `status` (`SISRelationSampleStatus`), `detail`, `engine_id`, `engine_epoch`, `relation`, `relation_detail`, `blocked_by`, `stamp_ns`, `clock_domain` (string), … | `header`, `relation` (`SISRelationStatus`), **`outcome`** (`SISSampleOutcome`), `? stamp_ns`, `clock_domain` (`ClockDomain`), `targets`, `inputs`, `cached`, `waited_ms`, `? plan` |

**Queries, operator surface, targets.**

| Type | 0.10 | 0.11 |
|---|---|---|
| `SISGraphQueryRequest` | `kind`, `client`, `from`, `to`, `max_items`, `cursor`, `temporal_type[]`, `scope[]`, `node_type[]`, `name_prefix`, `predicate`, `radius` (kind-dependent fields "ignored, not refused") | `query: SISGraphQuery` (union: `FRAGMENT` string, `NEIGHBOURS` `SRFrameRef`, `SEARCH` `SISGraphSearch { anchor, filter }`, `PATH` `SISGraphPath { from, to, predicate, radius }`), `cursor`. Removed: `node_type`, `name_prefix`, `scope`, `max_items` |
| `SISGraphFilter` (new, G3) | name prefixes | `timing[]`, `edge_kind[]`, `? materialized`, `? derived`, `? marker_type`, `? marker_system`, `? marker_domain` |
| `SISGraphQueryReply` | `status`, `nodes`, `edges`, `next_cursor`, `detail`, `path` (strings), `radius_exhausted` | `header`, `nodes`, `edges`, `? next_cursor`, `path` (`SRRelationRef`s), `radius_exhausted`, `? fragment` (`SISFragmentSummary`, for a FRAGMENT read) |
| `SISFragmentSummary` | `client`, `origin`, `joined`, `last_updated_ms`, `node_count`, `relation_count` | `fragment`, `origin`, `joined`, `last_updated_ms`, `node_count`, `edge_count`, `display_name`, `description` |
| `SISPatternSegment` | `relation`, `from`, `to`, `inverted`, `dynamic`, `stream_topic`, `stream_item_id`, `transform`, `computed`, `sensor_latency_ns`, `frame_rate`, `stream_item_bound` | `edge` (`SRRelationRef`), `dynamic`, `? stream_topic`, `? entry_id`, `? transform`, `computed`, `? sensor_latency_ns`, `? frame_rate`. Removed: `inverted` |
| `SISPatternInput` | `role_from`, `role_to`, `segments`, `observer`, `target` (`SISFrameRef`), `frame_rate` | `segments`, `observer`, `target` (`SRFrameRef`), `? frame_rate`. Removed: `role_from`, `role_to` |
| `SISPatternInstance` | …, `min_measurements`, `max_measurements`, `cost`, … | `max_measurements` removed |
| `SISPatternRole` | `frame`: `SISFrameRef` | `frame`: `SRFrameRef` |
| `SISPatternQueryRequest` | `from`, `to`, `wanted`, `radius`, `only_patterns`, `max_candidates` | `from`, `to` (`SRFrameRef`), `wanted`, `radius`. Removed: `only_patterns`, `max_candidates` |
| `SISPatternQueryReply` | `status`, `instances`, `detail` | `header`, `instances` |
| `SISFragmentListRequest` | `who`, `prefix`, `max_items`, `cursor` | `max_items` removed |
| `SISFragmentListReply` | `status`, `fragments`, `next_cursor`, `detail` | `header`, `fragments`, `? next_cursor` |
| `SISGraphChangeNotification` | `sequence_number`, `engine_epoch`, `kind`, `client`, `name`, `origin` (string) | `header` (`engine_id`, `engine_epoch`), `sequence_number`, `kind`, `fragment`, `? node` (`SRFrameRef`), `? edge` (`SRRelationRef`), `origin: SISChangeOrigin { kind: CLIENT/OPERATOR/ENGINE, id }` |
| `SISOperatorMutationRequest` | `who`, `client`, `mutation` | `who`, `fragment`, `mutation` |
| `SISExplainRequest` | `who`, `kind`, `observer`, `target`, `trigger`, `freshness_bias`, `handle` | `who`, `subject: SISExplainSubject` (union: `RELATION` `SISRelationTerms`, `STREAM` handle string) |
| `SISExplainReply` | `status`, `detail`, `relation`, `relation_detail`, `blocked_by`, `plan`, `live` | `header`, `relation` (`SISRelationStatus`), `? plan`, `? live` |
| `SISTransactionLogReply` | `status`, `detail`, … | `header`, `active`, `label`, `file`, `started_ns`, `records`, `dropped` |
| `SISTargetAlias` (N10) | `system` ("aruco:DICT_4X4_50", "art:lab1"), `key`, `config` | `family`, `variant`, `key`, `config` |
| `SISGlobalTarget` | `name`, `uuid`, `kind`, `aliases`, `description`, `custom_data` | `name`, `? uuid` (absent in a REGISTER asks the engine to assign one; a present uuid that differs from the stored one is refused on UPDATE), `kind`, `aliases`, `description`. Removed: `custom_data` |
| `SISTargetQueryRequest` | `kind`, `system`, `key`, `name` | `query: SISTargetQuery` (union: `RESOLVE` `SISTargetAliasKey { family, variant, key }`, `LIST` `SISTargetFamily { family, ? variant }`, `GET` name, `ALL` `SISEmpty`) |
| `SISTargetQueryReply`, `SISTargetChange` | `status` (`SISTargetQueryStatus`) / none | `header` first |

Removed members that nobody wrote meaningfully (F3, F4), as the moves check proves them: `SRNode.{node_template,
custom_data}`, `SRPoint2D.covariance`, `SISPatternInput.{role_from, role_to}`, `SISPatternSegment.inverted`,
`SISPatternInstance.max_measurements`, `SISPatternQueryRequest.{only_patterns, max_candidates}`,
`SISGraphQueryRequest.{node_type, name_prefix, scope, max_items}`, `SISFragmentListRequest.max_items`,
`SISTranslationPlan.plan_version`, `SISGlobalTarget.custom_data`, `SISLeaveRequest.client`.

#### Enums (E6, O5)

- **The complete old → new map, with values:** **[0.11-sqe-enum-map.md](0.11-sqe-enum-map.md)** (20 enums).
- **Every surviving `sqe_msgs` enum** gets the prefix `UPPER_SNAKE(EnumName)_` and a new `…_UNKNOWN` at 0, so every
  other value moves up by one (`SRG_TRIGGER_FIXED_RATE` = 0 → `SR_STREAM_TRIGGER_KIND_FIXED_RATE` = 1). The one
  irregular name: `SIS_GC_RELATION_CHANGED/REMOVED` → `SIS_GRAPH_CHANGE_KIND_EDGE_CHANGED/REMOVED`.
  `SISTargetKind` keeps its ordinals (`SIS_TK_UNSPECIFIED` = 0 → `SIS_TARGET_KIND_UNKNOWN` = 0).
- **Stored or hard-coded numbers invert silently:** `0` used to mean OK, ACTIVE, GENERIC, LOCAL, FIXED_RATE …; now
  it means `UNKNOWN`, and **a zeroed reply is not OK**. A producer that relied on a default must set the field. A
  reader treats an out-of-range value as `UNKNOWN`; the contract library does (`from_ordinal`).
- **New enums:** `SISStatus`, `SISReason`, `SISRelationState`, `SISParkReason`, `SISSampleOutcome`,
  `SISFusionPolicy`, `SISChangeOriginKind`, `SISGraphAnchorKind`, `SREdgeTiming`, `SREdgeKind`, `SRFrameSelector`,
  `ClockKind`, `SRDistortionModel`, `CameraIntrinsicsKind`. `SRTransformType` is reshaped (above).
- **`UNKNOWN` values in a request** (a fusion policy, reoptimise policy, evaluation mode or trigger of `UNKNOWN`)
  are refused: `REJECTED` + `MALFORMED`. `FUSED` and `FILTERED` are refused `REJECTED` + `UNSUPPORTED`: only
  `BEST_PATH` is implemented.

**The status mapping (R1).** Every reply carries `header.status` and `header.reason`; the relation's state and
a sample's outcome are domain fields, not statuses.

| 0.10 | 0.11 `header.status` + `reason` | domain field |
|---|---|---|
| `SIS_MU_ACCEPTED`, `SIS_GQ_OK`, `SIS_TQS_OK` | `OK` | – |
| `*_UNKNOWN_CLIENT`, `*_UNKNOWN_FRAME`, `SIS_GQ_TRUNCATED`, `SIS_TQS_NOT_FOUND` | the same name (`UNKNOWN_CLIENT`, `UNKNOWN_FRAME`, `TRUNCATED`, `NOT_FOUND`) | – |
| explain's `SIS_GQ_UNKNOWN_FRAME` for "no such handle"; `stop` of an unknown handle (R4) | `NOT_FOUND` | – |
| a frame, fragment or domain outside the engine's client scope | `NOT_IN_SCOPE` | – |
| `*_REJECTED` | `REJECTED` + `MALFORMED`, `NOT_PERMITTED` (authorization) or `SCHEMA_MISMATCH`; a start whose terms contradict a running stream (B28): `CONFLICT` + `TERMS_CONFLICT` | – |
| transaction log without a log directory | `REJECTED` + `UNSUPPORTED` | – |
| `*_UNAVAILABLE` | `UNAVAILABLE` (+ `STORE_UNAVAILABLE` when the store is down) | – |
| `SIS_RS_ACTIVE` / `PENDING` / `INACTIVE` | `OK` | `relation.state` `ACTIVE` / `PENDING` (+ `relation.park_reason`) / `INACTIVE` |
| `SIS_RS_NO_PATH` (reply) / (notification) | `REJECTED` + `NO_PATH` / `OK` | – / `PENDING` + `WITHDRAWN` |
| `SIS_RS_CLOCK_DOMAIN_MISMATCH` (reply) / (notification) | `REJECTED` + `CLOCK_DOMAIN_MISMATCH` / `OK` | – / `PENDING` + `CLOCK_DOMAIN_MISMATCH` |
| `SIS_SA_OK` / `NO_VALUE` / `TOO_OLD` | `OK` | `outcome` `VALUE` / `NO_VALUE` / `TOO_OLD` |
| `SIS_SA_NO_ROUTE` | `REJECTED` + `NO_PATH` (the relation fields are still filled) | – |

- **Rules.**
  - A stream start with no route is refused (`REJECTED` + `NO_PATH`). A running stream that loses its route goes
    `PENDING` + `WITHDRAWN` and says so on its status topic.
  - On a reply whose status is not OK, `relation.state` is `UNKNOWN`, `park_reason` is absent and `blocked_by` is
    empty; `relation.detail` carries the route refusal's text. **An OK stream reply whose state is `UNKNOWN` is a
    protocol error**, and the contract library reports it as `HandleInvariantViolated`.
  - A status notification always has status OK and a real state.
  - `park_reason` is present exactly when `state` is `PENDING`.
- **The reasons:** `MALFORMED` (undecodable, or a value outside its range), `SCHEMA_MISMATCH` (the annotation is
  missing or names another release), `UNSUPPORTED`, `TERMS_CONFLICT`, `NOT_PERMITTED`, `NO_PATH`,
  `CLOCK_DOMAIN_MISMATCH`, `LIMIT_EXCEEDED`, `STORE_UNAVAILABLE`.
- **Gate on `header.status`, never on a field of the old enums.** The library gives you `reply_header(reply)`.

#### Frame references (N2a): one text form, one vector file

`SRFrameRef { scope, owner, name, selector }` replaces every string spelling of a frame:

| 0.10 spelling | 0.11 `SRFrameRef` | Text form (N2a) |
|---|---|---|
| `{client: "tcn/loc/a", node: "camera"}` (`FrameRef::parse(client, node)`) | LOCAL, owner `tcn/loc/a`, name `camera` | `local:tcn/loc/a/camera` |
| a Global node (an empty client was refused by `FrameRef::parse`) | GLOBAL, owner `""`, name `world` | `global:world` |
| `@rig::cam` (domain node, B94) | DOMAIN, owner `rig`, name `cam` | `domain:rig/cam` |
| `<device>:::*` (all instances of a template, B87) | selector `INSTANCES` | the form plus `[instances]`, e.g. `local:app/dev1[instances]` |
| an empty client meaning "shared" | gone: say GLOBAL or DOMAIN | – |

Inside a join, an edge endpoint that is LOCAL with an empty owner means "this fragment". That form exists only on
the wire; it has **no text form**, and `FrameRef::from_wire` refuses it everywhere else (`MissingOwner`).

- **The text form is mandatory in every implementation** (configs, UIs, CLIs, logs, tests). There is no bare
  shorthand: `camera` is a parse error that names the three forms.
- **The vector file is the specification:** `src/sqe/vectors/frame_ref.tsv`, installed as
  `share/tcn_schema/sqe/frame_ref.tsv`. It has 69 rows (16 `ok`, 53 `err`). **An implementation that does not pass
  every row does not speak N2a.** The C++ test (`test_frame_ref.cpp`) and the engine's Rust tests must both run it.
  The file header states the parse order and the file format (TAB separated, no quoting, `#` comments, one CR
  before the LF is stripped).
- **The parse, in this order; the first failure wins:**
  1. empty text: `empty`;
  2. text containing `[`: it must end with `[instances]` and contain no other `[`, else `malformed_selector`; strip
     it (selector `INSTANCES`, otherwise `FRAME`);
  3. the scheme is everything before the **first** `:`; none: `missing_scheme`; not exactly `local`, `global` or
     `domain` (case-sensitive): `unknown_scheme`;
  4. `global`: a body containing `/` is `unexpected_owner`; else the identifier rule on the body;
  5. `local`, `domain`: split the body at the **last** `/` (none: `missing_owner`); the fragment rule on the owner,
     then the identifier rule on the name.
- **The alphabets:** a fragment or domain takes `[A-Za-z0-9_.:/-]`, non-empty, no empty chunk; a name takes
  `[A-Za-z0-9_.:-]`, non-empty, no `/`. A name may contain `:` (`global:a::b` is the name `a::b`).
- `FrameRef::format` is the inverse: text → ref → text is exact.

#### Keys

| 0.10 | 0.11 |
|---|---|
| `/sis/component/update`, `/sis/component/remove` | `/sis/node/update`, `/sis/node/remove` |
| `/sis/relation/update`, `/sis/relation/remove` | `/sis/edge/update`, `/sis/edge/remove` |
| `/sis/relation/sample` | `/sis/stream/sample` (so nothing is left under `/sis/relation/**`) |
| `{client}/cfg/marker/{device}/{marker_name}` | `{client}/cfg/marker/{device}/{marker_id}` (the decimal `uint32`; H5) |
| `{handle}/status`, `{handle}/plan`, composed by the client | `status_topic`, `plan_topic` from the start reply (H2). The engine may keep composing them, but a client **must not** |
| the data topic, `TargetTrackingMessage` plus a 13-byte attachment | the data topic named by `data_topic`, `sqe_msgs::rpc::SISRelationOutput`, no attachment (H3) |
| – | `sqe/{engine}/info`: any client may ask, before it joins, `SISEngineInfoRequest` → `SISEngineSummaryReply` (R5, D4). `sqe/*/info` finds every engine |

Unchanged: `/sis/join`, `/sis/leave`, `/sis/alive`, `/sis/stream/start|stop`, `/sis/graph/query`,
`/sis/pattern/query`, `/sis/target/query|update`, `sqe/{engine}/alive/{epoch}`, `…/targets/changed`, `…/op/*`.

#### Capabilities replace `protocol_break` gates (R5, D4)

`SISEngineCapabilities { protocol_break, capabilities, client_scope, engine_version, schema_version }` is in the join
reply, in `op/info` and in the `sqe/{engine}/info` summary.

- **One protocol floor.** The engine's B-number for the break is `protocol_break`; this library requires
  `>= kProtocolBreak` (`TCN_SQE_PROTOCOL_BREAK`, **99**, provisionally; Plan 6 owns the final number). Below it the
  engine speaks the 0.10 wire. **There are no other B-levels:** `kContractBLevel`, `kSessionBLevel`,
  `kProtocolBLevel` and their macros are deleted, so code that asserted on them now fails to compile.
- **Optional features are names, not higher numbers:** a client asks `Capabilities::has(...)` instead of
  `protocol_break >= 93`. A name is `[a-z0-9_]{1,64}`; a client refuses a malformed one (`MalformedCapability`) and
  ignores a well-formed one it does not know.

| Capability | Meaning | Replaces |
|---|---|---|
| `sample` | the engine answers `/sis/stream/sample` | `protocol_break >= 93` (B93) |
| `domain` | the engine serves DOMAIN-scoped frames (`SR_NODE_SCOPE_DOMAIN`, `domain:<domain>/<name>`) | B94 |
| `rational_lens` | the engine accepts and applies `SR_DISTORTION_MODEL_OPENCV_RATIONAL`; without it such an edge is refused (`REJECTED` + `UNSUPPORTED`) | new |
| `marker_domain` | the engine reads and writes marker domains: a marker id unique within a domain, not engine-wide (`SRMaterialization.marker_domain`, the `SISGraphFilter.marker_domain` filter) | `{system}@{domain}` in `marker_system` (B97) |

- **`client_scope` and "no engine serves `<id>`".** An engine serves the client ids its scope contains: `""`
  contains every client; otherwise a client id equals the scope or has it as a prefix **ending at a `/` chunk
  boundary** (`tcn/loc/ch` contains `tcn/loc/ch/room`, never `tcn/loc/chx`). A frame, fragment or domain outside the
  scope is answered `NOT_IN_SCOPE`. A client that asks `sqe/*/info` and finds no engine whose scope contains its id
  reports "no engine serves `<id>`" instead of timing out (before, a sample for such a client ended as a silent
  timeout).

#### Handles, topics and sequences (H1, H2, R4, R6)

- **The start reply** has `handle` (an `@optional SISStreamHandle`), present exactly when `header.status` is `OK` and
  `relation.state` is `ACTIVE` or `PENDING`. A client subscribes to `data_topic`, `status_topic` and `plan_topic` as
  given; `handle` itself is opaque. The engine may keep `handle == data_topic` and `{data_topic}/status|/plan`.
- **Stop:** `OK` or `NOT_FOUND` (an unknown handle) both release the lease.
- **Sequences:** `sequence_number` counters are per topic (status, plan, data), start at 1 and are gapless per
  handle. (The IDL keyword `sequence` cannot be a member name; `SISGraphChangeNotification` already used
  `sequence_number`, so the three new counters use it too.)
- **The relation output** is `SISRelationOutput`; `covariance_hops`/`total_hops` are once per message. The plan is
  published as `SISPlanNotification`.
- **Data topic subscribers must decode `SISRelationOutput`**, not `TargetTrackingMessage`: it is `targets` inside.

#### `marker_system` until the descriptor split (S1, N6 in Plan 6)

`StreamDescriptorMessage.marker_system` is still one string, and the engine still reads the 0.10 form
`{system}@{domain}` from it. The alias it resolves against is now `(family, variant, key)`, so until Plan 6:

| What | Until Plan 6 |
|---|---|
| tracker descriptor | keep writing `marker_system` as today: `"art"`, `"aruco:DICT_4X4_50"`; a marker domain, if any, as `…@{domain}` |
| the engine | reads `{system}@{domain}` as before; the alias lookup is `(family, variant, str(id))` (the `Targets.idl` comment), so `system` is split at the first `:` into `family` and `variant`. Confirm this mapping (SQE review) |
| registering a target | `SISTargetAlias { family: "aruco", variant: "DICT_4X4_50", key: "7" }`, not `system: "aruco:DICT_4X4_50"` |
| finding a marker node | by `SISGraphFilter.{materialized, marker_type, marker_system, marker_domain}`, never by name: node names are engine-chosen and opaque (N5), and `marker_node_name`/`marker_prefix` are deleted |

Plan 6 replaces the string with the descriptor's own `marker_system` and `marker_domain` fields and the sensor
union.

#### The contract library (`tcn::sqe`, sqe_client_cpp 0.7.0)

| 0.10 | 0.11 |
|---|---|
| `FrameRef { client, node }`, `FrameRef::domain(...)`, `.client()`, `.node()` | `FrameRef::local(owner, name)`, `global(name)`, `domain(domain, name)`, `parse(text)`, `format()`, `instances()`, `scope()/owner()/name()/selector()`, `from_wire`/`to_wire`; no public constructor |
| `kDomainNodeSigil`, `kNodeIdSeparator`, `domain_node_id`, `parse_domain_node_id`, `LocalNodeId`, `GlobalNodeId`, `DomainNodeId` | deleted (`NodeName` stays) |
| `marker_prefix`, `marker_node_name`, `kMarkerNodeInfix`; `marker_announce_key(client, device, name)` | deleted; `marker_announce_key(client, device, std::uint32_t id)` |
| `Status`, `status_from_ordinal`, `kMaxStatusOrdinal` | `ReplyStatus`, `Reason`, `RelationState`, `ParkReason` (`std::int32_t`, `Unknown` = 0), `from_ordinal<E>`, `wire_name`, `carries_handle` |
| – | `ReplyHeader` and `reply_header(reply)` (works on every reply and notification; a zeroed reply is `!ok()`) |
| `Handle` over a union (`kHandleKindNone/Assigned`, `reply_status`) | `Handle::from_reply`: not OK gives `ReplyRefused`; state and handle disagreeing (or state `Unknown`) gives `HandleInvariantViolated`; absent gives `HandleAbsent`; an empty topic gives `EmptyHandleTopic`. Accessors `str()`, `data_topic()`, `status_topic()`, `plan_topic()`, `state()` |
| `StopFn` returns a status | `StopFn` returns `Result<ReplyHeader>` |
| – | `Capabilities::from_wire`, `has`, `names`, `scope`, `meets_floor`, `capability::kSample/kDomain/kRationalLens/kMarkerDomain` |
| – | `ClientScope::parse`, `contains`, `unscoped`; `engine_info_key(id)`, `engine_info_pattern()` |
| `keys::kRelationSample`, `relation_sample_key()`; `keys::kNodeUpdate`/`kNodeRemove`/`kEdgeUpdate`/`kEdgeRemove` hold the old strings | `keys::kStreamSample`, `stream_sample_key()`; the four constants keep their names and now hold `/sis/node/*` and `/sis/edge/*` |
| `TriggerKind`, `ReferencePolicy`, `EvaluationMode` (`EvaluationMode` was `std::uint32_t`), no `Unknown` | all with `Unknown` = 0 and `std::int32_t`, values shifted as in the enum map; new `ReoptimisePolicy` and `FusionPolicy`; `RelationStreamRequest::make(…, evaluation, reoptimise = BestAvailableNow, fusion = BestPath)`; `same_terms_as` compares trigger, bias, evaluation, fusion and reoptimise |
| `kContractBLevel`, `kSessionBLevel`, `kProtocolBLevel`, `TCN_SQE_CONTRACT_B_LEVEL` | deleted; `kProtocolBreak` (`protocol.h`) |
| the declared-type registry (`wire_type.h`) | registers the 0.11 types; deleted types are gone; join is answered by `SISJoinReply` |
| annotation without `tcn_schema=` accepted as a pre-0.9 peer | **refused**: `MissingSchemaVersion`. `application/cdr` and `application/json` are matched **byte for byte** (no case folding, no trimming, no substring) |
| new errors | `UnknownScheme`, `MissingScheme`, `MissingOwner`, `UnknownScope`, `UnknownSelector`, `MalformedCapability`, `ReplyRefused`, `MissingSchemaVersion`, … (every error has a `code()` string) |

**Contract-library notes for the SQE review (the implementer choices, none changes the wire):**

- `same_terms_as` compares `reoptimise` because the lease table sends one start per relation, so a second local
  component's stronger policy would otherwise be dropped before it reaches the engine. Refusing it locally
  (`ConflictingLease`) is explicit. The engine's stream identity is unchanged (it takes the union of what it
  receives).
- The media type is compared byte for byte. If the engine lower-cases media types, the engine and the library differ
  on upper-case peers only.
- A lease is released on any stop answer, as before.

#### Golden files (new in this plan)

`test/golden/*.hex`, values in `test/golden/golden_test.cpp`; encode with the Plan 4 rules, and check every
presence byte:

| File | What it pins |
|---|---|
| `tcn_types_SRFrameRef.hex` | a `LOCAL` / `"a"` / `"b"` / `FRAME` reference: DHEADER 0x18, then four fields |
| `tcnart_msgs_TargetTrackingMessage.hex` | now two targets: one without and one with `pose_covariance` (presence byte, `SRCovariance6` DHEADER 0xa8) |
| `sqe_msgs_SRGraph.hex` | regenerated: bare names, `domain`, frame-ref endpoints, an edge with a projection (`CameraIntrinsics`, presence bytes of `metric_radius`, `image_width`, `image_height`), `entry_id` 0 present, absent `calibration` |
| `sqe_msgs_SISMutationReply.hex` | `SISReplyHeader` |
| `sqe_msgs_SISRelationStreamStartRequest.hex` | `SISRelationTerms`, the trigger union, three enums (`BEST_POSSIBLE`, `ENGINE`, `BEST_PATH`) |
| `sqe_msgs_SISRelationStreamReply.hex` | `@optional handle` present (`01`), `@optional park_reason` present (`01`), `@optional plan` absent (`00`), a `blocked_by` element |

- **An `@optional` member costs one presence byte, then the value.** A reader that treats an absent member as a
  default (identity transform, "exact" covariance, no handle) is wrong: absent means absent.
- **Hand-written codecs:** no IDL member uses an escaped identifier. The empty structs `SISEmpty` and
  `SISLeaveRequest` encode as a DHEADER of 0.

#### For SQE review

Deviations and choices made while implementing, all inside the ruling that each is the smallest change that
satisfies the spec. Each needs an answer from the SQE side before `0.11.0-rc1`.

1. **`SRGraph.name` is removed**; `display_name` serves (N9).
2. **`SRMaterialization` uses `marker_domain` and an `SRFrameRef` template device** (N5). `SRNodeRef` was deleted in
   Plan 3.
3. **`SISPlanInput` names its edge by `SRRelationRef`** (it had `buffer_client`, `device` and `target`).
4. **`covariance_hops` and `total_hops` are per `SISRelationOutput`**, not per target. With an `[instances]` selector
   the instances may differ in covariance completeness; moving them per target is a reshape, so decide before rc1.
5. **The R6 counters are named `sequence_number`** (`sequence` is an IDL keyword), like
   `SISGraphChangeNotification.sequence_number`.
6. **`NOT_IN_SCOPE` answers a frame, fragment or domain outside the engine's client scope**; `NOT_PERMITTED` is
   authorization. A transaction log without a log directory is `REJECTED` + `UNSUPPORTED`.
7. **Non-OK relation state:** `SISRelationStatus` on a non-OK reply has state `UNKNOWN`, no `park_reason` and an
   empty `blocked_by`. The library reports an OK reply with state `UNKNOWN` as `HandleInvariantViolated`.
8. **`UNKNOWN` term values are refused as `MALFORMED`**: the engine must implement it (fusion, reoptimise, evaluation,
   trigger).
9. **`SISPlanInput.frame_rate`, `SISTranslationPlan.route_epoch` and the status notification's `route_epoch` are
   `@optional`**, as are `SISPatternSegment.{sensor_latency_ns, frame_rate}`, `SISPatternInput.frame_rate` and
   `PinholeIntrinsics.{image_width, image_height, metric_radius}`: a `0` that meant "none" is now absence (P3).
10. **Capability wording:** `rational_lens` and `marker_domain` are described in the table above (the proposal does
    not say more); confirm or correct.
11. **`SRReferencePolicy.SLOWEST`** is "the lowest declared `frame_rate`" (`frame_rate` is `@optional` now; a hop with
    none does not count).

#### Tools

- `tools/idl_cli.py moves --old <plan 4 src/dds> --new src/dds --spec tools/idl/sqe_0_11.json --plan "plan 5"
  --json src/dds/tcn/type_map_0_11_sqe.json --markdown docs/migration/0.11-sqe-type-map.md` proves that nothing
  else changed (new, moved, reshaped, deleted and removed members are all listed in the spec).
- `tools/idl_cli.py enums` applied `enum_renames_0_11_sqe.json`; the lint baseline is now empty.
- `tools/idl/tests/test_contract_enums.py` pins every C++ enum mirror to the IDL (UNKNOWN first, `std::int32_t`) and
  every wire-registered and request-keyed type to the schema.

## 2. Per consumer

Each list collects the items of §1 that need action from that consumer, plus the decided changes still to come (with
their plan), so that each team can prepare.

### SQE engine (spatial_query_engine_rs, sqe-calibrate)

- *Plan 3:* (counts are approximate: a `git grep` for the old fully qualified names, outside third-party and build directories)
  - about 82 moved type names in 14 files, mostly the `messages.rs` wire registry: `tcnart_msgs::rpc::SIS*` → `sqe_msgs::rpc::*` (74), `tcnart_msgs::msg::SR*` → `sqe_msgs::msg::*` (5), shared types → `tcn_types::msg::*` (3);
  - drop the `Pose6DMapMessage` tracker payload (`messages.rs:2081-2112`, decided), the definitions of `Pose6DMapMessage`, `RigidTransformMap` and `RigidTransformMapItem` (`crates/sqe-core/src/messages.rs:1958-1985`), and the `CameraInfoMessage` test mention;
  - generate the renames from `share/tcn_schema/type_map_0_11.json`.
- *Plan 2:* nothing to do. Optional: `ros2_types.json` gives ROS names and hashes for showing ROS frames later.
- *Plan 1:* optional:
  - check the Rust codec against `test/golden/*.hex`;
  - start checking the `messages.rs` registry against `share/tcn_schema/types.json`.
- *Plan 4:*
  - **Rust codec to XCDR2** for every tcn type: `00 09`, DHEADERs, 4-byte maximum alignment, optional flags, also
    inside the nested ROS `Header`. Today `messages.rs:3737` and `serialization.rs:315,360` check `00 01`.
  - Test against `test/golden/*.hex`, including the two `evo_*` files.
  - Annotation `tcn_schema=0.11`.
  - **Removed fields it reads:**
    - `TargetTrackingMessage.target_origin` (`src/ingest.rs:718-730`, the warning at `messages.rs:2167`);
    - `Target.quality` (#35 G2).
  - **Removed fields it writes or mirrors:** `StreamDescriptorMessage.{calib_topic_schema, audio_channels,
    aac_profile}` and the `AudioAACProfile` mirror (`crates/sqe-core/src/engine.rs:7515,7538-7539,9610-9624`).
  - Media enum and `CoordinateAxis` mirrors in `messages.rs` take the new names and values (enum map).
- *Plan 5:* (counts are approximate: a `grep` for the old names in the `.rs` files, outside `target/` and the docs)
  - **Read first:** [Plan 5 in §1](#plan-5-the-sqe-wire-redesign-35-wire-change). The whole `sqe_msgs` wire changes, so
    this is the largest item of the plan. Generate the renames from `type_map_0_11.json`, then `type_map_0_11_sqe.json`.
  - **The wire mirrors in `crates/sqe-core/src/messages.rs`** (about 840 uses of the deleted status and handle
    types, 360 of `SISFrameRef`, in about 60 files, tests included): every reshaped type and every enum value of
    the tables above, in particular
    - `SRNode`, `SREdge`, `SRCalibration`, `SRGraph` (no `name`), the views, `SRTransform` (no `_COV` cases; covariance
      is an optional `SRCovariance6`/`SRCovariance3` inside the case), `SRPoint2D` (no covariance, still 16 bytes);
    - **every reply starts with `SISReplyHeader`**; `SISMutationStatus`, `SISGraphQueryStatus`, `SISTargetQueryStatus`,
      `SISRelationStreamStatus`, `SISRelationSampleStatus` and `SISRelationStreamHandle` go (`messages.rs:570, 908,
      1335, 1383`);
    - `SISFrameRef` (`messages.rs:679`) becomes `SRFrameRef`; edges name each other with `SRRelationRef`;
    - the stream types: `SISRelationTerms`, `SISRelationStatus`, `SISStreamHandle`, `SISPlanNotification`,
      `SISRelationOutput`, the plan types (`ClockDomain`, optional epochs and indexes).
  - **Stored values: the engine persists enum names, not ordinals.** `src/persistence.rs` writes `node_type` and
    `temporal_type` through `enum_to_stored_str`, i.e. the `#[serde(rename = "SRG_NODE_GENERIC")]` strings of
    `messages.rs`. After the rename a 0.10 row holds a name that no longer parses:
    - an unreadable `temporal_type` reads as **`StaticUncalibrated`** (`read_temporal_type`, fail closed: the edge is
      not traversed) and an unreadable `node_type` reads as `Generic`. A store written by 0.10 would silently lose
      every edge;
    - migrate the stored rows, or keep the old names as `serde(alias)`. The edge's single `temporal_type` also splits
      into `kind` and `timing`; the store must write both;
    - the same holds for any JSON fixture (`fixtures/wire/*.json`, `tests/wire_golden.rs`) and for `0` as a default:
      `0` is `UNKNOWN` now.
  - **`FrameRef` text and wire:** implement parse and format against **`share/tcn_schema/sqe/frame_ref.tsv`**
    (`src/sqe/vectors/frame_ref.tsv`, 69 rows) in the Rust tests; it is mandatory. `sqe-op`, sqe-calibrate command
    lines and logs use the text form. Domain ids as `@{domain}::{name}` and the `:::*` suffix go (`context.rs`,
    `driver.rs:1327, 2225, 3176`).
  - **Header at every refusal site, including ingest:** every reply and every notification carries
    `SISReplyHeader { status, reason, detail, engine_id, engine_epoch }` (the refusals in `src/ingest.rs`, the
    mutation replies, the operator replies). Map the old statuses with the status table; `NOT_IN_SCOPE` for a client
    scope miss, `NOT_PERMITTED` only for authorization; `UNKNOWN` terms are `REJECTED` + `MALFORMED`; `FUSED` and
    `FILTERED` are `REJECTED` + `UNSUPPORTED`; the transaction log without a directory is `REJECTED` + `UNSUPPORTED`.
  - **Data topic:** publish `SISRelationOutput` and no attachment (`crates/sqe-core/src/pose_attachment.rs` and its
    use in `src/pipeline_task.rs` go; `TargetTrackingMessage` only inside `targets`). `route_epoch` is set; counters
    are `sequence_number`, per topic, from 1, gapless per handle.
  - **Explicit topics:** the start reply carries `data_topic`, `status_topic` and `plan_topic`. The engine may keep
    composing them (`crates/sqe-core/src/topics.rs:243,249` do `{output_topic}/status`); a client no longer does.
    Status and plan are published as `SISRelationStreamStatusNotification` and `SISPlanNotification`.
  - **Keys** (`topics.rs:34-43, 102`): `/sis/node/*`, `/sis/edge/*`, `/sis/stream/sample`; the marker announcement
    key carries the decimal id (`topics.rs:131-152`, `marker_announce_key`/`parse_marker_announce_key`); add the
    queryable `sqe/{engine}/info` (`SISEngineInfoRequest` → `SISEngineSummaryReply`).
  - **Join, info, capabilities:** join answers `SISJoinReply`; `SISEngineInfoReply` has `header` and `engine`
    (`src/operator.rs:49` `PROTOCOL_BREAK` 98 → the 0.11 break, provisionally 99, and `SCHEMA_VERSION` 0.10.7 →
    0.11.0). Announce `sample`, `domain`, `rational_lens` (if implemented) and `marker_domain`, and the
    `client_scope` ending at a `/`.
  - **Markers and names:** `marker_node_name`/`marker_prefix` (`engine.rs:612`, `context.rs:150,190`,
    `relation_streams.rs:182-195`) become engine-private; write `SRMaterialization` and `SREdge.materialized`; answer
    `SISGraphFilter` (G3) instead of prefix filters. `SISTargetAlias` is `family` + `variant` + `key`. Until Plan 6,
    `marker_system` on the descriptor is still `{system}@{domain}`.
  - **`SISRelationOutput`, sample outcomes, relation state:** a start with no route is `REJECTED` + `NO_PATH`; a running
    stream that loses its route goes `PENDING` + `WITHDRAWN`; a sample's `NO_VALUE`/`TOO_OLD` are `OK` with
    `outcome`. On a non-OK reply `relation.state` is `UNKNOWN`.
  - **The §6 shims that go (one break, one B-number):**
    - B73: the warning subscribers for a `put` on mutation keys (`src/ingest.rs:1217-1230`);
    - B90 (decision D5): marker discovery from poses (the warning at `engine.rs:5725` and the code behind it);
      every publisher announces its markers;
    - B41: the inactivity sweep and heartbeat fallback for clients without a liveliness token (`src/liveness.rs`,
      `driver.rs:1094`);
    - B94: re-deriving unqualified marker announcements (moot after H5);
    - pre-0.9 descriptor padding (`serialization.rs:205-225`, `decode_stream_descriptor`; `ingest.rs:376`);
    - unversioned payloads (`serialization.rs`, "absent is a pre-0.9 peer: accepted"): `tcn_schema=0.11` is
      **mandatory**, and `application/cdr` or `application/json` are matched exactly.
    - **One B-number** for the break; there is no B-level per feature after it.
  - **sqe-calibrate** (`crates/sqe-calibrate`): `temporal_type`, `SRIntrinsics` and `…WithCovariance` in
    `protocol.rs`, `publish.rs`, `local.rs`, `materialise.rs`, `sampler.rs`; the `/sis/relation/*` keys in
    `command.rs` and `client.rs:394`; `stream_item_*` in `sampler.rs` and `materialise.rs` (now `entry_id`).
    `SRCalibration` has optional `residual` and `covariance`, and a projection is a `CameraIntrinsics`.

### sqe_client_cpp

- *Plan 1:* nothing to do.
- *Plan 4:* the contract library sends `tcn_schema=0.11`. Encode with `XCDRv2` + `DELIMIT_CDR2`, and decode by the
  encapsulation id (see the table above).
- *Plan 5:* sqe_client_cpp 0.7.0 (the break; the library of this repository is its base). Details in the table
  "The contract library" in §1.
  - **`FrameRef`:** build with `FrameRef::local/global/domain`, parse text with `FrameRef::parse`, print with
    `format()`; the text form is mandatory and the vector file is run by your tests. `@{domain}::{name}`,
    `<device>:::*`, the empty-client "shared" ref and `FrameRef::domain(...)`-as-id are gone.
  - **Handles and replies:** `Handle` exposes only the topics the reply gave (`data_topic()`, `status_topic()`,
    `plan_topic()`), never composed ones; read every reply with `reply_header(reply)` (`ReplyHeader`, `ok()`);
    `StopFn` returns `Result<ReplyHeader>`; handle the new errors (`ReplyRefused`, `HandleInvariantViolated`,
    `MissingSchemaVersion`, `MalformedCapability`, the frame-ref errors).
  - **Engine discovery:** `Capabilities::from_wire`, `has(capability::kSample)` and `meets_floor()` replace
    `protocol_break` comparisons; `ClientScope`, `engine_info_key`/`engine_info_pattern` ("no engine serves `<id>`").
  - **Removed:** `kDomainNodeSigil`, `kNodeIdSeparator`, `domain_node_id`, `parse_domain_node_id`, `LocalNodeId`,
    `GlobalNodeId`, `DomainNodeId`, `marker_prefix`, `marker_node_name`, `kMarkerNodeInfix`, `Status`,
    `status_from_ordinal`, `kContractBLevel`, `kSessionBLevel`, `kProtocolBLevel` and the two B-level macros. Code that
    used one fails to compile.
  - **Keys:** `stream_sample_key()` (was `relation_sample_key()`); node and edge keys changed their strings only;
    `marker_announce_key(client, device, std::uint32_t id)`.
  - **Enums:** the C++ mirrors (`TriggerKind`, `ReferencePolicy`, `EvaluationMode`, `ReoptimisePolicy`,
    `FusionPolicy`, the status enums) are `std::int32_t`, `Unknown` = 0; unknown ordinals map to `Unknown`.
  - **Wire registry:** `wire_type.h` registers the 0.11 types; annotate every payload `…;tcn_schema=0.11`.

### tcnart (v3)

- *Plan 3:*
  - about 131 moved type names in 60–70 files under `v3/`: to `sqe_msgs::rpc` (80), `pcpd_msgs::rpc` (23, the parameter RPC), `sqe_msgs::msg` (16) and `tcn_types::msg` (12);
  - idlgen's default package list gains `tcn_types` and `sqe_msgs`; the exclusion of the legacy rpc SpatialRelations types goes (the file is deleted);
  - the vendored snapshot needs the new `src/dds/tcn` layout;
  - goldens that list `device_hl2_msgs`/`device_orbbec_msgs` types need regenerating;
  - remove the two stale device opt-ins (`TcnartV3Options.cmake:27`);
  - **`Pose6DMapMessage` is deleted (XA7):** drop its registration (`wire/include/tcnart/wire/wire_type.hpp:105`), its decoder (`features/src/pose/pose_wire.cpp:56,73,82,144`) and its tests (`tests/sqe/test_sqe_conformance.cpp:297,310`, `tests/wire/test_cdr.cpp:117`, `tests/wire/test_wire_type.cpp:111`);
  - **`conversation_msgs` headers are PascalCase now:** `wire/include/tcnart/wire/conversation_msgs.hpp:17-22` includes `conversation_msgs/msg/audio.h`, `common.h`, `rpc/discovery.h`, … , which become `Audio.h`, `Common.h`, `rpc/Discovery.h` (they fail on case-sensitive file systems otherwise);
  - `tools/idlgen/mapping.py:39` names the deleted `ComponentInfoReply`.
- *Plan 2:*
  - refresh the vendored snapshot `v3/third_party/tcn_schema_idl` (`Time_.idl` → `Time.idl`, `String_.idl` →
    `String.idl`, generated ROS files with `@final`, new packages);
  - `idlgen.py check-package` will report the differences until then;
  - the new ROS packages are opt-in for idlgen, as before;
  - C# names of the fixed-array typedefs (`double__36`) disappear: use the array type;
  - tcnart's own fixtures that name the old files or typedefs need updating with the snapshot:
    - `v3/tools/idlgen/tests/test_parser.py` (lines 46, 98–103, 120: `geometry_msgs::msg::double__36`, `Time_.idl`);
    - `v3/tools/idlgen/tests/test_gen.py` (line 147: `builtin_interfaces/msg/Time_.h`);
    - the C# check's golden `hosts/unity/Tools~/csharp-check/golden/types.json` (`Range`, `NavSatStatus`).
- *Plan 1:*
  - Check that no C++ code uses `operator==` on the 14 parameter and service-info types listed in §1 (it compares pointers).
  - Optional: point the C# codec tests at `test/golden/*.hex`.
- *Plan 4:*
  - **XCDR2 in every codec:** the C# codec, the zero-copy encoders, the mesh-bitstream decoder and the C API.
    `docs/developers/interop.md:63` documents `DDS_CDR` today.
  - **Removed fields it reads:**
    - `MeshBitstreamMessage.origin_offset` and `is_compressed` (`mesh_bitstream_decode.cpp:61,77-78`);
    - `DeviceContextReply.is_valid` (`TcnartRigCamera.cs:63` and Unreal: gate on `status`);
    - `Target.quality` (XR writes a copy of `is_tracked`, and `features/src/camera/marker_publish_op.cpp:92` writes
      it: drop both).
  - **Eye tracking:** leave `scene_point` and `gaze_space` absent instead of writing (0,0) and `LOCAL`.
  - **Enum renames:** media types, `CameraModelType`, `XRGazeSpaceType`, `CoordinateAxis`, the parameter RPC enums.
- *Plan 5:* (counts are approximate, `v3/` only, outside `third_party/`)
  - **C# regeneration and the idlgen goldens:** every C# type of the SQE (the generated types in
    `hosts/unity/Tools~/csharp-check/golden/types.json`, about 40 old enumerators, and
    `tools/idlgen/tests/golden/default_schema_defs.json`, about 100) changes; re-run `idlgen` against the new
    snapshot (`v3/third_party/tcn_schema_idl`) and refresh both goldens. Take the new type names from the two type
    maps.
  - **Frame refs in configs as text:** `sqe/src/frame_text.cpp` (`frame_from_text`, bare name or `<client>/<node>`)
    is replaced by `tcn::sqe::FrameRef::parse`. **A bare name is an error now**: configs, CLIs and the C ABI take
    `local:<fragment>/<name>`, `global:<name>` or `domain:<domain>/<name>`, with `[instances]` where a template
    is meant. The token `SQE_FRAME_INVALID` stays; the message names the three forms. Run the vector file in your
    tests.
  - **Capability names instead of `protocol_break`:** `docs/internals/sqe.md` and `testing.md` gate the sample on
    `protocol_break >= 93`; use `Capabilities::has("sample")`. `SqeLink::sample` can fail fast with "no engine
    serves `<id>`" (`sqe/*/info`) instead of the 2.5 s timeout.
  - **Marker announce by id:** `tcn::sqe::marker_node_name` and the marker node name go
    (`sqe/src/control_thread.cpp:769`, `features/src/camera/camera_sqe_part.cpp:177`, `sqe/src/marker_ref.cpp`,
    `sqe/include/tcnart/sqe/{marker_ref,sqe_link,client_port}.hpp`, `vision/include/tcnart/vision/observations.hpp:23`):
    announce `{client}/cfg/marker/{device}/{marker_id}` (decimal `uint32`), and find marker nodes by
    `SISGraphFilter.{materialized, marker_type, marker_system, marker_domain}`. Descriptor `marker_system` stays one
    string until Plan 6.
  - **Output type:** the data topic is `SISRelationOutput`, with no attachment: `PoseOrigin` and
    `sqe_client::PoseAttachment` (`sqe/include/tcnart/sqe/client_port.hpp:32-43`) go, as does the baseline matching
    on the attachment in `features/src/sqe/sqe_relations.cpp:59-63`; take `engine_epoch` and `route_epoch` from the
    message. The generic zenoh attachment plumbing in `wire/` stays; only the SQE pose attachment goes.
  - **Status handling:** about 100 uses of the removed status types in 21 files, goldens included (`tests/sqe/test_sqe_parts.cpp` 37,
    `features/src/sqe/sqe_relations.cpp`, `sqe/src/control_thread.cpp`, `tools/tcnart_wire_peer/sqe_sample.cpp`):
    gate on `reply_header(reply).ok()` and read `relation.state` and `outcome`.
  - **Enum values:** about 700 old enumerator mentions in 33 files (`tests/features/test_sqe_relations.cpp`,
    `tests/sqe/*`, `tests/features/test_xr_user_frames_daemon.cpp`); take the values from the sqe enum map. The default
    `0` is `UNKNOWN`.
  - **Everything else of the table above**: `SRGraph.name`, `SRNode.domain`/`materialization`, `SREdge`
    (`kind`, `timing`, `@optional transform`), the graph query union, the target alias split, `SISRelationTerms`.
    The `pose_covariance` members are `@optional SRCovariance6`; a reader must not treat an absent covariance as
    zero ("exact"). `docs/internals/sqe.md` and `conanfile.py:128` describe the 0.10 wire.

### HTA (holoscan-tcn-applications)

- *Plan 3:*
  - about 45 moved type names in about 40 files: `sqe_msgs::rpc` (19), the parameter RPC `pcpd_msgs::rpc` (15), `tcn_types::msg` (7), `sqe_msgs::msg` (5);
  - HTA links `tcn_schema::tcn_schema`, so no component change is needed;
  - `tools/common/web_golden.hpp:16` includes `tcnart_msgs/msg/Types.h`, which is gone: the media enums are in `MediaTypes.h` (see "Same name, new header" in the type map);
  - the FlatBuffers mirrors follow the renames.
- *Plan 2:* nothing to do. No HTA code includes the renamed headers or names the removed typedefs. `PointCloud2` and `Header` are byte-identical.
- *Plan 1:*
  - Check that no C++ code uses `operator==` on the 14 parameter and service-info types listed in §1 (it compares pointers).
  - Requiring `fast-cdr` yourself is no longer necessary (it is harmless to keep).
- *Plan 4:*
  - **`tcn::zenoh` serde:** `XCDRv2` + `DELIMIT_CDR2` for tcn types, `DDS_CDR` for ROS types, and decode by
    encapsulation id.
  - **Removed fields it writes or passes through:**
    - `DeviceContextReply.{timestamp_offset, is_valid}` (`mock_capture_server.cpp:77-82`);
    - `Person` role and orientation fields (`person_wire.cpp`);
    - `MeshBitstreamMessage.{origin_offset, is_compressed}` (`libs/transport/mesh_bitstream/src/stream.cpp:35-36,68`);
    - `target_origin` (`sqe_marker_camera_op.cpp:459`, `sqe_tracker_op.cpp:296`, `pose_publisher_op.cpp:229`) and
      `Target.quality` (`sqe_marker_camera_op.cpp:470`);
    - also `operators/transport/pose_publish/src/pose_wire.cpp:49,114-115`,
      `operators/transport/zenoh_io/src/zenoh_cloud_publisher_op.cpp:151`, `libs/transport/zenoh/src/calibration.cpp:77`;
    - the eye-tracking pass-through (`tcnart_mirror.cpp:127`) keeps `scene_point`/`gaze_space`, now optional: set
      them only for samples that have them (the Tobii recordings).
  - FlatBuffers mirrors and the enum renames (media types, parameter RPC). Set enum fields explicitly where the old
    default was relied on (above).
- *Plan 5:* (counts are approximate, outside `third_party/` and `docs/history`)
  - **Naming code goes** (about 320 uses of marker, domain and `:::` naming in 31 files): in
    `libs/transport/sqe_link/src/sqe_link.cpp` the domain name helpers around `domain_node_id`/`parse_domain_node_id`
    (lines 172-193, 1018) and the frame text; in `operators/sqe/sqe_camera/src/sqe_marker_camera_op.cpp:128` the
    `tcn::sqe::marker_node_name(kind, id, marker_system_part(system))`. Domain nodes carry `SRNode.domain` and a bare
    name; marker nodes are found by `SISGraphFilter` and named by the engine. **Announce markers by id**
    (`{client}/cfg/marker/{device}/{marker_id}`).
  - **FrameRef text in configs:** every YAML or Python spelling of a frame (`@domain::name`, `<device>:::*`, a bare
    name; see `python/tests/test_marker_sqe_graph.py`, `operators/sqe/sqe_camera/tests/test_sqe_camera.cpp`) becomes
    `local:…`, `global:…`, `domain:…` (+ `[instances]`), also in the Python bindings (`python/sqe_bindings.cpp`).
    Run the vector file in the `sqe_link` tests.
  - **Capabilities instead of `protocol_break`:** `engine_protocol_break()`, `known_engine_protocol_break()`
    (`sqe_link.hpp:219-227,348`) and the Python binding (`sqe_bindings.cpp:257-277,494-502`) become
    `Capabilities`/`has(...)` and `meets_floor()` (about 90 uses in 16 files, mostly `sqe_link` and `sqe_camera`
    tests).
  - **Output type:** whatever subscribes to a relation's data topic decodes `SISRelationOutput` on the `data_topic`
    given by the start reply, and takes `status_topic`/`plan_topic` from the reply instead of composing them. The
    13-byte attachment is gone: `tests/fake_sqe_engine.hpp`, `operators/transport/pose_publish/include/tcn/operators/pose_wire.hpp`,
    `docs/developers/reference/operators.md`.
  - **Status and enums:** about 170 old enumerator uses in 18 files (`libs/transport/sqe_link/tests/test_sqe_link.cpp`
    37, `src/sqe_link.cpp` 17, `operators/sqe/sqe_state`, `sqe_camera`, `python/sqe_bindings.cpp`); the reply status
    is `header.status`.
  - **Mutations and queries:** node and edge keys (`/sis/node|edge/*`), `SISGraphQueryRequest` as a union,
    `SISTargetAlias` family/variant, `SRGraph.name` (15 uses), `stream_item_*` → `entry_id` (6).
  - **Rational lens and marker spaces:** `docs/migration/requests/2026-11-27-sqe-rational-lens-and-marker-spaces.md`
    is answered by the `rational_lens` and `marker_domain` capabilities: gate on them.

### tcn web controller

- *Plan 3:*
  - about 35 moved type names in about 15 files (`schemas.ts`, registries): `sqe_msgs::rpc` (23), `pcpd_msgs::rpc` (7), `tcn_types::msg` (6);
  - `Pose6DMapMessage` and `RigidTransformMap(Item)` are deleted; the fallback is already gone on `srg-dead-fields`.
- *Plan 2:* nothing to do. The type names in `types.json` are unchanged. ROS types are now `"extensibility": "final"` in `types.json` (XCDR1, as before).
- *Plan 1:* optional:
  - check `@tcn-web/cdr` against `test/golden/*.hex`;
  - start generating `packages/sqe/src/schemas.ts` from `types.json` and `enums.json` (O7). The fields you asked for (member order, extensibility, `@optional`, bounds, union labels, fully qualified names) are in `types.json`.
- *Plan 4:*
  - **`packages/cdr`:**
    - XCDR2 reader and writer (verified feasible);
    - `reader.ts:14` and the tests expect `00 01` today;
    - keep XCDR1 for ROS types.
  - **Removed fields:**
    - read: `Target.quality` (`apps/mcp/src/tools/srg.ts:487`), `target_origin` (done on `srg-dead-fields`);
    - **layout mirrors:** `packages/tcn-zenoh/src/streams/messages.ts:34-35,74,90,103-104,130,144,146`
      (`AudioAACProfile`, `BufferInfo.is_fixed_size`, the StreamDescriptor fields, `target_origin`,
      `MeshBitstream.{origin_offset, is_compressed}`) and `rig/messages.ts:35-66` (`CameraSensorTypesEnum`,
      `camera_type`, `raw_calibration`, `timestamp_offset_ns`, `DeviceContextReply.{is_valid, timestamp_offset}`).
  - **Enum names and values** in `schemas.ts` (media types, `CameraModelType`, `CoordinateAxis`,
    `RPCResponseStatus`, the parameter RPC enums).
- *Plan 5:* (counts are approximate)
  - **`packages/sqe/src/schemas.ts`** (the layout mirrors; generate it from `types.json` and `enums.json` now that
    the SQE is stable) for every reshaped type and every enum value of §1: `SRNode`, `SREdge` (`kind`, `timing`,
    optional `transform`, `entry_id`, `calibration`), `SRGraph` (no `name`), the views (`fragment`), `SRTransform`
    without the `_COV` cases, `SRPoint2D` (16 bytes), `PinholeIntrinsics` (`SRDistortionModel` enum, 16
    coefficients, optional `metric_radius`, `image_width`, `image_height`), every reply with `SISReplyHeader`
    first, `SISRelationTerms`, `SISRelationStatus`, `SISStreamHandle`, `SISRelationOutput`, the graph and target
    query unions, `SISExplainRequest.subject`, `SISGraphChangeNotification` (`header`, `origin`). All enumerators
    take the new names and values (about 415 enumerator mentions in 39 `.ts`/`.tsx`/`.json` files, among them
    `schemas.ts`, `model.ts`, `client.ts`, `apps/mcp/src/tools/srg.ts`, `SrgInspector.tsx`, the tests); `0` is `UNKNOWN`. `client.ts:447` builds a graph query with
    `name_prefix`, `temporal_type`, `scope`, `node_type` and `"SIS_PP_WALKABLE"`: all of that changes.
  - **Graph filters (G3) instead of name prefixes:** `SISGraphFilter { timing[], edge_kind[], materialized, derived,
    marker_type, marker_system, marker_domain }` replaces name-prefix filtering: `model.ts:17` (`isDomainId`, `@…::`),
    `edit.ts:110` (the `sqe.derived:` prefix: use `SREdge.derived`) and the marker checks in the SRG panels.
  - **`SRRelationRef` in views and paths:** an edge is `(source, target, name)`, shown in the inspector and in a
    graph path (`SISGraphQueryReply.path`); `SISEdgeRemoveRequest` carries the ref. Show frame refs with the N2a
    text form and run the vector file in `packages/sqe` tests.
  - **Absent `transform`:** an `STATIC_UNCALIBRATED` or `PROJECTION` edge may have **no** `transform` (and a
    calibration may have no `residual` or `covariance`); the editor and the scene panel must not read an absent
    transform as identity (`SrgScenePanel.tsx`, `calibration.ts`, `explain.ts`).
  - **Keys:** `packages/sqe/src/keys.ts:31` sample key → `{client}/sis/stream/sample`; mutation keys in `client.ts`
    (`/sis/node|edge/*`); new `sqe/{engine}/info`; `OperatorKey` is unchanged. `keys.ts:47` (`client === scope ||
    client.startsWith(scope + "/")`) already follows the chunk-boundary rule; an empty scope contains every client.
  - **Handles:** read `status_topic` and `plan_topic` from the start reply; do not compose them.
  - **`packages/cdr`/goldens:** `packages/sqe/test/golden/*.cdr` and `.json` (47 files, `SISGraphQueryReply`,
    `SISExplainReply`, `SISMutationReply`, `SISEngineInfoReply`, …) regenerate against `test/golden/*.hex` and the
    new types; `Pose6DMapMessage.*` are already stale (Plan 3).

### IVA (realtime conversational pipeline)

- *Plan 3:*
  - about 24 moved type names in about 10 files, almost all the parameter RPC in `tcn-wire` (`tcnart_msgs::rpc::*` → `pcpd_msgs::rpc::*`, 23), plus `tcn_types::msg` (1);
  - the IDL sync takes `pcpd_msgs/rpc/Parameters.idl` instead of `tcnart_msgs/rpc/Requests.idl`, and the PascalCase `conversation_msgs` files.
- *Plan 2:* when you next sync `idl/` from tcn_schema:
  - the closure has `builtin_interfaces/msg/Time.idl` instead of `Time_.idl`;
  - update `crates/tcn-wire/tests/alignment.rs` (lines 36 and 85), which name `Time_.idl`.
- *Plan 1:* optional:
  - check the `tcn-wire` mirrors against `types.json`;
  - the parameter types in `tcn-wire` are not affected by the C++ `operator==` defect.
- *Plan 4:*
  - **`tcn-wire`:**
    - write and check `00 09 00 00` and DHEADERs (`crates/tcn-wire/src/encap.rs:8` has `00 01`);
    - `conversation_msgs` now carry the encapsulation header too (E4; `crates/common-types/src/wire.rs:6` says plain
      CDR without one).
  - **Enum renames with value changes:** `MemoryScopeKind`, `InteractionMode`, `RetrievalPolicy`,
    `ExecutionPreference` and `InteractionActivityState` gain `…_UNKNOWN` at 0, so every value moves up by one. Use
    the names, and update any stored or JSON numbers.
  - `ErrorCode` → `ERROR_CODE_*` (same values).
  - **The parameter RPC enums shift too:** `RPCResponseStatus`, `ParameterValueType`, `ParameterAccess` and
    `ParameterSetStatus` gain `…_UNKNOWN` at 0. `crates/tcn-wire/src/msgs.rs:40-59` has `#[default] Success = 0` and
    explicit discriminants: take the new values from the enum map, and set `status` explicitly (an unset one now
    reads `UNKNOWN`).
  - **Removed F5 fields:** `TtsRequest` voice settings, `LlmRequest.{system_prompt, low_latency_mode}`,
    `RetrievalOptions` domain lists, `SttPartial.stability`, `StageMetric.gpu_utilization_pct`,
    `AudioOpusFrame.packet_loss_concealment_applied`, and `shm_ref`/`payload_kind` with `ShmRef`/`PayloadKind` (A2).
- *Plan 5:* nothing. IVA does not use the SQE protocol, and `conversation_msgs` and the parameter RPC are not touched
  by this plan. (Its one `tcn_types` name from Plan 3 is not in the reshaped list of the
  [sqe type map](0.11-sqe-type-map.md); check it there.)

## 3. Coming, decided (not yet on `main`)

Summary of what the later plans bring, so teams can plan their work. The details are in the decisions and will be
written into §1 and §2 as each plan lands. Plan 5 (the #35 SQE redesign) is written up above and is in review; what
it leaves for Plan 6 is the descriptor split below.

- **Plan 6:**
  - the #36, #37 and #38 changes land;
  - **#35 S1 and N6** (left out of Plan 5): `StreamDescriptorMessage` becomes a common header plus a union per sensor
    kind, and the tracker part gets its own `marker_system` and `marker_domain` (until then `marker_system` is
    `{system}@{domain}`, see Plan 5);
  - HumanPose and the joint-set table, `StreamHeader` (with `PersonTrackingMessage.up`/`world_frame`);
  - the final engine B-number for the 0.11 break (`kProtocolBreak` is provisionally 99);
  - `0.11.0-rc1`.
