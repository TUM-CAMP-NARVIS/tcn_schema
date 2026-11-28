# Migration to tcn_schema 0.11.0 (running log)

**Status: `0.11.0-rc1`, the release candidate.** This is the log of every 0.11 change that a consumer has to follow.
It is updated with each 0.11 pull request, so it is always current on `main`. With Plan 6 every decided 0.11 change
is in: consumers regenerate from rc1 and work through their checklist in §2. When `0.11.0` is released, this log
becomes that release's migration note.

- **What was decided and why:** [docs/proposals/0.11-decisions.md](../proposals/0.11-decisions.md). Where this log and
  the decisions differ, the decisions win; please report the difference.
- **Order of work:** six plans (Plan 4 was split, see its section), each one or more PRs (decisions §10). Plan 1's plan file is
  [docs/superpowers/plans/2026-11-27-0.11-plan-1-layout-and-tooling.md](../superpowers/plans/2026-11-27-0.11-plan-1-layout-and-tooling.md).

| Plan | Content | Wire change | Status |
|---|---|---|---|
| 1 | Layout and tooling: docs, out-of-tree generation, package table, `types.json`/`enums.json`, lint, round-trip tests | **none** | merged (#41) |
| 2 | ROS 2 tree regenerated from Jazzy, `@final`, service/action types, `ros2_types.json`, tiers | ROS types only (`Range.variance`) | merged (#42) |
| 3 | Moves and deletions: `sqe_msgs`, `tcn_types`, parameter RPC → `pcpd_msgs`, `custom/` → `tcn/`, deleted types | type names, removals | merged (#43) |
| 4 | Encoding (XCDR2, `tcn_schema=0.11`), enum renames outside `sqe_msgs`, field removals F1, F2, F5 | **yes** | merged (#44) |
| 5 | The #35 SQE redesign, with F3/F4 and the `sqe_msgs` enums | **yes** | merged (#45) |
| 6 | #36, #37, #38 changes, the typed stream descriptor (#35 S1, N6), HumanPose and joint sets; `0.11.0-rc1` | **yes** | in review (PR for branch `0.11-plan6-rc1`) |

**How to use this log.** Find your checklist in §2 and work through it in order. Each item names the plan that
brought it; the details are in that plan's part of §1. Items marked *(wire)* need the lockstep cut-over; all others
can be done ahead of it. §3 lists what is decided for 0.11.x.

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
- **Not in this plan:** `PersonTrackingMessage.{up, world_frame}` move into `StreamHeader` with #38 (done in Plan 6).
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
  write `…CdrAux` files for enum-only IDL (`tcnart_msgs/msg/MediaTypes.idl`). The fix (with an `install_smoke` test)
  is part of Plan 4 (PR #44) and is merged into this branch, so a Plan 5 tree installs as a whole.

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
| `SISPlanInput` | `topic` ("" = unbound), `schema`, `device_coord`, `entry_bound`, `entry_id`, `sensor_latency_ns`, `frame_rate` (uint32), `clock_domain` (string), `source`, `buffer_client`, `device`, `target` | `? topic` (absent when unbound), `schema`, `device_coord`, `? entry_id`, `sensor_latency_ns`, `? frame_rate` (double), `clock_domain` (`ClockDomain`), `source`, `edge` (`SRRelationRef`; replaces the three strings) |
| `SISPlanHop` | `instance`, `from`, `to`, `edge_name`, `temporal_type`, `derived`, `walked_inverted`, `weight`, `derived_from` (strings), `input` (int32, -1 = none), `covariance` (0 or 21) | `instance` (unchanged), `edge` (`SRRelationRef`), `kind`, `timing`, `derived`, `walked_inverted`, `weight`, `derived_from` (`SRRelationRef`s), `? input`, `? SRCovariance6 covariance` |
| `SISPlanOp` (union) | `PROJECT` case: `SRIntrinsics` | `CameraIntrinsics` |
| `SISTranslationPlan` | `plan_version`, `output_topic`, `route_epoch` (uint32), `engine_epoch`, `target_coord`, `instances`, `inputs`, `reference_input` (int32, -1 = none), `correlated_via` (strings), `clock_domain` (string), `path_weight`, `hops`, `route_notes` | `? route_epoch` (uint64; absent for a dry run), `target_coord`, `instances`, `inputs`, `? reference_input`, `correlated_via` (`SRRelationRef`s), `clock_domain` (`ClockDomain`), `path_weight`, `hops`, `route_notes`. Removed: `plan_version`, `output_topic`, `engine_epoch` |
| `SISRelationSampleRequest` | `observer`, `target`, `freshness_bias`, `instant`, `stamp_ns`, `wait_ms`, `include_plan` | `terms`, `instant`, `? stamp_ns` (present exactly when `instant` is AT), `? wait_ms`, `include_plan` |
| `SISRelationSampleReply` | `status` (`SISRelationSampleStatus`), `detail`, `engine_id`, `engine_epoch`, `relation`, `relation_detail`, `blocked_by`, `stamp_ns`, `clock_domain` (string), … | `header`, `relation` (`SISRelationStatus`), **`outcome`** (`SISSampleOutcome`), `? stamp_ns`, `clock_domain` (`ClockDomain`), `targets`, `inputs`, `cached`, `waited_ms`, `? plan` |

**Queries, operator surface, targets.**

| Type | 0.10 | 0.11 |
|---|---|---|
| `SISGraphQueryRequest` | `kind`, `client`, `from`, `to`, `max_items`, `cursor`, `temporal_type[]`, `scope[]`, `node_type[]`, `name_prefix`, `predicate`, `radius` (kind-dependent fields "ignored, not refused") | `query: SISGraphQuery` (union: `FRAGMENT` string, `NEIGHBOURS` `SRFrameRef`, `SEARCH` `SISGraphSearch { anchor, filter }`, `PATH` `SISGraphPath { from, to, predicate, ? radius }`; absent radius = the engine's default), `cursor` (still a string: `""` is the first page, a true reading, not a sentinel). Removed: `node_type`, `name_prefix`, `scope`, `max_items` |
| `SISGraphFilter` (new, G3) | name prefixes | `timing[]`, `edge_kind[]`, `? materialized`, `? derived`, `? marker_type`, `? marker_system`, `? marker_domain` |
| `SISGraphQueryReply` | `status`, `nodes`, `edges`, `next_cursor`, `detail`, `path` (strings), `radius_exhausted` | `header`, `nodes`, `edges`, `? next_cursor`, `path` (`SRRelationRef`s), `radius_exhausted`, `? fragment` (`SISFragmentSummary`, for a FRAGMENT read) |
| `SISFragmentSummary` | `client`, `origin`, `joined`, `last_updated_ms`, `node_count`, `relation_count` | `fragment`, `origin`, `joined`, `last_updated_ms`, `node_count`, `edge_count`, `display_name`, `description` |
| `SISPatternSegment` | `relation`, `from`, `to`, `inverted`, `dynamic`, `stream_topic`, `stream_item_id`, `transform`, `computed`, `sensor_latency_ns`, `frame_rate`, `stream_item_bound` | `edge` (`SRRelationRef`), `dynamic`, `? stream_topic`, `? entry_id`, `? transform`, `computed`, `? sensor_latency_ns`, `? frame_rate`. Removed: `inverted` |
| `SISPatternInput` | `role_from`, `role_to`, `segments`, `observer`, `target` (`SISFrameRef`), `frame_rate` | `segments`, `observer`, `target` (`SRFrameRef`), `? frame_rate`. Removed: `role_from`, `role_to` |
| `SISPatternInstance` | …, `min_measurements`, `max_measurements`, `cost`, … | `max_measurements` removed |
| `SISPatternRole` | `frame`: `SISFrameRef` | `frame`: `SRFrameRef` |
| `SISPatternQueryRequest` | `from`, `to`, `wanted`, `radius` (0 = the engine's default), `only_patterns`, `max_candidates` | `from`, `to` (`SRFrameRef`), `wanted`, `? radius` (absent = the engine's default; a present 0 is zero hops). Removed: `only_patterns`, `max_candidates` |
| `SISPatternQueryReply` | `status`, `instances`, `detail` | `header`, `instances` |
| `SISFragmentListRequest` | `who`, `prefix`, `max_items`, `cursor` | `max_items` removed |
| `SISFragmentListReply` | `status`, `fragments`, `next_cursor`, `detail` | `header`, `fragments`, `? next_cursor` |
| `SISGraphChangeNotification` | `sequence_number`, `engine_epoch`, `kind`, `client`, `name`, `origin` (string) | `header` (`engine_id`, `engine_epoch`), `sequence_number`, `kind`, `fragment`, `? node` (`SRFrameRef`), `? edge` (`SRRelationRef`), `origin: SISChangeOrigin { kind: CLIENT/OPERATOR/ENGINE, id }` |
| `SISOperatorMutationRequest` | `who`, `client`, `mutation` | `who`, `fragment`, `mutation` |
| `SISExplainRequest` | `who`, `kind`, `observer`, `target`, `trigger`, `freshness_bias`, `handle` | `who`, `subject: SISExplainSubject` (union: `RELATION` `SISRelationTerms`, `STREAM` handle string) |
| `SISExplainReply` | `status`, `detail`, `relation`, `relation_detail`, `blocked_by`, `plan`, `live` | `header`, `relation` (`SISRelationStatus`), `? plan`, `? live` |
| `SISTransactionLogReply` | `status`, `detail`, `active`, `label`, `file`, `started_ns` (0 = no log), `records`, `dropped` | `header`, `active`, `label`, `file`, `? started_ns` (absent = no log), `records`, `dropped` |
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
| a node, edge or remove request naming a LOCAL frame whose `owner` is not the key's client (an empty owner included; it is accepted only inside a join) | `REJECTED` + `NOT_PERMITTED` | – |
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
  `share/tcn_schema/sqe/frame_ref.tsv`. It has 70 rows (16 `ok`, 54 `err`). **An implementation that does not pass
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
  `>= kProtocolBreak` (`TCN_SQE_PROTOCOL_BREAK`, **99**, provisional: set at 0.11.0 to the engine's B-number). Below it the
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
- **All four strings of `SISStreamHandle` (`handle`, `data_topic`, `status_topic`, `plan_topic`) are always named and
  non-empty, whatever the evaluation mode.** A topic nothing is published on is still named: `data_topic` of a LOCAL
  stream, `plan_topic` of an ENGINE stream. An empty one is a protocol error, not "this mode has no such topic"; the
  contract library refuses it (`EmptyHandleTopic`).
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
union: **superseded by "The stream descriptor" in Plan 6** (`TrackerStreamInfo`, no `{system}@{domain}`).

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
satisfies the spec. Each needs an answer from the SQE side before `0.11.0` (Plan 6 adds four more).

1. **`SRGraph.name` is removed**; `display_name` serves (N9).
2. **`SRMaterialization` uses `marker_domain` and an `SRFrameRef` template device** (N5). `SRNodeRef` was deleted in
   Plan 3.
3. **`SISPlanInput` names its edge by `SRRelationRef`** (it had `buffer_client`, `device` and `target`).
4. **`covariance_hops` and `total_hops` are per `SISRelationOutput`**, not per target. With an `[instances]` selector
   the instances may differ in covariance completeness; moving them per target is a reshape, so decide before 0.11.0.
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
12. **More P3 optionals:** `SISTransactionLogReply.started_ns` (absent = no log), `SISPlanInput.topic` (absent when
    unbound), `SISGraphPath.radius` and `SISPatternQueryRequest.radius` (absent = the engine's default; a present 0 is
    zero hops). The two `cursor` members stay strings: `""` is the first page, a true reading, not a sentinel.
13. **Whose LOCAL frames a mutation may name** (`Mutations.idl`): in `/sis/node/*`, `/sis/edge/*` and the remove
    requests, a LOCAL `SRFrameRef`'s `owner` must equal the key's client, otherwise `REJECTED` + `NOT_PERMITTED`. An
    empty LOCAL owner is accepted only inside a join (where it means this fragment); elsewhere it is refused the same
    way.
14. **A sample's `evaluation` and `fusion`** (`Sample.idl`): validated like a start's (`UNKNOWN` is `MALFORMED`,
    `FUSED`/`FILTERED` `UNSUPPORTED`). ENGINE, LOCAL and BOTH are all accepted and the engine evaluates the sample in
    every mode; a client that wants the route asks with `include_plan`.

#### Tools

- `tools/idl_cli.py moves --old <plan 4 src/dds> --new src/dds --spec tools/idl/sqe_0_11.json --plan "plan 5"
  --json src/dds/tcn/type_map_0_11_sqe.json --markdown docs/migration/0.11-sqe-type-map.md` proves that nothing
  else changed (new, moved, reshaped, deleted and removed members are all listed in the spec).
- `tools/idl_cli.py enums` applied `enum_renames_0_11_sqe.json`; the lint baseline is now empty.
- `tools/idl/tests/test_contract_enums.py` pins every C++ enum mirror to the IDL (UNKNOWN first, `std::int32_t`) and
  every wire-registered and request-keyed type to the schema.

### Plan 6: streams, persons, calibration, parameters, voice; `0.11.0-rc1` (wire change)

The last plan of 0.11. It places the #36 (IVA), #37 (tcnart) and #38 (HTA) decisions, the two #35 items Plan 5 left
out (S1, the typed stream descriptor; N6, `marker_domain` on it), HumanPose with the joint-set table, and it cuts the
release candidate. Read it with [decisions §7](../proposals/0.11-decisions.md) and the proposals it cites. The ids
below carry their issue: #38 A1 is HTA's, #37 T8 tcnart's, #36 S4 the voice pipeline's.

**The cut.**

| In Plan 6 | Left for 0.11.x (§3) |
|---|---|
| `StreamHeader` on the ten stream messages (#38 A1); the typed descriptor (#35 S1, N6; #38 A2, A7; #37 T7, T8, T10); HumanPose, HumanShape and the joint-set table (decisions §7); `Target.pose` optional (#37 T19) and a per-target capture time (#38 A5) | the surfel, eye, hand and IMU descriptor fields (their cases are empty structs now); `XRViewInfo` with T4's `FOV_ANGLES` |
| `CameraSensor` with `CameraIntrinsics`, `revision`, provenance and axes (#38 A3, A4); `CameraModel`/`CameraModelType` deleted | the typed calibration RPC (#38 A4) |
| the parameter `ENUM` case (#37 T13); `StreamRole`/`StreamMode` and `ProcessInfo.started_at` as a `Time` (T14); presence `rpc_root` and `service_type` (T15) | |
| `conversation_msgs` per #36: S1 (no echo), S3, S4 (`PlaybackEvent`), S5 (reasons), S6 (barge-in modes), A1 (`VadEvent`), F1/F2 (turn timeline), F3 (face layouts), T1 (`TtsDone` length), D1/D4 (capabilities), the `state`/`has_*` removals, M1 (`component_name`) | F4 `FaceAnimationChunk` and F5; per-session voice and language overrides |

**Already done in earlier plans, not redone here:**
- *Plan 4:* the encapsulation header on `conversation_msgs` (E4), the UNKNOWN-first conversation enums, the #36 field
  removals (A2, L3, L5, T2), the optional `EyeTrackingMessage.gaze_space` and `scene_point`.
- *Plan 5:* `SRFrameRef`, `ClockDomain`, `CameraIntrinsics`/`PinholeIntrinsics`, `SRCovariance6`, `SISTargetInfo`,
  the optional `pose_covariance` of `Target` and `Pose6DMessage`, and the graph side of N6
  (`SRMaterialization.marker_domain`, `SISGraphFilter.marker_domain`).

**The release candidate.**

| What | Value |
|---|---|
| CMake project version | `0.11.0` |
| conan package version | `0.11.0-rc1` |
| `schema_version` in `share/tcn_schema/types.json` and `enums.json` | `"0.11.0-rc1"` |
| Wire annotation | unchanged: `application/cdr;<type>;tcn_schema=0.11` |
| `tcn::sqe::kProtocolBreak` (`TCN_SQE_PROTOCOL_BREAK`, `src/sqe/include/tcn/sqe/protocol.h`) | **99, provisional**: set at 0.11.0 to the engine's B-number for the break |

**Map, generated by `tools/idl_cli.py` (every row is checked against the IDL):**

| Map | Content | Source |
|---|---|---|
| **[0.11-streams-type-map.md](0.11-streams-type-map.md)**, installed as `share/tcn_schema/type_map_0_11_streams.json` (`new`, `moved`, `relocated`, `reshaped`, `deleted`) | 33 new types, 36 reshaped, 18 deleted (8 types and 10 constants); nothing moved | `tools/idl/streams_0_11.json` |

- **Apply the three maps in order:** [0.11-type-map.md](0.11-type-map.md) (plan 3), then
  [0.11-sqe-type-map.md](0.11-sqe-type-map.md) (plan 5), then the streams map (plan 6).
- **There is no generated enum map for Plan 6:** its enums were replaced or reshaped, not renamed. The old → new
  value tables are below.

#### Types: new, reshaped and deleted

| | Types |
|---|---|
| **New, `tcn_types::msg`** | `StreamHeader` (`Stream.idl`, new file); `CalibrationProvenance` (`Camera.idl`) |
| **New, `tcnart_msgs::msg`** | `StreamKind`, `PixelSemantics`, `VideoCodec`, `MeshEncoding` (`MediaTypes.idl`); `StreamInfo` (union), `VideoStreamInfo`, `TrackerStreamInfo`, `MeshStreamInfo`, `SurfelStreamInfo`, `EyeStreamInfo`, `HandStreamInfo`, `ImuStreamInfo` (`StreamDescriptor.idl`); `JointSet` (`JointSet.idl`, new file, generated); `PersonState`, `JointSource`, `SmplModel`, `SmplGender`, `PersonSmoothing`, `SmplParams`, `HumanShape`, `HumanShapeMessage`, `PersonStreamInfo` (`HumanPose.idl`); `PersonTrackState` (`PersonTracking.idl`) |
| **New, `pcpd_msgs::rpc`** | `StreamRole`, `StreamMode` (`ServiceController.idl`) |
| **New, `conversation_msgs`** | `msg::BargeInMode`, `msg::SessionStateReason` (`Common.idl`); `msg::PlaybackState`, `msg::PlaybackEvent` (`Control.idl`); `msg::FaceRig`, `msg::FaceAnimationLayout` (`Face.idl`, new file, header `conversation_msgs/msg/Face.h`) |
| **Deleted types** | `tcnart_msgs::msg::SensorType`, `VideoImageCompression`, `BufferInfo`, `CameraModel`, `CameraModelType` (the file `CameraModel.idl` is gone); `conversation_msgs::msg::EventState`, `SessionConfigUpdate`, `SttRequest` |
| **Deleted constants** | `PERSON_STATE_TENTATIVE`, `PERSON_STATE_CONFIRMED`, `PERSON_STATE_PREDICTED` (`PersonTracking.idl`); `HUMAN_POSE_TRACKED`, `HUMAN_POSE_COASTING`, `HUMAN_POSE_LOST`, `HUMAN_JOINT_OBSERVED`, `HUMAN_JOINT_DERIVED`, `HUMAN_JOINT_DEFAULTED`, `HUMAN_JOINT_HAS_ROTATION` (`HumanPose.idl`) |
| **Reshaped** | the 36 types of the map; their fields are below |

- **A name that now means something else:** `PERSON_STATE_*` are now the enumerators of HumanPose's `PersonState`
  (`TRACKED`, `COASTING`, `LOST`), not 0.10's track-state constants. Those are `PersonTrackState`.
- **`pcpd_msgs` no longer depends on `tcnart_msgs`:** its dependencies are `builtin_interfaces` and `tcn_types`
  (`cmake/tcn_packages.json`). A consumer that got `tcnart_msgs` through `pcpd_msgs` now links
  `tcn_schema::tcnart_msgs` itself.

#### `StreamHeader` (#38 A1)

`tcn_types::msg::StreamHeader` replaces `std_msgs/Header` as the first member of exactly these ten messages:
`tcnart_msgs::msg::VideoStreamMessage`, `Pose6DMessage`, `TargetTrackingMessage`, `EyeTrackingMessage`,
`HandTrackingMessage`, `ImuMessage`, `PersonTrackingMessage`, `HumanPoseMessage`, `HumanShapeMessage`, and
`pcpd_msgs::msg::MeshBitstreamMessage`. `TcnartPresenceMessage` keeps `std_msgs/Header` (its `frame_id` is unused).

| Member | Meaning | Replaces |
|---|---|---|
| `int64 capture_time_ns` | when the sample was captured, ns in `clock`'s time base; for the deployment clock, since the Unix epoch | `stamp` (`sec`·10⁹ + `nanosec`) |
| `? int64 device_time_ns` | the device's own counter, arbitrary epoch; absent when the device has none | – |
| `uint64 sequence_number` | per stream, gapless, from 1 | – |
| `? uint64 group_id` | one capture group across streams (video, mesh, persons of one capture); a `HumanPoseMessage` matches its `PersonTrackingMessage` by it; absent: not grouped | `PersonTrackingMessage.group` |
| `? ClockDomain clock` | absent: **the stream descriptor's `clock`**, never the deployment clock | the descriptor's `clock_domain` string |
| `SRFrameRef frame` | the frame the payload is expressed in | `frame_id` |

- **Absent `clock` is not the deployment clock.** A reader that falls back to the deployment clock composes stamps
  from different clocks.
- **`SISRelationOutput`** (Plan 5) embeds a `TargetTrackingMessage`: its outer `clock_domain` and `sequence_number`
  win. The engine leaves `targets.header.clock` absent and sets `targets.header.sequence_number` to the outer one.
- **Encoding:** `StreamHeader` is `@appendable` (DHEADER), so the fixed prefix of every stream message changed.
  Golden: `tcn_types_StreamHeader.hex` (presence bytes of `device_time_ns`, `group_id`, `clock`).

#### Fields: reshaped types

`?` means `@optional` (absent is explicit; there is no sentinel). A type listed here must be re-decoded as a whole.

**Streams** (`tcnart_msgs/msg/*`, `tcn_types/msg/Tracking.idl`, `pcpd_msgs/msg/MeshBitstream.idl`).

| Type | Plan 5 | rc1 |
|---|---|---|
| `Target` | `target_id`, `pose`, `is_tracked`, `? pose_covariance` | `target_id`, **`? pose`** (absent: no pose, never identity; #37 T19), `is_tracked`, `? pose_covariance`, **`? int64 capture_time_ns`** (absent: the message's `header.capture_time_ns`; #38 A5) |
| `VideoStreamMessage` | `std_msgs/Header header`, `pose`, `camera_focal_length[2]`, `camera_principal_point[2]`, `camera_radial_distortion[3]`, `camera_tangential_distortion[2]`, `image_bytes`, `image` | `StreamHeader header`, `pose`, **`? CameraIntrinsics intrinsics`** (absent: the descriptor's nominal intrinsics, never zeros; #37 T8), `image_bytes`, `image` (still last) |
| `Pose6DMessage`, `TargetTrackingMessage`, `EyeTrackingMessage`, `ImuMessage`, `MeshBitstreamMessage` | `std_msgs/Header header`, … | `StreamHeader header`, the rest unchanged (comments now name `header.frame` and `header.capture_time_ns`) |
| `HandTrackingMessage` | `header`, …, `boolean gaze_valid`, `Ray gaze` | `StreamHeader header`, …, **`? Ray gaze`** (absent: no gaze this sample) |

The per-frame camera arrays of `VideoStreamMessage` go into the `pinhole` case of `intrinsics`:

| 0.10 / Plan 5 | rc1 `intrinsics.pinhole` |
|---|---|
| `camera_focal_length[0]`, `[1]` | `fx`, `fy` |
| `camera_principal_point[0]`, `[1]` | `cx`, `cy` |
| `camera_radial_distortion[0..2]` (k1 k2 k3), `camera_tangential_distortion[0..1]` (p1 p2) | `distortion_model` `SR_DISTORTION_MODEL_OPENCV_RADTAN`, `distortion` in OpenCV order: k1 k2 p1 p2 k3 |
| all zero ("unknown") | `intrinsics` absent (the descriptor's `info.video.intrinsics` hold) |

**Persons** (`tcnart_msgs/msg/PersonTracking.idl`, `HumanPose.idl`; decisions §7).

| Type | Plan 5 | rc1 |
|---|---|---|
| `PersonTrackingMessage` | `std_msgs/Header header`, `uint64 group`, `string<64> joint_set`, `string<128> world_frame`, `Vector3 up`, `camera_count`, `persons`, `camera_reprojection_px` | `StreamHeader header` (`group` → `header.group_id`, `world_frame` → `header.frame`), **`JointSet joint_set`**, **`uint16 joint_set_version`**, `camera_count`, `persons`, `camera_reprojection_px`. `up` → the descriptor's `PersonStreamInfo.up` |
| `Person` | …, `uint32 previous_id` (0 = none), `uint8 state` (`PERSON_STATE_*`), … | …, **`? uint32 previous_id`** (absent: continues none), **`PersonTrackState state`** (values +1, below), … |
| `HumanPoseMessage` | `std_msgs/Header header`, `string<64> joint_set`, `string<64> source`, `sequence<HumanPose,16> persons` | `StreamHeader header`, **`JointSet joint_set`**, **`uint16 joint_set_version`**, `sequence<HumanPose,32> persons`. `source` removed |
| `HumanPose` | `person`, `string<32> participant`, `uint8 state` (`HUMAN_POSE_*`), `source_ids`, `sequence<HumanJoint,32> joints`, `sequence<float,32> bone_lengths` | `person`, **`? string<128> member`** (was `participant`; absent: anonymous), **`PersonState state`**, **`float confidence`**, `source_ids`, `sequence<HumanJoint,64> joints`, **`? SmplParams smpl`**. `bone_lengths` moved to `HumanShape` |
| `HumanJoint` | `position`, `Quaternion rotation`, `confidence`, `uint8 flags` (`HUMAN_JOINT_*` bits) | `position`, `confidence`, **`JointSource source`**, **`? Quaternion rotation`** (was flag `HUMAN_JOINT_HAS_ROTATION`; absent, never identity) |
| `HumanShapeMessage` (new, latched) | – | `StreamHeader header`, `sequence<HumanShape,32> persons` |
| `HumanShape` (new) | – | `person`, `joint_set`, `joint_set_version`, `sequence<float,64> bone_lengths` (m; NaN while not frozen), `? SmplModel smpl_model`, `? SmplGender smpl_gender`, `sequence<float,300> smpl_betas` |
| `SmplParams` (new) | – | `SmplModel model`, `global_orient`, `translation`, `sequence<Vector3,54> pose` (parent-relative axis-angle), `sequence<float,100> expression` |
| `PersonStreamInfo` (new, the descriptor's `PERSONS` case) | the descriptor properties `joint_set`, `up`, `smoothing` | `JointSet joint_set`, `uint16 joint_set_version`, `Vector3 up`, `float coast_limit_s`, `float reid_window_s`, `PersonSmoothing smoothing` |

- **One joint set per stream:** every message (and every `HumanShape`) of a person stream carries exactly the
  descriptor's `PersonStreamInfo.joint_set` and `joint_set_version`.
- **`bone_lengths` NaN** is the documented exception to the `@optional` rule: a bone that is not frozen yet.
- The 0.10 `joint_set` strings: `coco17` → `JOINT_SET_COCO17` v1, `halpe26` → `JOINT_SET_HALPE26` v1,
  `tcnart_body_v1` → `JOINT_SET_TCN_BODY` v1 (`JointSets.md`, "Retired names").

**The stream descriptor** (`tcnart_msgs/msg/StreamDescriptor.idl`; #35 S1, N6; #38 A2, A7; #37 T7, T8, T10).

`StreamDescriptorMessage` is now a common header plus `StreamInfo info`, a union over `StreamKind` with one case per
kind (`VIDEO` → `video`, `TRACKER` → `tracker`, `PERSONS` → `persons`, `MESH` → `mesh`, `SURFELS` → `surfels`, `EYE` →
`eye`, `HAND` → `hand`, `IMU` → `imu`):

```
StreamDescriptorMessage { stream_topic; stream_topic_schema; ? uint32 revision; SRFrameRef frame; ClockDomain clock;
                          double frame_rate; int64 sensor_latency_ns; calib_topic; StreamInfo info;
                          sequence<BufferInfoProperty> properties; }
VideoStreamInfo   { width; height; step; VideoPixelFormat pixel_format; PixelSemantics semantics; VideoCodec codec;
                    VideoH26xProfile h26x_profile; ? uint32 bitrate; ? float depth_units_per_meter;
                    ? CameraIntrinsics intrinsics; sequence<uint8> codec_data; }
TrackerStreamInfo { MarkerType marker_type; string marker_system; string marker_domain;
                    sequence<SISTargetInfo> targets; ? SRCovariance6 nominal_covariance; }
MeshStreamInfo    { MeshEncoding encoding; uint64 vertices_capacity; uint64 faces_capacity; uint32 camera_id_count; }
SurfelStreamInfo, EyeStreamInfo, HandStreamInfo, ImuStreamInfo: empty in 0.11.0
```

Where each Plan 5 descriptor field went:

| Plan 5 | rc1 |
|---|---|
| `std_msgs/Header header`: `stamp` | gone; `revision` says when the content changed (A7, below) |
| `header.frame_id` | `frame` (`SRFrameRef`) |
| `stream_topic`, `stream_topic_schema`, `sensor_latency_ns`, `calib_topic` | unchanged |
| `SensorType sensor_type` | `info`'s discriminator (`StreamKind`); for video also `info.video.semantics` (table below) |
| `uint32 frame_rate` | `double frame_rate` (Hz; 0 = irregular). The persons' `rate_hz` property goes here too |
| `string clock_domain` | `ClockDomain clock` (`id` "" = the deployment clock); authoritative for every message whose `header.clock` is absent |
| `BufferInfo buffer_info` (`semantic_type`, `frame_size`, `width`, `height`, `bits_per_element`, `stride`, `properties`) | gone. Its `properties` → the top-level `properties`, now **vendor extensions only**; the sizes are in the typed case |
| `image_width`, `image_height`, `image_step`, `image_format` | `info.video.width`, `height`, `step`, `pixel_format` |
| `image_compression` (`VideoImageCompression`) | `info.video.codec` (`VideoCodec`; table below) |
| `h26x_profile` | `info.video.h26x_profile`: the profile only, it no longer selects the codec |
| `h26x_bitrate` | `info.video.bitrate` (`? uint32`; absent: lossless, uncompressed or not stated) |
| the property `h26x_codec_data` | `info.video.codec_data` (Annex-B parameter sets; empty if none or not known yet) |
| – | `info.video.depth_units_per_meter` (`?`; present for `DEPTH` with an integer format, e.g. 1000 for L16 in mm; absent for `DEPTH_F32`), `info.video.intrinsics` (`?`, the nominal intrinsics) |
| `marker_type` | `info.tracker.marker_type` |
| `marker_system` as `{system}@{domain}` (B97) | `info.tracker.marker_system` (`family[:variant]`, never with `@`) + `info.tracker.marker_domain` ("" = engine-wide) |
| the properties `target/<name>` = id | `info.tracker.targets` (`SISTargetInfo { id, name, kind }`; `id` = `Target.target_id`) |
| – | `info.tracker.nominal_covariance` (`?`; the default fusion weight for a target without `pose_covariance`) |
| the person properties `joint_set`, `up`, `smoothing` | `info.persons.joint_set` + `joint_set_version`, `info.persons.up`, `info.persons.smoothing`; the one-euro parameters (`min_cutoff_hz`, `beta`, `d_cutoff_hz`, `bone_tol`) stay in `properties` |
| the person properties `world_frame`, `clock_domain`, `handedness`, `units` | `frame`, `clock`; handedness (right) and units (m) are fixed by the schema and not sent |
| the mesh properties (`num_cameraids`, capacities) | `info.mesh.camera_id_count`, `vertices_capacity`, `faces_capacity`, `encoding` |

- **The revision rule (#38 A7).** `revision` present: a changed revision means changed content, and a reader (the
  SQE engine) re-reads the descriptor; an unchanged revision means unchanged content. Absent: unversioned, the reader
  compares the content. A producer that changes content (filling `codec_data` later, a recalibration) bumps it.
  `CameraSensor.revision` follows the same rule.
- **The marker rule (#35 N6).** `marker_system` is `family[:variant]` (`"art"`, `"aruco:DICT_4X4_50"`). For a
  template marker tracker the engine splits it at the **first** `:` into `SISTargetAlias.family` and `variant` (""
  without `:`), and looks a marker id N up as `(family, variant, "N")`. A `marker_system` containing `@` is malformed.
  `marker_domain` is copied into `SRMaterialization.marker_domain`. *(Split at the first `:`: to be confirmed by the
  SQE review.)*
- **A 0.11.0 reader meeting a 0.11.x case** (a `StreamKind` it does not know) skips the union by its DHEADER and
  reads `properties` after it. Goldens `evo_v1_Boxed`/`evo_v2_Boxed` pin this; fast-cdr does it.
- `BufferInfoProperty` stays: it is the element type of `properties`.

**Calibration, parameters, process info, presence** (`pcpd_msgs`, `tcnart_msgs/msg/Presence.idl`; #38 A3, A4;
#37 T13–T15).

| Type | Plan 5 | rc1 |
|---|---|---|
| `CameraSensor` | …, `tcnart_msgs::msg::CameraModel depth_parameters`, `color_parameters`, `color2depth_transform`, `uint32 frame_rate`, … | …, **`? tcn_types::msg::CameraIntrinsics depth_parameters`**, **`? … color_parameters`** (absent: disabled or uncalibrated), `color2depth_transform`, **`double frame_rate`**, …, appended **`? uint32 revision`** (A7 rule), **`? CalibrationProvenance provenance`**, **`CoordinateSystem coord`** (the camera axes of `camera_pose`; ArteKMed's are OpenGL's) |
| `CalibrationProvenance` (new) | – | `string tool` (`"sqe-calibrate"`, `"hta.rig_calibration"`), `string method`, `? double residual`, `? SRCovariance6 covariance`, `? int64 computed_at_ns` (Unix epoch), `uint32 samples` |
| `DeviceContextReply` | `int32 frame_rate` | `double frame_rate` |
| `ParameterValueType` | `…`, `STRUCT` (7), `UINT32` (8), `UINT64` (9) | `STRUCT` deleted, `UINT32` 7, `UINT64` 8, **`ENUM` 9** (table below) |
| `ParameterValue` (union) | case `STRUCT`: `ParameterList struct_value` | case `ENUM`: **`EnumEntry enum_value`** (`{ label, int64 value }`); `struct_value` removed |
| `ParameterDescriptor` | `has_minimum`/`minimum`, `has_maximum`/`maximum`, `has_step`/`step` | **unchanged**: kept as a named exception to the `@optional` rule ([idl-style.md](../idl-style.md)); `enum_entries` is non-empty for value type `ENUM` |
| `StreamAnnouncement` | `string role` (`"data"`, `"descriptor"`, `"state"`), `string mode` (`"stream"`, `"latched"`) | `StreamRole role`, `StreamMode mode` (table below) |
| `ProcessInfo` | `string started_at` (ISO 8601 UTC) | `builtin_interfaces::msg::Time started_at` (UTC) |
| `GraphReply` | – | no wire change: `schema` (e.g. `"tcn.plan_graph.v1"`) is the authority for `graph_json` (#37 T14; HTA delivers the schema document) |
| `TcnartPresenceMessage` | `header`, `heart_beat_counter`, `services` | `header`, **`string rpc_root`** (the process's RPC root, `<P>/rpc`), `heart_beat_counter`, `services` |
| `ServiceInfo` | `string service_info` | `string service_type` (renamed: the service's type name) |

The deleted `CameraModel` maps to the `pinhole` case of `CameraIntrinsics`:

| `tcnart_msgs::msg::CameraModel` | `tcn_types::msg::PinholeIntrinsics` |
|---|---|
| `focal_length[0]`, `[1]` | `fx`, `fy` |
| `principal_point[0]`, `[1]` | `cx`, `cy` |
| `radial_coefficients[0..5]` (k1 … k6; `[6..7]` unused), `tangential_coefficients[0..1]` (p1 p2) | `distortion` in OpenCV order: k1 k2 p1 p2 k3 for `OPENCV_RADTAN`, k1 k2 p1 p2 k3 k4 k5 k6 for `OPENCV_RATIONAL` |
| `camera_model` (`CameraModelType`) | `distortion_model` (`SRDistortionModel`; table below) |
| `uint16 image_width`, `image_height` | `? uint32 image_width`, `? uint32 image_height` |

**Voice** (`conversation_msgs`; #36).

| Type | Plan 5 | rc1 |
|---|---|---|
| `SessionStart` | `header`, `direction`, `ingress_format`, `expected_stt_format`, `expected_tts_format`, `egress_format`, `stt_backend`, `llm_backend`, `tts_backend`, `language_hint`, `voice_hint`, `allow_barge_in`, `enable_partial_stt`, `enable_partial_llm`, `enable_partial_tts`, `identity` | `header`, `identity`, `allow_barge_in` (false: off, whatever the mode), `enable_partial_stt`, **`? BargeInMode barge_in_mode`** (a lower mode than the server's; absent: the server's). Formats, backends and language/voice defaults come from discovery only (S1) |
| `SessionConfigUpdate`, `SttRequest` | – | **deleted** |
| `SessionStateEvent` | `header`, `state`, `status` | `header`, `state`, **`SessionStateReason reason`**, `status`. `SESSION_STATE_PLAYING` (7) is reachable now: between `TtsDone` and the client's `PLAYBACK_STATE_DRAINED` (S3) |
| `PlaybackEvent` (new) | – | `header`, `PlaybackState state`, `unsigned long long played_samples`, `unsigned long sample_rate_hz`; the client publishes it per turn on `{session}/control/playback`, only to a server that advertises `playback_events` (S4) |
| `VadEvent` | `header`, `speech_active`, `float speech_probability`, `start_ms`, `end_ms`, `EventState state`, `backend` | `header`, `speech_active`, **`unsigned long long stream_position_ms`** (ms since the first sample of the session's input audio stream), **`? float speech_probability`** (only from a VAD that has one), `backend` (A1) |
| `AudioOpusFrame` | …, `first_sample_index`, `packet` | …, `first_sample_index`, **`unsigned long long turn_sample_index`**, `packet` (F2) |
| `TtsAudioChunk` | …, `is_final_chunk`, `samples_f32` | …, `is_final_chunk`, **`unsigned long long turn_sample_index`** (0 at the turn's first TTS sample), `samples_f32` (F1) |
| `TtsDone` | …, `total_chunks`, `status` | …, `total_chunks`, **`unsigned long long total_samples`**, **`unsigned long sample_rate_hz`**, `status` (T1) |
| `SttPartial`, `LlmPartial` | …, `EventState state` | `state` removed |
| `SttFinal` | …, `EventState state`, `float confidence`, `boolean has_confidence` | `state` and `has_confidence` removed; **`? float confidence`** |
| `LlmFinal` | …, `EventState state`, `prompt_tokens`, `generated_tokens`, `has_prompt_tokens`, `has_generated_tokens` | `state` and the `has_*` removed; **`? unsigned long prompt_tokens`**, **`? unsigned long generated_tokens`** |
| `InteractionActivity` | no header | **`MessageHeader header`** first |
| `HealthStatus` | `string _component` (escaped keyword; the member `component`) | `string component_name` (M1) |
| `rpc::AppServerInfo` | …, `language_hint`, `voice_hint` (kept: server defaults) | appended **`sequence<string> capabilities`**, **`BargeInMode barge_in_mode`** (the server's mode), **`sequence<FaceAnimationLayout> face_layouts`** |
| `FaceAnimationLayout` (new) | – | `string layout_id`, `FaceRig rig`, `sequence<string> channel_names` (always filled; ARKit's 52 names for `ARKIT_52`), `sequence<string> emotion_names`, `float frame_rate_hz` |

- **C++:** the new `@optional` members have `std::optional`-style accessors (`eprosima::fastcdr::optional`):
  `Target::pose()`, `Target::capture_time_ns()`, `HandTrackingMessage::gaze()`, `Person::previous_id()`,
  `HumanPose::member()`, `HumanJoint::rotation()`, `VideoStreamMessage::intrinsics()`,
  `StreamDescriptorMessage::revision()`, `VideoStreamInfo::bitrate()`, `CameraSensor::revision()`/`provenance()`,
  `SttFinal::confidence()`, `LlmFinal::prompt_tokens()`/`generated_tokens()`, `VadEvent::speech_probability()`,
  `SessionStart::barge_in_mode()`, and the others marked `?` above. Code that read `gaze_valid` or
  `has_confidence` no longer compiles. An optional bounded string needs an explicit
  `eprosima::fastcdr::fixed_string<128>` to set (`HumanPose::member`).
- **C++: `ParameterValue` is held by value.** With the forward declarations gone, `Parameter`,
  `ParameterDescriptor` (`minimum`, `maximum`, `step`, `default_value`) and the types that contain them
  (`ParameterList`, `GenericParameterRequest`/`Reply`, `ServiceInfoReply`, `ParameterDescriptorList`, …) hold
  `ParameterValue` as a member, not a pointer, and their `operator==` compares values. The accessors keep their
  signatures; `struct_value()` is gone and `enum_value()` is new. Code that relied on pointer identity or on
  `sizeof` of these classes changes behaviour. This also fixes the Plan 1 note about `operator==` on these types.
- **`HealthStatus.component_name`:** the generated accessor changes from `_component`/`component` (per language) to
  `component_name`.

#### Enums: old → new values

These are the values on the wire. **A mirror that keeps old numbers decodes plausible wrong values**, so take the
names and values from here (or from `share/tcn_schema/enums.json`). The "0.10" column is the 0.10.7 value, the
"Plan 4" column the value after the Plan 4 renames, where it differs.

**`tcnart_msgs::msg::SensorType` → `StreamKind` + `PixelSemantics`** (`SensorType` was not renamed by Plan 4).

| `SensorType` (0.10 = Plan 5) | value | `StreamKind` | value | `PixelSemantics` (video only) |
|---|---|---|---|---|
| `SENSOR_TYPE_NONE` | 0 | `STREAM_KIND_UNKNOWN` | 0 | – |
| `SENSOR_TYPE_VIDEO_COLOR` | 1 | `STREAM_KIND_VIDEO` | 1 | `PIXEL_SEMANTICS_COLOR` (1) |
| `SENSOR_TYPE_VIDEO_DEPTH` | 2 | `STREAM_KIND_VIDEO` | 1 | `PIXEL_SEMANTICS_DEPTH` (2) |
| `SENSOR_TYPE_VIDEO_IR` | 3 | `STREAM_KIND_VIDEO` | 1 | `PIXEL_SEMANTICS_IR` (3) |
| – | – | `STREAM_KIND_VIDEO` | 1 | `PIXEL_SEMANTICS_MASK` (4), new: a per-pixel label or alpha |
| `SENSOR_TYPE_TRACKING_POSE` | 4 | `STREAM_KIND_TRACKER` | 2 | – |
| `SENSOR_TYPE_TRACKING_BODY` | 7 | `STREAM_KIND_PERSONS` | 3 | – |
| `SENSOR_TYPE_MESH_BITSTREAM` | 12 | `STREAM_KIND_MESH` | 4 | – |
| `SENSOR_TYPE_SURFEL` | 13 | `STREAM_KIND_SURFELS` | 5 | – |
| `SENSOR_TYPE_TRACKING_EYE` | 5 | `STREAM_KIND_EYE` | 6 | – |
| `SENSOR_TYPE_TRACKING_HAND` | 6 | `STREAM_KIND_HAND` | 7 | – |
| `SENSOR_TYPE_IMU_ACCEL`, `IMU_GYRO`, `IMU_MAG` | 9, 10, 11 | `STREAM_KIND_IMU` | 8 | – |
| `SENSOR_TYPE_AUDIO` | 8 | no kind (no tcn audio stream has a descriptor) | – | – |

**`tcnart_msgs::msg::VideoImageCompression` (+ `VideoH26xProfile`) → `VideoCodec`.**

| 0.10 | value | Plan 4 | value | `VideoCodec` | value |
|---|---|---|---|---|---|
| – | | `VIDEO_IMAGE_COMPRESSION_UNKNOWN` | 0 | `VIDEO_CODEC_UNKNOWN` | 0 |
| `CompressionType_Raw` | 0 | `VIDEO_IMAGE_COMPRESSION_RAW` | 1 | `VIDEO_CODEC_RAW` | 1 |
| `CompressionType_Png` | 1 | `VIDEO_IMAGE_COMPRESSION_PNG` | 2 | `VIDEO_CODEC_PNG` | 2 |
| `CompressionType_Jpeg` | 4 | `VIDEO_IMAGE_COMPRESSION_JPEG` | 5 | `VIDEO_CODEC_JPEG` | 3 |
| `CompressionType_JpegLs` | 5 | `VIDEO_IMAGE_COMPRESSION_JPEG_LS` | 6 | `VIDEO_CODEC_JPEG_LS` | 4 |
| `CompressionType_H26x` with an H.264 profile (`H264_BASE`, `H264_MAIN`, `H264_HIGH`) | 2 | `VIDEO_IMAGE_COMPRESSION_H26X` | 3 | `VIDEO_CODEC_H264` | 5 |
| `CompressionType_H26x` with `H265_MAIN` | 2 | `VIDEO_IMAGE_COMPRESSION_H26X` | 3 | `VIDEO_CODEC_H265` | 6 |
| – | | – | | `VIDEO_CODEC_H264_LOSSLESS` (new) | 7 |
| `CompressionType_Zdepth` | 3 | `VIDEO_IMAGE_COMPRESSION_ZDEPTH` | 4 | `VIDEO_CODEC_ZDEPTH` | 8 |
| – | | – | | `VIDEO_CODEC_NDQ` (new) | 9 |

- `VideoH26xProfile` keeps its Plan 4 values (`NONE` 0, `H264_BASE` 1, `H264_MAIN` 2, `H264_HIGH` 3, `H265_MAIN` 4);
  it is the profile only, `NONE` for other codecs.
- `VideoPixelFormat` gains `VIDEO_PIXEL_FORMAT_DEPTH_F32` = **8** (float32 metres; `depth_units_per_meter` absent).
  The others are unchanged (`UNKNOWN` 0 … `RGBA32` 7).
- `MeshEncoding` (new): `UNKNOWN` 0, `BITSTREAM_V1` 1 (`MeshBitstreamMessage`, format A, 28-byte vertices),
  `STREAM_V2` 2 (MeshStream v2, `"TMS2"`; its IDL comes with #37 T12 in 0.11.x).

**`tcnart_msgs::msg::CameraModelType` (deleted) → `tcn_types::msg::SRDistortionModel`.** Unreal's hard-coded
ordinals and every JSON mirror of `CameraModelType` go.

| `CameraModelType` (0.10 and Plan 4 values) | value | rc1 |
|---|---|---|
| `CAMERA_MODEL_TYPE_NONE` | 0 | `depth_parameters`/`color_parameters` **absent** |
| `CAMERA_MODEL_TYPE_PINHOLE` | 1 | `SR_DISTORTION_MODEL_NONE` (1) |
| `CAMERA_MODEL_TYPE_OPEN_CV` | 2 | `SR_DISTORTION_MODEL_OPENCV_RADTAN` (2) |
| `CAMERA_MODEL_TYPE_FULL_OPEN_CV` | 3 | `SR_DISTORTION_MODEL_OPENCV_RATIONAL` (3) |
| `CAMERA_MODEL_TYPE_SENSOR_AZURE_KINECT` | 4 | `SR_DISTORTION_MODEL_OPENCV_RATIONAL` (3) |

**`pcpd_msgs::rpc::ParameterValueType`.** **Hazard: 9 was `UINT64` and is now `ENUM`.** A Plan 4 mirror that sends
9 for a `uint64` parameter is read as an `ENUM` case. And a 0.10 mirror that was never updated (`UINT32` 7,
`UINT64` 8) matches rc1 for those two by accident only: every other value moved.

| 0.10 | value | Plan 4 | value | rc1 | value |
|---|---|---|---|---|---|
| – | | `PARAMETER_VALUE_TYPE_UNKNOWN` | 0 | `PARAMETER_VALUE_TYPE_UNKNOWN` | 0 |
| `PVT_STRING` … `PVT_BOOL` | 0 … 5 | `…_STRING` … `…_BOOL` | 1 … 6 | `…_STRING` … `…_BOOL` | 1 … 6 (unchanged from Plan 4) |
| `PVT_STRUCT` | 6 | `PARAMETER_VALUE_TYPE_STRUCT` | 7 | **deleted** (never produced) | – |
| `PVT_UINT32` | 7 | `PARAMETER_VALUE_TYPE_UINT32` | 8 | `PARAMETER_VALUE_TYPE_UINT32` | **7** |
| `PVT_UINT64` | 8 | `PARAMETER_VALUE_TYPE_UINT64` | 9 | `PARAMETER_VALUE_TYPE_UINT64` | **8** |
| – | | – | | `PARAMETER_VALUE_TYPE_ENUM` (new, `EnumEntry enum_value`) | **9** |

(`…_STRING` 1, `INT32` 2, `INT64` 3, `FLOAT` 4, `DOUBLE` 5, `BOOL` 6.) Golden: `pcpd_msgs_Parameter.hex` (an
`ENUM` value, discriminator 9).

**Strings that became enums.**

| Field | 0.10 / Plan 5 string | rc1 enumerator | value |
|---|---|---|---|
| `StreamAnnouncement.role` | – | `STREAM_ROLE_UNKNOWN` | 0 |
| | `"data"` | `STREAM_ROLE_DATA` | 1 |
| | `"descriptor"` | `STREAM_ROLE_DESCRIPTOR` | 2 |
| | `"state"` | `STREAM_ROLE_STATE` | 3 |
| `StreamAnnouncement.mode` | – | `STREAM_MODE_UNKNOWN` | 0 |
| | `"stream"` | `STREAM_MODE_STREAM` | 1 |
| | `"latched"` | `STREAM_MODE_LATCHED` | 2 |
| `PersonTrackingMessage.joint_set`, `HumanPoseMessage.joint_set` | `"coco17"`, `"halpe26"`, `"tcnart_body_v1"` | `JOINT_SET_COCO17`, `JOINT_SET_HALPE26`, `JOINT_SET_TCN_BODY` (each with `joint_set_version` 1) | 1, 2, 4 |
| `PersonStreamInfo.smoothing` (was the property `smoothing`) | `"off"`, `"one_euro"` | `PERSON_SMOOTHING_OFF`, `PERSON_SMOOTHING_ONE_EURO` | 1, 2 |

**Constants that became enums (values +1 or re-based).**

| 0.10 / Plan 5 constant | value | rc1 enumerator | value |
|---|---|---|---|
| (none) | – | `PERSON_TRACK_STATE_UNKNOWN` | 0 |
| `PERSON_STATE_TENTATIVE` | 0 | `PERSON_TRACK_STATE_TENTATIVE` | **1** |
| `PERSON_STATE_CONFIRMED` | 1 | `PERSON_TRACK_STATE_CONFIRMED` | **2** |
| `PERSON_STATE_PREDICTED` | 2 | `PERSON_TRACK_STATE_PREDICTED` | **3** |
| (none) | – | `PERSON_STATE_UNKNOWN` | 0 |
| `HUMAN_POSE_TRACKED` | 0 | `PERSON_STATE_TRACKED` | **1** |
| `HUMAN_POSE_COASTING` | 1 | `PERSON_STATE_COASTING` | **2** |
| `HUMAN_POSE_LOST` | 2 | `PERSON_STATE_LOST` | **3** |
| (none) | – | `JOINT_SOURCE_UNKNOWN` | 0 |
| `HUMAN_JOINT_OBSERVED` (bit) | 1 | `JOINT_SOURCE_OBSERVED` | 1 |
| `HUMAN_JOINT_DERIVED` (bit) | 2 | `JOINT_SOURCE_DERIVED` | 2 |
| `HUMAN_JOINT_DEFAULTED` (bit) | 4 | `JOINT_SOURCE_DEFAULTED` | **3** |
| `HUMAN_JOINT_HAS_ROTATION` (bit) | 8 | the presence of `HumanJoint.rotation` | – |

**New enums** (all `@appendable`, the first enumerator at 0):

| Enum | Enumerators (value = position) |
|---|---|
| `tcnart_msgs::msg::JointSet` (generated) | `JOINT_SET_UNKNOWN` 0, `COCO17` 1, `HALPE26` 2, `TCN_LITE` 3, `TCN_BODY` 4, `SMPL_24` 5, `SMPLX_55` 6, `SMPLH_52` 7 |
| `tcnart_msgs::msg::SmplModel` | `SMPL_MODEL_UNKNOWN` 0, `SMPL` 1, `SMPLH` 2, `SMPLX` 3 |
| `tcnart_msgs::msg::SmplGender` | `SMPL_GENDER_UNKNOWN` 0, `NEUTRAL` 1, `FEMALE` 2, `MALE` 3 |
| `tcnart_msgs::msg::PersonSmoothing` | `PERSON_SMOOTHING_UNKNOWN` 0, `OFF` 1, `ONE_EURO` 2 |
| `conversation_msgs::msg::BargeInMode` | `BARGE_IN_MODE_UNKNOWN` 0, **`OFF` 1**, `VAD` 2, `SUSTAINED` 3, `WORDS` 4. #36 S6 drafted `OFF` = 0: every value is one higher than the draft (E6) |
| `conversation_msgs::msg::SessionStateReason` | **`SESSION_STATE_REASON_NONE` 0** (the one 0.11 enum whose first enumerator is `NONE`: 0 is "no reason"), `COMPLETED` 1, `BARGE_IN` 2, `NON_SPEECH` 3, `NOT_ADDRESSED` 4, `CANCELLED` 5, `ERROR` 6 |
| `conversation_msgs::msg::PlaybackState` | `PLAYBACK_STATE_UNKNOWN` 0, `STARTED` 1, `DRAINED` 2, `CLEARED` 3 |
| `conversation_msgs::msg::FaceRig` | `FACE_RIG_UNKNOWN` 0, `ARKIT_52` 1, `CUSTOM` 2 |
| `pcpd_msgs::rpc::StreamRole`, `StreamMode`; `tcnart_msgs::msg::StreamKind`, `PixelSemantics`, `VideoCodec`, `MeshEncoding`, `PersonTrackState`, `PersonState`, `JointSource` | in the tables above |

- **Deleted enums:** `SensorType`, `VideoImageCompression`, `CameraModelType`, `conversation_msgs::msg::EventState`
  (`UNKNOWN` 0, `PARTIAL` 1, `FINAL` 2; the `Stt*`/`Llm*` messages are partial or final by their type).
- No other existing enum changed its values. `SessionState` keeps its values.

#### Capabilities (`AppServerInfo.capabilities`, #36 D1, D4)

A capability is a name `[a-z0-9_]{1,64}`, as in the SQE (Plan 5). A client ignores a name it does not know.

| Capability | Meaning |
|---|---|
| `playback_events` | the server consumes `PlaybackEvent`; a client sends it only when this is advertised. Without it the server falls back to `TtsDone.total_samples` (T1) |
| `barge_in_modes` | the server honours `SessionStart.barge_in_mode` |
| `wake_word` | turns are gated by a wake word |

`face_animation` follows in 0.11.x with `FaceAnimationChunk` (F4).

#### The joint-set table (decisions §7 HumanPose)

- **The only source** is `src/dds/tcn/tcnart_msgs/msg/joint_sets.yaml`. Everything else is generated from it by
  `python3 -I tools/idl_cli.py joint-sets --table src/dds/tcn/tcnart_msgs/msg/joint_sets.yaml` (`--check` fails when a
  generated file differs; `--previous <yaml>` refuses a changed set whose `version` was not bumped). Never re-type
  the rows.

| Generated file | Installed as | For |
|---|---|---|
| `src/dds/tcn/tcnart_msgs/msg/JointSet.idl` | the `JointSet` enum in `tcnart_msgs` | every codec |
| `src/dds/tcn/tcnart_msgs/msg/JointSets.md` | `share/tcn_schema/dds/tcnart_msgs/msg/JointSets.md` | people: order, parents, bones, rest pose, body frame, "Rotations", "Retired names", SMPL verification status |
| `src/joint_sets/include/tcn/joint_sets.h` | `include/tcn/joint_sets.h` | C++: header-only `tcn::joint_sets::find(set, version)`; reachable through the include directory of any `tcn_schema` component (there is no dedicated component) |
| `src/joint_sets/cs/JointSets.cs` | `share/tcn_schema/JointSets.cs` | C# (Unity copies it): `Tcn.Schema.JointSets.Find(set, version)`; C# 7.2. Only the drift test checks it; no CI compiles it |
| `src/joint_sets/joint_sets.json` | `share/tcn_schema/joint_sets.json` | every other language (TypeScript, Rust, Python) |

- **Refuse an unknown (`joint_set`, `joint_set_version`) pair**, loudly. `find` returns `nullptr` (C++) or `null`
  (C#) for it. A set's `version` is bumped on any change to its joints, bones or rest pose; an ordinal is never
  reused.
- **Trees are "one acyclic tree", not "parent first":** `TCN_LITE` lists the head before its parent, the chest, so
  forward kinematics sorts topologically.
- `SMPL_24` (joints 22–23) and `SMPLX_55` are not yet verified against the reference parents (`verified` in the
  YAML and in `JointSets.md`); closing that may bump their `version` (0.11.x).

#### Golden files (new in this plan)

`test/golden/*.hex`, values in `test/golden/golden_test.cpp`; encode with the Plan 4 rules, and check every
presence byte.

| File | What it pins |
|---|---|
| `tcn_types_StreamHeader.hex` | `StreamHeader`: DHEADER 0x34, `device_time_ns`, `group_id` and `clock` absent, an `SRFrameRef` |
| `tcnart_msgs_TargetTrackingMessage.hex` (regenerated) | three targets: pose present without covariance; **pose absent** with `capture_time_ns` 5; pose with a present `SRCovariance6` |
| `tcnart_msgs_VideoStreamMessage.hex` | intrinsics present (the `CameraIntrinsics` union, a pinhole) before `image_bytes` and `image` |
| `tcnart_msgs_VideoStreamMessage_nominal.hex` | the same frame with **intrinsics absent**: the new fixed prefix for zero-copy encoders |
| `tcnart_msgs_HumanPoseMessage.hex` | `group_id` present, `TCN_LITE` v1, `member` absent, one joint with and one without `rotation`, `smpl` absent |
| `tcnart_msgs_StreamInfo.hex` | the union's tracker case |
| `tcnart_msgs_StreamDescriptorMessage.hex` | a video descriptor: `revision` present, `bitrate` present, `depth_units_per_meter` absent, intrinsics present, `codec_data` |
| `tcnart_msgs_StreamDescriptorMessage_unversioned.hex` | a tracker descriptor with **`revision` absent** |
| `evo_v1_Boxed.hex` / `evo_v2_Boxed.hex` | a union with a case unknown to the older reader: it skips the case by DHEADER and reads `tail` (V2 → V1: `tail` = -1; V1 → V2: `a` = 5, `tail` = 42) |
| `pcpd_msgs_Parameter.hex` | `Parameter { "mode", enum_value { "auto", 2 } }`: discriminator 9 (`ENUM`) |
| `conversation_msgs_SessionStart.hex` | the reshaped `SessionStart`, `barge_in_mode` present (`VAD` = 2); no padding after a struct's last member |

#### For SQE review (before 0.11.0)

1. **The B-number.** `kProtocolBreak` is 99, provisional: set at 0.11.0 to the engine's B-number for the break.
2. **The marker split.** `TrackerStreamInfo.marker_system` is `family[:variant]`, split at the first `:`; no `@`.
   `marker_domain` is separate. Confirm.
3. **The device frame of a tracker** is `TargetTrackingMessage.header.frame`, an `SRFrameRef`; `Target.pose` is
   optional and a target may carry its own `capture_time_ns`.
4. **The revision rule (A7):** a changed `revision` means changed content; absent means compare the content.

#### Tools

- `tools/idl_cli.py moves --old <plan 5 src/dds> --new src/dds --spec tools/idl/streams_0_11.json --plan "plan 6"
  --json src/dds/tcn/type_map_0_11_streams.json --markdown docs/migration/0.11-streams-type-map.md` proves that
  nothing else changed. The report now lists the spec's new types and says "none" under an empty section.
- `tools/idl_cli.py joint-sets` generates and checks the joint-set files (above).
- `tools/idl/tests/test_streams_enums.py` pins every enum of this plan (names, order, `@appendable`) and the deleted
  ones; `test_stream_header.py` pins the ten stream messages and the descriptor's shape.

## 2. Per consumer: the rc1 checklist

Each team's list, in the order to work through it. Every item is a box to tick and names the plan that brought it;
the details are in that plan's part of §1. Items marked *(wire)* need the lockstep cut-over to `tcn_schema=0.11`; all
others can be done ahead of it. File and line references come from a `grep` of each repository's default branch
(outside third-party, build and history directories) and the counts are **approximate**: use them to size the work,
not as the list of sites.

### SQE engine (spatial_query_engine_rs, sqe-calibrate)

**1. Switch to rc1**
- [ ] *Plan 6:* depend on tcn_schema `0.11.0-rc1`; regenerate the `messages.rs` wire registry and mirrors against its
  `share/tcn_schema/types.json` and `enums.json` (optional since Plan 1, now the easiest way to get Plans 4–6 right).
- [ ] *Plan 3:* about 82 moved type names in 14 files, mostly the `messages.rs` wire registry:
  `tcnart_msgs::rpc::SIS*` → `sqe_msgs::rpc::*` (74), `tcnart_msgs::msg::SR*` → `sqe_msgs::msg::*` (5), shared types
  → `tcn_types::msg::*` (3). Apply `share/tcn_schema/type_map_0_11.json`, then `type_map_0_11_sqe.json`, then
  `type_map_0_11_streams.json`.
- [ ] *Plan 3:* drop the `Pose6DMapMessage` tracker payload (`messages.rs:2081-2112`, decided), the definitions of
  `Pose6DMapMessage`, `RigidTransformMap` and `RigidTransformMapItem` (`crates/sqe-core/src/messages.rs:1958-1985`),
  and the `CameraInfoMessage` test mention.
- Plan 2: nothing to do (optional: `ros2_types.json` gives ROS names and hashes for showing ROS frames later).

**2. The codec** *(wire)*
- [ ] *Plan 4:* **the Rust codec to XCDR2** for every tcn type: `00 09`, DHEADERs, 4-byte maximum alignment, optional
  flags, also inside the nested ROS `Header`. Today `messages.rs:3737` and `serialization.rs:315,360` check `00 01`.
- [ ] *Plan 4:* the annotation `tcn_schema=0.11`; `application/cdr` and `application/json` matched exactly (Plan 5).
- [ ] *Plan 4:* the media enum and `CoordinateAxis` mirrors in `messages.rs` take the new names and values
  ([0.11-enum-map.md](0.11-enum-map.md)).

**3. Removed fields** *(wire)*
- [ ] *Plan 4:* fields it reads: `TargetTrackingMessage.target_origin` (`src/ingest.rs:718-730`, the warning at
  `messages.rs:2167`); `Target.quality` (#35 G2).
- [ ] *Plan 4:* fields it writes or mirrors: `StreamDescriptorMessage.{calib_topic_schema, audio_channels,
  aac_profile}` and the `AudioAACProfile` mirror (`crates/sqe-core/src/engine.rs:7515,7538-7539,9610-9624`).

**4. The SQE wire redesign** *(wire)* (counts: a `grep` for the old names in the `.rs` files, outside `target/` and
the docs). Read [Plan 5 in §1](#plan-5-the-sqe-wire-redesign-35-wire-change) first; this is the largest item.
- [ ] *Plan 5:* **the wire mirrors in `crates/sqe-core/src/messages.rs`** (about 840 uses of the deleted status and
  handle types, 360 of `SISFrameRef`, in about 60 files, tests included): every reshaped type and every enum value of
  the Plan 5 tables, in particular
  - `SRNode`, `SREdge`, `SRCalibration`, `SRGraph` (no `name`), the views, `SRTransform` (no `_COV` cases; covariance
    is an optional `SRCovariance6`/`SRCovariance3` inside the case), `SRPoint2D` (no covariance, still 16 bytes);
  - **every reply starts with `SISReplyHeader`**; `SISMutationStatus`, `SISGraphQueryStatus`, `SISTargetQueryStatus`,
    `SISRelationStreamStatus`, `SISRelationSampleStatus` and `SISRelationStreamHandle` go (`messages.rs:570, 908,
    1335, 1383`);
  - `SISFrameRef` (`messages.rs:679`) becomes `SRFrameRef`; edges name each other with `SRRelationRef`;
  - the stream types: `SISRelationTerms`, `SISRelationStatus`, `SISStreamHandle`, `SISPlanNotification`,
    `SISRelationOutput`, the plan types (`ClockDomain`, optional epochs and indexes).
- [ ] *Plan 5:* **stored values: the engine persists enum names, not ordinals.** `src/persistence.rs` writes
  `node_type` and `temporal_type` through `enum_to_stored_str`, i.e. the `#[serde(rename = "SRG_NODE_GENERIC")]`
  strings of `messages.rs`. After the rename a 0.10 row holds a name that no longer parses:
  - an unreadable `temporal_type` reads as **`StaticUncalibrated`** (`read_temporal_type`, fail closed: the edge is
    not traversed) and an unreadable `node_type` reads as `Generic`. A store written by 0.10 would silently lose
    every edge;
  - migrate the stored rows, or keep the old names as `serde(alias)`. The edge's single `temporal_type` also splits
    into `kind` and `timing`; the store must write both;
  - the same holds for any JSON fixture (`fixtures/wire/*.json`, `tests/wire_golden.rs`) and for `0` as a default:
    `0` is `UNKNOWN` now.
- [ ] *Plan 5:* **stored node identities.** The store keys nodes by the 0.10 composed names: a domain node is stored
  with `global_identifier` and `name` both `@{domain}::{name}` (`src/persistence.rs:1000-1002`, `stored_scope`), and a
  marker node keeps its composed `…__sis_marker__…` name as both (`direct_discover_marker`,
  `src/persistence.rs:2745-2746`). After N1 and N5 a node's identity is its bare `name` plus `domain` (and, for a
  marker, the `SRMaterialization` fields), so a 0.11 engine on a 0.10 store must migrate those rows or read the old
  form too; otherwise it creates a second node for the same frame and breaks the meeting points.
- [ ] *Plan 5:* **`FrameRef` text and wire:** parse and format against `share/tcn_schema/sqe/frame_ref.tsv` in the
  Rust tests (mandatory). `sqe-op`, sqe-calibrate command lines and logs use the text form. Domain ids as
  `@{domain}::{name}` and the `:::*` suffix go (`context.rs`, `driver.rs:1327, 2225, 3176`).
- [ ] *Plan 5:* **the header at every refusal site, ingest included** (`src/ingest.rs`, the mutation and operator
  replies): map the old statuses with the status table; `NOT_IN_SCOPE` for a client scope miss, `NOT_PERMITTED` only
  for authorization; `UNKNOWN` terms are `REJECTED` + `MALFORMED`; `FUSED` and `FILTERED` are `REJECTED` +
  `UNSUPPORTED`; the transaction log without a directory is `REJECTED` + `UNSUPPORTED`.
- [ ] *Plan 5:* **data topic:** publish `SISRelationOutput` and no attachment (`crates/sqe-core/src/pose_attachment.rs`
  and its use in `src/pipeline_task.rs` go); `route_epoch` is set; the counters are `sequence_number`, per topic, from
  1, gapless per handle.
- [ ] *Plan 5:* **explicit topics:** the start reply carries `data_topic`, `status_topic` and `plan_topic` (the engine
  may keep composing them, `crates/sqe-core/src/topics.rs:243,249`); status and plan are published as
  `SISRelationStreamStatusNotification` and `SISPlanNotification`.
- [ ] *Plan 5:* **keys** (`topics.rs:34-43, 102`): `/sis/node/*`, `/sis/edge/*`, `/sis/stream/sample`; the marker
  announcement key carries the decimal id (`topics.rs:131-152`); add the queryable `sqe/{engine}/info`
  (`SISEngineInfoRequest` → `SISEngineSummaryReply`).
- [ ] *Plan 5:* **markers and names:** `marker_node_name`/`marker_prefix` (`engine.rs:612`, `context.rs:150,190`,
  `relation_streams.rs:182-195`) become engine-private; write `SRMaterialization` and `SREdge.materialized`; answer
  `SISGraphFilter` (G3) instead of prefix filters; `SISTargetAlias` is `family` + `variant` + `key`.
- [ ] *Plan 5:* **relation state and sample outcomes:** a start with no route is `REJECTED` + `NO_PATH`; a running
  stream that loses its route goes `PENDING` + `WITHDRAWN`; a sample's `NO_VALUE`/`TOO_OLD` are `OK` with `outcome`;
  on a non-OK reply `relation.state` is `UNKNOWN`.
- [ ] *Plan 5:* **the §6 shims that go** (one break, one B-number): B73 (the warning subscribers for a `put` on
  mutation keys, `src/ingest.rs:1217-1230`); B90 (marker discovery from poses, the warning at `engine.rs:5725`;
  every publisher announces its markers); B41 (the inactivity sweep and heartbeat fallback, `src/liveness.rs`,
  `driver.rs:1094`); B94 (re-deriving unqualified marker announcements); the pre-0.9 descriptor padding
  (`serialization.rs:205-225`, `ingest.rs:376`); unversioned payloads (`tcn_schema=0.11` is mandatory).

**5. Streams and the descriptor** *(wire)* (about 37 lines of `SensorType` in 11 files, 23 of them in `engine.rs`;
about 111 lines of `marker_system` in 19 files; about 24 lines of `buffer_info` in 8 files)
- [ ] *Plan 6:* **decode `StreamHeader`** in `TargetTrackingMessage` (the mirror at `messages.rs:2059`), so also inside
  `SISRelationOutput`, and in `Pose6DMessage`: the device frame is `header.frame` (an `SRFrameRef`), not a
  `frame_id` string. In `SISRelationOutput` the outer `clock_domain` and `sequence_number` win; leave
  `targets.header.clock` absent and set `targets.header.sequence_number` to the outer one.
- [ ] *Plan 6:* **`Target`** (`messages.rs:2027`): `pose` is optional (absent: no pose, never identity; skip the target
  for fusion, as for `is_tracked` false), and a per-target `capture_time_ns` overrides the header's for that target.
  sqe-calibrate reads `Target.pose` too (`crates/sqe-calibrate/src/sampler.rs`, `materialise.rs`).
- [ ] *Plan 6:* **the tracker descriptor** (the mirror at `messages.rs:1802`, `decode_stream_descriptor` at
  `serialization.rs:208`): read the `TRACKER` case `info.tracker` — `marker_type`, `marker_system`
  (`family[:variant]`), `marker_domain`, `targets` (`SISTargetInfo`, `id` = `Target.target_id`) and
  `nominal_covariance` (the fusion weight when `pose_covariance` is absent). **No `{system}@{domain}` parsing:**
  `split_marker_system` (`crates/sqe-core/src/engine.rs:7594`, split at the last `@`) goes; a `marker_system` with an
  `@` is malformed.
- [ ] *Plan 6:* **re-read a descriptor whose `revision` changed**; an absent `revision` means compare the content
  (the A7 rule). The descriptor no longer has a header stamp.
- [ ] *Plan 6:* **`clock` is a `ClockDomain`** (`id`, `kind`; equality on `id`, "" = the deployment clock), not the
  `clock_domain` string (`messages.rs:1829`); a message whose `header.clock` is absent is in the descriptor's clock.
  The clock-mismatch refusal is unchanged.
- [ ] *Plan 6:* the `SensorType` mirror becomes `StreamKind` (the value table in Plan 6); `BufferInfo` is gone
  (`engine.rs:9615`); `frame_rate` is a `double` (0 = irregular); `properties` holds vendor extensions only.
- [ ] *Plan 6:* skip an unknown `StreamInfo` case by its DHEADER (a 0.11.x kind), and read `properties` after it.

**6. The protocol floor and capabilities** *(wire)*
- [ ] *Plan 5:* join answers `SISJoinReply`; `SISEngineInfoReply` has `header` and `engine`; `SCHEMA_VERSION`
  0.10.7 → 0.11.0 (`src/operator.rs:52`). Announce `sample`, `domain`, `rational_lens` (if implemented) and
  `marker_domain`, and a `client_scope` ending at a `/`.
- [ ] *Plan 6:* **announce B99**: `PROTOCOL_BREAK` 98 → 99 (`src/operator.rs:49`). 99 is provisional: if the engine's
  B-number for the break is another, tell us and `kProtocolBreak` is set to it at 0.11.0.
- [ ] *Plan 6:* answer the SQE review questions of Plan 6 (B-number, marker split) and Plan 5.

**7. sqe-calibrate** (`crates/sqe-calibrate`)
- [ ] *Plan 5:* `temporal_type`, `SRIntrinsics` and `…WithCovariance` in `protocol.rs`, `publish.rs`, `local.rs`,
  `materialise.rs`, `sampler.rs`; the `/sis/relation/*` keys in `command.rs` and `client.rs:394`; `stream_item_*` in
  `sampler.rs` and `materialise.rs` (now `entry_id`). `SRCalibration` has optional `residual` and `covariance`, and a
  projection is a `CameraIntrinsics`.
- [ ] *Plan 6:* `TargetTrackingMessage` with `StreamHeader` and optional `Target.pose` (`sampler.rs`,
  `materialise.rs`, `tests/sampler_stream.rs`, `tests/materialise.rs`).

**8. Tests against the shared files**
- [ ] *Plan 4:* `test/golden/*.hex`, including `evo_v1/v2_Thing` and `evo_v1/v2_Holder`.
- [ ] *Plan 5:* the Plan 5 goldens and `share/tcn_schema/sqe/frame_ref.tsv` (70 rows, mandatory).
- [ ] *Plan 6:* `tcn_types_StreamHeader`, `tcnart_msgs_TargetTrackingMessage` (an absent pose),
  `tcnart_msgs_StreamInfo`, `tcnart_msgs_StreamDescriptorMessage_unversioned` (a tracker descriptor without
  `revision`), `evo_v1/v2_Boxed` (an unknown union case).

### sqe_client_cpp

- [ ] *Plan 6:* **a new `tcn_schema` pin**, `0.11.0-rc1`, and **the floor B99**: the library of this repository is the
  base, and `kProtocolBreak` (`TCN_SQE_PROTOCOL_BREAK`) is 99, provisional until 0.11.0. Rebuild against the rc1
  headers.
- [ ] *Plan 4:* the contract library sends `tcn_schema=0.11`; encode with `XCDRv2` + `DELIMIT_CDR2`, decode by the
  encapsulation id.
- [ ] *Plan 5:* sqe_client_cpp 0.7.0 (the break). Details in "The contract library" in §1:
  - **`FrameRef`:** build with `FrameRef::local/global/domain`, parse text with `FrameRef::parse`, print with
    `format()`; the text form is mandatory and your tests run the vector file. `@{domain}::{name}`, `<device>:::*`,
    the empty-client "shared" ref and `FrameRef::domain(...)`-as-id are gone.
  - **Handles and replies:** `Handle` exposes only the topics the reply gave (`data_topic()`, `status_topic()`,
    `plan_topic()`); read every reply with `reply_header(reply)` (`ReplyHeader`, `ok()`); `StopFn` returns
    `Result<ReplyHeader>`; handle the new errors (`ReplyRefused`, `HandleInvariantViolated`, `MissingSchemaVersion`,
    `MalformedCapability`, the frame-ref errors).
  - **Engine discovery:** `Capabilities::from_wire`, `has(capability::kSample)` and `meets_floor()` replace
    `protocol_break` comparisons; `ClientScope`, `engine_info_key`/`engine_info_pattern` ("no engine serves `<id>`").
  - **Removed:** `kDomainNodeSigil`, `kNodeIdSeparator`, `domain_node_id`, `parse_domain_node_id`, `LocalNodeId`,
    `GlobalNodeId`, `DomainNodeId`, `marker_prefix`, `marker_node_name`, `kMarkerNodeInfix`, `Status`,
    `status_from_ordinal`, `kContractBLevel`, `kSessionBLevel`, `kProtocolBLevel` and the two B-level macros.
  - **Keys:** `stream_sample_key()` (was `relation_sample_key()`); node and edge keys changed their strings only;
    `marker_announce_key(client, device, std::uint32_t id)`.
  - **Enums:** the C++ mirrors (`TriggerKind`, `ReferencePolicy`, `EvaluationMode`, `ReoptimisePolicy`,
    `FusionPolicy`, the status enums) are `std::int32_t`, `Unknown` = 0; unknown ordinals map to `Unknown`.
  - **Wire registry:** `wire_type.h` registers the 0.11 types; annotate every payload `…;tcn_schema=0.11`.
- [ ] *Plan 6:* a relation's data topic carries `SISRelationOutput` whose `targets` now start with `StreamHeader`
  and whose `Target.pose` is optional (`Target::pose()` returns an optional); the tracker descriptor is
  `info.tracker` (`tracker.h` documents it).
- [ ] *Plan 6:* if you link `pcpd_msgs` and use `tcnart_msgs`, link `tcn_schema::tcnart_msgs` yourself (no longer
  transitive).

### tcnart (v3)

(Counts: `v3/` only, outside `third_party/`.)

**1. Switch to rc1 and regenerate**
- [ ] *Plan 6:* **regenerate C++, C# and the JSON goldens from rc1:** refresh the vendored snapshot
  `v3/third_party/tcn_schema_idl` from `0.11.0-rc1`, re-run `idlgen`, and refresh
  `hosts/unity/Tools~/csharp-check/golden/types.json` and `tools/idlgen/tests/golden/default_schema_defs.json`. Take
  the type names from the three type maps.
- [ ] *Plan 3:* about 131 moved type names in 60–70 files under `v3/`: to `sqe_msgs::rpc` (80), `pcpd_msgs::rpc` (23,
  the parameter RPC), `sqe_msgs::msg` (16) and `tcn_types::msg` (12); idlgen's default package list gains
  `tcn_types` and `sqe_msgs`; the exclusion of the legacy rpc SpatialRelations types goes; the vendored snapshot
  needs the `src/dds/tcn` layout; goldens that list `device_hl2_msgs`/`device_orbbec_msgs` types need regenerating;
  remove the two stale device opt-ins (`TcnartV3Options.cmake:27`); `tools/idlgen/mapping.py:39` names the deleted
  `ComponentInfoReply`.
- [ ] *Plan 3:* **`Pose6DMapMessage` is deleted (XA7):** its registration (`wire/include/tcnart/wire/wire_type.hpp:105`),
  decoder (`features/src/pose/pose_wire.cpp:56,73,82,144`) and tests (`tests/sqe/test_sqe_conformance.cpp:297,310`,
  `tests/wire/test_cdr.cpp:117`, `tests/wire/test_wire_type.cpp:111`).
- [ ] *Plan 3:* **`conversation_msgs` headers are PascalCase:** `wire/include/tcnart/wire/conversation_msgs.hpp:17-22`
  includes `audio.h`, `common.h`, `rpc/discovery.h`, …, which become `Audio.h`, `Common.h`, `rpc/Discovery.h`.
- [ ] *Plan 2:* `Time_.idl` → `Time.idl`, `String_.idl` → `String.idl`; the C# fixed-array typedef names
  (`double__36`) disappear; update `v3/tools/idlgen/tests/test_parser.py` (lines 46, 98–103, 120),
  `v3/tools/idlgen/tests/test_gen.py` (line 147) and the C# golden (`Range`, `NavSatStatus`).

**2. Codecs** *(wire)*
- [ ] *Plan 4:* **XCDR2 in every codec:** the C# codec, the zero-copy encoders, the mesh-bitstream decoder and the C
  API (`docs/developers/interop.md:63` documents `DDS_CDR`). The time-ref encoder (`wire/src/timeref.cpp`) too:
  `TimeRef` keeps its shape, only the encapsulation and DHEADERs change.
- [ ] *Plan 6:* **the zero-copy video encoder** (`wire/include/tcnart/wire/video.hpp`, `wire/src/video.cpp`): the new
  `VideoStreamMessage` prefix is `StreamHeader` (a DHEADER, `capture_time_ns`, the presence bytes, `sequence_number`,
  an `SRFrameRef`), `pose`, then the `intrinsics` presence byte (and the union when present) instead of the four
  `camera_*` arrays, then `image_bytes`, `image`. Pin it with `tcnart_msgs_VideoStreamMessage_nominal.hex`
  (intrinsics absent) and `tcnart_msgs_VideoStreamMessage.hex` (present).
- [ ] *Plan 1:* check that no C++ code uses `operator==` on the parameter and service-info types to compare pointers
  (Plan 6 made them compare values).

**3. Enum values** (take them from the tables; `0` is `UNKNOWN`)
- [ ] *Plan 4:* the renames: media types, `XRGazeSpaceType`, `CoordinateAxis`, the parameter RPC enums.
- [ ] *Plan 5:* about 700 old SQE enumerator mentions in 33 files (`tests/features/test_sqe_relations.cpp`,
  `tests/sqe/*`, `tests/features/test_xr_user_frames_daemon.cpp`): the sqe enum map.
- [ ] *Plan 6:* `SensorType` → `StreamKind` + `PixelSemantics` (about 68 `SENSOR_TYPE_` mentions in 26 files);
  `VideoImageCompression` → `VideoCodec`; `ParameterValueType` (`UINT32` 7, `UINT64` 8, `ENUM` 9; the Unity client
  `hosts/unity/Runtime/TcnartParamsClient.cs:212-215` still names `PVT_UINT32`/`PVT_UINT64`).
- [ ] *Plan 6:* **Unreal without `CameraModelType` ordinals:** `hosts/unreal/Plugins/Tcnart/Source/TcnartRuntime/Private/TcnartRigCalibration.cpp:62-80`
  maps `CAMERA_MODEL_*` names to 0–4; Unity has `hosts/unity/Runtime/Rig/TcnartCameraModel.cs`. Read
  `CameraSensor.depth_parameters`/`color_parameters` as optional `CameraIntrinsics` with an `SRDistortionModel`.

**4. Removed fields** *(wire)*
- [ ] *Plan 4:* `MeshBitstreamMessage.origin_offset` and `is_compressed` (`mesh_bitstream_decode.cpp:61,77-78`);
  `DeviceContextReply.is_valid` (`TcnartRigCamera.cs:63` and Unreal: gate on `status`); `Target.quality` (XR writes a
  copy of `is_tracked`, and `features/src/camera/marker_publish_op.cpp:92` writes it).
- [ ] *Plan 4:* eye tracking: leave `scene_point` and `gaze_space` absent instead of writing (0,0) and `LOCAL`.
- [ ] *Plan 6:* `HandTrackingMessage.gaze_valid` → optional `gaze` (about 44 lines in 19 files: `capi/include/tcnart.h`,
  `capi/src/capi_xr.cpp`, `hosts/unity/Runtime/TcnartXrPublisher.cs`, the C# check).

**5. The SQE** *(wire)*
- [ ] *Plan 5:* **frame refs in configs as text:** `sqe/src/frame_text.cpp` is replaced by `tcn::sqe::FrameRef::parse`;
  a bare name is an error; configs, CLIs and the C ABI take `local:<fragment>/<name>`, `global:<name>` or
  `domain:<domain>/<name>`, with `[instances]` where a template is meant. `SQE_FRAME_INVALID` stays.
- [ ] *Plan 5:* **capability names instead of `protocol_break`** (`docs/internals/sqe.md`, `testing.md`):
  `Capabilities::has("sample")`; `SqeLink::sample` can fail fast with "no engine serves `<id>`".
- [ ] *Plan 5:* **marker announce by id:** `tcn::sqe::marker_node_name` goes (`sqe/src/control_thread.cpp:769`,
  `features/src/camera/camera_sqe_part.cpp:177`, `sqe/src/marker_ref.cpp`,
  `sqe/include/tcnart/sqe/{marker_ref,sqe_link,client_port}.hpp`, `vision/include/tcnart/vision/observations.hpp:23`);
  announce `{client}/cfg/marker/{device}/{marker_id}` and find marker nodes by `SISGraphFilter`.
- [ ] *Plan 5:* **output type:** the data topic is `SISRelationOutput`, no attachment (`PoseOrigin` and
  `sqe_client::PoseAttachment`, `sqe/include/tcnart/sqe/client_port.hpp:32-43`, and the baseline matching in
  `features/src/sqe/sqe_relations.cpp:59-63` go).
- [ ] *Plan 5:* **status handling:** about 100 uses of the removed status types in 21 files: gate on
  `reply_header(reply).ok()` and read `relation.state` and `outcome`.
- [ ] *Plan 5:* the rest of the Plan 5 tables: `SRGraph.name`, `SRNode.domain`/`materialization`, `SREdge`, the graph
  query union, the target alias split, `SISRelationTerms`; an absent `pose_covariance` is not zero.
  `docs/internals/sqe.md` and `conanfile.py:128` describe the 0.10 wire.

**6. Streams and descriptors** *(wire)* (about 128 lines of `StreamDescriptorMessage` in 65 files, 36 of
`buffer_info` in 26)
- [ ] *Plan 6:* **`StreamHeader`** in every stream message tcnart writes or reads (video, poses, targets, eye, hand,
  IMU, mesh): `capture_time_ns`, `sequence_number` from 1, `frame` as an `SRFrameRef`; leave `clock` absent when the
  descriptor's clock applies.
- [ ] *Plan 6:* **descriptors through `StreamInfo`:** video (`codec`, `semantics`, `h26x_profile` as profile only,
  `bitrate`, `codec_data` instead of the `h26x_codec_data` property: about 10 lines in 9 files), tracker
  (`marker_system` `family[:variant]` plus `marker_domain`, about 65 lines in 33 files, e.g.
  `features/src/camera/marker_publish_op.cpp`, `capi/src/capi_marker.cpp`), mesh, with `revision` bumped on a content
  change and `clock` a `ClockDomain`.
- [ ] *Plan 6:* **XR target ids in `targets`:** `features/src/xr/xr_wire.cpp:182` writes `target/<name>` properties
  and `features/src/pose/pose_wire.cpp:91` reads them; use `info.tracker.targets` (`SISTargetInfo`).
- [ ] *Plan 6:* **`Target.pose` optional** and per-target `capture_time_ns` in the pose and XR features.

**7. Persons, presence, parameters, voice** *(wire)*
- [ ] *Plan 6:* **HumanPose** (tcnart's `human_pose` feature, on the roadmap): `HumanPoseMessage`/`HumanShapeMessage`
  with `JointSet` + `joint_set_version`, `PersonState`, `JointSource`, optional `rotation`; the joint arrays from
  `include/tcn/joint_sets.h` (C++) and `share/tcn_schema/JointSets.cs` (Unity); refuse an unknown (set, version)
  pair; `PersonTrackingMessage` with `PersonTrackState` (values +1) and the descriptor's `PersonStreamInfo`.
  `tests/capi/test_capi.c` names `"coco17"`/`"halpe26"`.
- [ ] *Plan 6:* **presence `rpc_root`** (`engine/src/membership.cpp:596`) after `header`; `ServiceInfo.service_type`
  (was `service_info`, `engine/src/membership.cpp`, `tests/engine/test_membership.cpp`).
- [ ] *Plan 6:* **parameter `ENUM`:** `engine/src/param_service.cpp` and its tests use `struct_value` (4 files): the
  case is gone; an enum parameter is `PARAMETER_VALUE_TYPE_ENUM` with `enum_value` (`EnumEntry`), not a string.
  `ProcessInfo.started_at` is a `builtin_interfaces::msg::Time` (`engine/src/membership.cpp`, `engine/src/engine.cpp`).
- [ ] *Plan 6:* **`voice_agent.cpp`** (`features/src/voice/voice_agent.cpp`, the fake server
  `tests/voice/fake_app_server.cpp`): S1, no echo (the `SessionStart` built at `voice_agent.cpp:530-545` copies
  the formats, `language_hint` and `voice_hint` from discovery; send only `identity`, `allow_barge_in`, `enable_partial_stt`,
  `barge_in_mode`); S3/S4 (publish `PlaybackEvent` on `{session}/control/playback` when the server advertises
  `playback_events`; measured at the mixer); S5 (`SessionStateEvent.reason`); the `capabilities`; the
  `face_layouts`; drop `SessionConfigUpdate`, `SttRequest` and `EventState`; `turn_sample_index` in the downlink.

**8. Tests**
- [ ] *Plan 4/5:* the C# codec tests and the C++ wire tests against `test/golden/*.hex`; run `frame_ref.tsv`.
- [ ] *Plan 6:* the Plan 6 goldens, in particular `tcnart_msgs_VideoStreamMessage{,_nominal}`,
  `tcnart_msgs_HumanPoseMessage`, `tcnart_msgs_StreamDescriptorMessage`, `pcpd_msgs_Parameter`,
  `conversation_msgs_SessionStart`, `evo_v1/v2_Boxed`.

### HTA (holoscan-tcn-applications)

(Counts: outside `third_party/`, `docs/` and history.)

**1. Switch to rc1**
- [ ] *Plan 6:* depend on tcn_schema `0.11.0-rc1` (HTA links `tcn_schema::tcn_schema`, so `pcpd_msgs` losing its
  `tcnart_msgs` dependency needs no component change).
- [ ] *Plan 3:* about 45 moved type names in about 40 files: `sqe_msgs::rpc` (19), the parameter RPC `pcpd_msgs::rpc`
  (15), `tcn_types::msg` (7), `sqe_msgs::msg` (5); `tools/common/web_golden.hpp:16` includes the gone
  `tcnart_msgs/msg/Types.h` (the media enums are in `MediaTypes.h`).
- Plan 2: nothing to do. Plan 1: requiring `fast-cdr` yourself is no longer necessary.

**2. Codec** *(wire)*
- [ ] *Plan 4:* **`tcn::zenoh` serde:** `XCDRv2` + `DELIMIT_CDR2` for tcn types, `DDS_CDR` for ROS types, decode by
  encapsulation id.
- [ ] *Plan 4:* the FlatBuffers mirrors and the enum renames (media types, parameter RPC); set enum fields explicitly
  where the old default was relied on.

**3. Removed fields** *(wire)*
- [ ] *Plan 4:* `DeviceContextReply.{timestamp_offset, is_valid}` (`mock_capture_server.cpp:77-82`); `Person` role and
  orientation fields (`person_wire.cpp`); `MeshBitstreamMessage.{origin_offset, is_compressed}`
  (`libs/transport/mesh_bitstream/src/stream.cpp:35-36,68`); `target_origin` (`sqe_marker_camera_op.cpp:459`,
  `sqe_tracker_op.cpp:296`, `pose_publisher_op.cpp:229`) and `Target.quality` (`sqe_marker_camera_op.cpp:470`); also
  `operators/transport/pose_publish/src/pose_wire.cpp:49,114-115`,
  `operators/transport/zenoh_io/src/zenoh_cloud_publisher_op.cpp:151`, `libs/transport/zenoh/src/calibration.cpp:77`.
- [ ] *Plan 4:* the eye-tracking pass-through (`tcnart_mirror.cpp:127`): set `scene_point`/`gaze_space` only for
  samples that have them (the Tobii recordings).

**4. The SQE** *(wire)*
- [ ] *Plan 5:* **naming code goes** (about 320 uses in 31 files): in `libs/transport/sqe_link/src/sqe_link.cpp` the
  domain name helpers (lines 172-193, 1018) and the frame text; in
  `operators/sqe/sqe_camera/src/sqe_marker_camera_op.cpp:128` `tcn::sqe::marker_node_name(…)`. Domain nodes carry
  `SRNode.domain`; marker nodes are found by `SISGraphFilter`; announce markers by id.
- [ ] *Plan 5:* **FrameRef text in configs** (YAML, Python, `python/sqe_bindings.cpp`): `local:…`, `global:…`,
  `domain:…` (+ `[instances]`); run the vector file in the `sqe_link` tests.
- [ ] *Plan 5:* **capabilities instead of `protocol_break`** (`sqe_link.hpp:219-227,348`,
  `sqe_bindings.cpp:257-277,494-502`; about 90 uses in 16 files): `Capabilities`/`has(...)`, `meets_floor()`; gate
  on `rational_lens` and `marker_domain` (they answer
  `docs/migration/requests/2026-11-27-sqe-rational-lens-and-marker-spaces.md`).
- [ ] *Plan 5:* **output type:** decode `SISRelationOutput` on the reply's `data_topic`; take `status_topic`/
  `plan_topic` from the reply; the 13-byte attachment is gone (`tests/fake_sqe_engine.hpp`,
  `operators/transport/pose_publish/include/tcn/operators/pose_wire.hpp`).
- [ ] *Plan 5:* **status and enums** (about 170 old enumerator uses in 18 files): `header.status`; node and edge keys,
  `SISGraphQueryRequest` as a union, `SISTargetAlias` family/variant, `SRGraph.name` (15 uses), `stream_item_*` →
  `entry_id` (6).

**5. Every publisher and subscriber: `StreamHeader`** *(wire)*
- [ ] *Plan 6:* **`StreamHeader` on all ten stream messages**, `ImuMessage` and `EyeTrackingMessage` included: the
  serde (`libs/transport/zenoh/src/frame_serde.cpp`, `person_wire.cpp`), the pose wire
  (`operators/transport/pose_publish/src/pose_wire.cpp`), the web goldens (`tools/web/cdr_golden/cdr_golden.cpp`,
  `tools/common/web_golden.hpp`); `std_msgs::msg::Header` appears in 6 files. Leave `clock` absent when it is the
  descriptor's clock.
- [ ] *Plan 6:* **`group_id` on video and mesh** (and persons): one capture group across streams; a
  `HumanPoseMessage` matches its `PersonTrackingMessage` by it.
- [ ] *Plan 6:* **`VideoStreamMessage` per frame:** `camera_focal_length`, `camera_principal_point`,
  `camera_radial_distortion` and `camera_tangential_distortion` are gone from every frame. Write the optional
  `intrinsics` (a `CameraIntrinsics` pinhole case; the array → `fx fy cx cy` + OpenCV `distortion` map is in Plan 6)
  only when a frame's intrinsics differ from the descriptor's, and leave it absent otherwise (never zeros). Sites: the
  serde `libs/transport/zenoh/src/frame_serde.cpp:23-24,36-37` (behind `operators/transport/zenoh_io/src/zenoh_publisher_op.cpp`),
  `tools/common/web_golden.hpp`, the tests `libs/transport/zenoh/tests/test_serde.cpp` and
  `operators/transport/zenoh_io/tests/test_zenoh_io.cpp`. Pin the prefix with `tcnart_msgs_VideoStreamMessage_nominal.hex`.
- [ ] *Plan 6:* **`Target.pose` is optional, with a per-target `capture_time_ns`:** a target without a pose leaves
  `pose` absent (never identity); a target captured at another instant than the message carries its own
  `capture_time_ns`. Writers: `operators/transport/pose_publish/src/pose_wire.cpp:32-35` (`to_target`),
  `operators/transport/pose_publish/src/pose_publisher_op.cpp:230-250`, `operators/sqe/sqe_tracker/src/sqe_tracker_op.cpp:297`,
  `operators/sqe/sqe_camera/src/sqe_marker_camera_op.cpp:463-468` (they write through `t.pose()`, which is now an
  optional: set a `RigidTransform`). Readers: `pose_wire.cpp:84-87` (skip a target whose `pose` is absent).
- [ ] *Plan 6:* **`HandTrackingMessage.gaze_valid` → optional `gaze`:** no HTA code uses `HandTrackingMessage`
  (the grep finds none), so nothing to do unless a mirror is added; `libs/transport/tcnart_mirror/src/tcnart_mirror.cpp`
  passes eye tracking only (Plan 4's optional `gaze_space`/`scene_point`).

**6. Descriptors** *(wire)* (about 121 lines of `StreamDescriptorMessage` in 52 files; `buffer_info` about 23 lines in
14 files; `SENSOR_TYPE_` about 44 lines in 23 files)
- [ ] *Plan 6:* **video:** `info.video` with `codec` (H264/H265 no longer chosen by `h26x_profile`), `semantics`
  (colour, depth, IR, mask), `depth_units_per_meter` or `DEPTH_F32`, `codec_data` instead of the `h26x_codec_data`
  property (`operators/transport/zenoh_io/src/zenoh_publisher_op.cpp`), the nominal `intrinsics` (the per-frame
  arrays are gone from `VideoStreamMessage`: see step 5).
- [ ] *Plan 6:* **mesh:** `info.mesh` (`encoding`, `vertices_capacity`, `faces_capacity`, `camera_id_count` instead of
  `num_cameraids`); `BufferInfo` and its `semantic_type` go (`libs/transport/mesh_bitstream/src/stream.cpp:37`,
  `operators/transport/stream_receive/src/mesh_bitstream_subscriber_op.cpp:54`).
- [ ] *Plan 6:* **persons:** `info.persons` (`PersonStreamInfo`: `joint_set`, `joint_set_version`, `up`,
  `coast_limit_s`, `reid_window_s`, `smoothing`) instead of the properties
  (`operators/transport/person_publish/src/person_track_publisher_op.cpp:83-101`); the one-euro parameters stay in
  `properties`.
- [ ] *Plan 6:* **tracker targets:** `info.tracker` (`marker_system` `family[:variant]`, `marker_domain`, `targets`)
  instead of `{system}@{domain}` and the `target/<name>` properties
  (`operators/transport/pose_publish/src/pose_wire.cpp:95`); about 146 lines of `marker_system` in 26 files.
- [ ] *Plan 6:* `revision` bumped on a content change; `clock` a `ClockDomain`; `frame` an `SRFrameRef`.

**7. Persons, calibration, process info** *(wire)*
- [ ] *Plan 6:* **`PersonTrackingMessage`:** `group` → `header.group_id`, `world_frame` → `header.frame` (about 107
  lines in 31 files), `up` → `PersonStreamInfo.up`, `joint_set` the enum + `joint_set_version`; `Person.state` is
  `PersonTrackState` (values +1; `libs/transport/zenoh/include/tcn/zenoh/person_wire.hpp`); `previous_id` absent
  instead of 0. Joint arrays from `include/tcn/joint_sets.h`.
- [ ] *Plan 6:* **`CameraSensor`:** `depth_parameters`/`color_parameters` as optional `CameraIntrinsics` (the
  `CameraModel` mapping in Plan 6; `libs/transport/zenoh/src/calibration.cpp`), `frame_rate` a `double`, `revision`
  (bump on recalibration), `provenance` (`tool` e.g. `"hta.rig_calibration"`), `coord` (the camera axes of
  `camera_pose`). Python: `applications/capture/capture_server/python/capture_server.py`,
  `applications/calibration/rig_calibrator/python/rig_calibrator.py`.
- [ ] *Plan 6:* **the FlatBuffers mirrors** (`schemas/*.fbs`, 36 files; e.g. `person_track_list.fbs`,
  `camera_image.fbs`, `mesh.fbs`, `imu_sample.fbs`, `eye_tracking_sample.fbs`): the reshaped members, the new enum
  values.
- [ ] *Plan 6:* **`StreamRole`/`StreamMode`** in `StreamAnnouncement` and **`started_at`** as a `Time`
  (`libs/core/params_service/src/json_codec.cpp`, `parameter_service.cpp`, `tools/params/…`, about 13 files);
  parameter `ENUM` values (`json_codec.cpp`).
- [ ] *Plan 6:* **the Python dict builders** (`python/holoscan_tcn/zenoh_senders.py`, `capture_serving.py`,
  `applications/streaming/zenoh_listen/python/zenoh_listen.py`, the tests) for every reshaped type.
- [ ] *Plan 6:* **the `GraphReply.graph_json` schema document** (#37 T14): HTA delivers it; no wire change.

**8. Tests**
- [ ] *Plan 4/5/6:* the serde tests against `test/golden/*.hex` (the Plan 6 ones: `tcn_types_StreamHeader`,
  `tcnart_msgs_HumanPoseMessage`, `tcnart_msgs_StreamDescriptorMessage`, `tcnart_msgs_VideoStreamMessage`,
  `pcpd_msgs_Parameter`); `frame_ref.tsv` in `sqe_link`.

### tcn web controller

**1. Switch to rc1**
- [ ] *Plan 6:* generate `packages/sqe/src/schemas.ts` (and the `tcn-zenoh` layout mirrors) from rc1's `types.json`
  and `enums.json` (O7), instead of keeping them by hand.
- [ ] *Plan 3:* about 35 moved type names in about 15 files (`schemas.ts`, registries): `sqe_msgs::rpc` (23),
  `pcpd_msgs::rpc` (7), `tcn_types::msg` (6). `Pose6DMapMessage` and `RigidTransformMap(Item)` are deleted.
- Plan 2: nothing to do (ROS types are `"extensibility": "final"` in `types.json`, XCDR1 as before).

**2. Codec** *(wire)*
- [ ] *Plan 4:* **`packages/cdr`:** the XCDR2 reader and writer (`reader.ts:14` and the tests expect `00 01`); keep
  XCDR1 for ROS types.

**3. Removed fields and enum values** *(wire)*
- [ ] *Plan 4:* read: `Target.quality` (`apps/mcp/src/tools/srg.ts:487`); layout mirrors
  `packages/tcn-zenoh/src/streams/messages.ts:34-35,74,90,103-104,130,144,146` and `rig/messages.ts:35-66`.
- [ ] *Plan 4:* enum names and values in `schemas.ts` (media types, `CoordinateAxis`, `RPCResponseStatus`, the
  parameter RPC enums).

**4. The SQE** *(wire)* (counts approximate)
- [ ] *Plan 5:* **`schemas.ts`** for every reshaped type and enum value of Plan 5 (`SRNode`, `SREdge`, `SRGraph`, the
  views, `SRTransform`, `SRPoint2D`, `PinholeIntrinsics`, `SISReplyHeader` first, `SISRelationTerms`,
  `SISRelationStatus`, `SISStreamHandle`, `SISRelationOutput`, the query unions, `SISExplainRequest.subject`,
  `SISGraphChangeNotification`); about 415 enumerator mentions in 39 files; `client.ts:447` builds a graph query
  with `name_prefix`, `temporal_type`, `scope`, `node_type` and `"SIS_PP_WALKABLE"`.
- [ ] *Plan 5:* **graph filters (G3)** instead of name prefixes (`model.ts:17`, `edit.ts:110`, the SRG panels);
  **`SRRelationRef`** in views and paths; the N2a text form and the vector file in `packages/sqe` tests.
- [ ] *Plan 5:* **an absent `transform`** is not identity (`SrgScenePanel.tsx`, `calibration.ts`, `explain.ts`).
- [ ] *Plan 5:* **keys** (`packages/sqe/src/keys.ts:31`, `client.ts`, `sqe/{engine}/info`) and **handles** (read
  `status_topic`/`plan_topic` from the start reply).

**5. Streams, rig and discovery** *(wire)*
- [ ] *Plan 6:* **`schemas.ts` and the `tcn-zenoh` mirrors for every reshaped type and enum of Plan 6:**
  `StreamHeader` in `packages/tcn-zenoh/src/streams/messages.ts` (video, pose, mesh); `VideoStreamMessage.intrinsics`
  instead of the `camera_*` arrays (5 files); `StreamDescriptorMessage` with `StreamInfo` and without `BufferInfo`;
  `SensorType` → `StreamKind` + `PixelSemantics` (about 22 `SENSOR_TYPE_` mentions in 11 files),
  `VideoImageCompression` → `VideoCodec`; `CameraSensor` in `packages/tcn-zenoh/src/rig/messages.ts` (its
  `CameraModel`/`CameraModelType` go: optional `CameraIntrinsics`, `SRDistortionModel`); `ServiceInfo.service_type`
  and `rpc_root` in `packages/tcn-zenoh/src/messages.ts`; `ProcessInfo.started_at` as a `Time`
  (`packages/tcn-zenoh/src/discovery.ts`, about 14 lines in 8 files); `StreamRole`/`StreamMode`;
  `ParameterValueType` and the `ENUM` case (`packages/tcn-zenoh/src/params/wire.ts`).
- [ ] *Plan 6:* **previews from `StreamInfo`:** `packages/preview` picks the decoder by `info.video.codec` and
  `semantics` (not `sensor_type`/`image_compression`), takes the H.26x parameter sets from `codec_data` (not the
  `h26x_codec_data` property: about 12 lines in 8 files), and the nominal intrinsics from `info.video.intrinsics`
  (per-frame `intrinsics` override them).
- [ ] *Plan 6:* **persons from `share/tcn_schema/joint_sets.json`:** generate the TypeScript joint arrays from it, and
  **refuse an unknown (`joint_set`, `joint_set_version`) pair**.

**6. Tests**
- [ ] *Plan 5:* `packages/sqe/test/golden/*.cdr` and `.json` (47 files) regenerate against `test/golden/*.hex`.
- [ ] *Plan 6:* **the goldens:** `packages/tcn-zenoh/test/golden/` (`descriptor_*.json`, `video_*.json`, `header`,
  `pose6d`, `mesh_v1`, `presence*.json`, `describe_camera*.json`) regenerate for the new shapes; check the codec
  against the Plan 6 `test/golden/*.hex`.

### IVA (realtime conversational pipeline)

**1. Switch to rc1**
- [ ] *Plan 6:* sync `idl/` from `0.11.0-rc1` (the closure has `conversation_msgs/msg/Face.idl` now).
- [ ] *Plan 3:* about 24 moved type names in about 10 files, almost all the parameter RPC in `crates/tcn-wire`
  (`tcnart_msgs::rpc::*` → `pcpd_msgs::rpc::*`, 23), plus `tcn_types::msg` (1); the IDL sync takes
  `pcpd_msgs/rpc/Parameters.idl` and the PascalCase `conversation_msgs` files.
- [ ] *Plan 2:* the closure has `builtin_interfaces/msg/Time.idl` instead of `Time_.idl`
  (`crates/tcn-wire/tests/alignment.rs`, lines 36 and 85).
- Plan 5: nothing (IVA does not use the SQE protocol).

**2. Codec** *(wire)*
- [ ] *Plan 4:* `crates/tcn-wire`: write and check `00 09 00 00` and DHEADERs (`crates/tcn-wire/src/encap.rs:8` has
  `00 01`); `conversation_msgs` carry the encapsulation header too (E4; `crates/common-types/src/wire.rs:6` says plain
  CDR without one).

**3. Enum values**
- [ ] *Plan 4:* `MemoryScopeKind`, `InteractionMode`, `RetrievalPolicy`, `ExecutionPreference` and
  `InteractionActivityState` gain `…_UNKNOWN` at 0 (every value +1); `ErrorCode` → `ERROR_CODE_*` (same values).
- [ ] *Plan 4 + 6:* **the parameter RPC enums:** `RPCResponseStatus`, `ParameterAccess`, `ParameterSetStatus` gain
  `…_UNKNOWN` at 0 (Plan 4). `ParameterValueType` (`crates/tcn-wire/src/msgs.rs:48-59` still has the 0.10 values,
  `String = 0` … `Struct = 6`, `UInt32 = 7`, `UInt64 = 8`) becomes `UNKNOWN` 0, `STRING` 1 … `BOOL` 6, `UINT32` 7,
  `UINT64` 8, `ENUM` 9; `Struct` goes, `Enum` with an `EnumEntry` comes. `UInt32`/`UInt64` keep their numbers by
  accident only. Set `status` explicitly.
- [ ] *Plan 6:* the new enums: `BargeInMode` (**`OFF` = 1**, not the draft's 0), `SessionStateReason` (`NONE` = 0 is
  "no reason"), `PlaybackState`, `FaceRig`; `EventState` is deleted (about 20 files use it, e.g.
  `crates/stt-api/src/sherpa.rs`, `crates/app-server/src/main.rs`).

**4. Removed fields** *(wire)*
- [ ] *Plan 4:* the F5 fields: `TtsRequest` voice settings, `LlmRequest.{system_prompt, low_latency_mode}`,
  `RetrievalOptions` domain lists, `SttPartial.stability`, `StageMetric.gpu_utilization_pct`,
  `AudioOpusFrame.packet_loss_concealment_applied`, and `shm_ref`/`payload_kind` with `ShmRef`/`PayloadKind` (A2).

**5. #36 in the wire types** *(wire)* (the `conversation_msgs` mirrors are in `crates/common-types/src`: `control.rs`,
`audio.rs`, `stt.rs`, `llm.rs`, `tts.rs`, `metrics.rs`, `discovery.rs`, `wire.rs`; checked by
`crates/common-types/tests/idl_rust_schema_alignment.rs` and `serialization_roundtrip.rs`)
- [ ] *Plan 6:* `SessionStart` reshaped (S1); `SessionConfigUpdate` and `SttRequest` deleted; `SessionStateEvent.reason`
  before `status`; `PlaybackEvent` new; `VadEvent` reshaped (A1: `stream_position_ms`, optional
  `speech_probability`; `start_ms`, `end_ms`, `state` go); `state` dropped from `SttPartial`, `SttFinal`, `LlmPartial`,
  `LlmFinal`; `has_*` → optional (`SttFinal.confidence`, `LlmFinal.prompt_tokens`/`generated_tokens`; about 21 files);
  `InteractionActivity.header` first; `turn_sample_index` before the payload in `TtsAudioChunk` and `AudioOpusFrame`;
  `TtsDone.total_samples`/`sample_rate_hz` before `status`; `HealthStatus.component_name`; three trailing members of
  `AppServerInfo`; `FaceAnimationLayout`.

**6. Behaviour** *(wire)*
- [ ] *Plan 6:* **S4/S5 in the orchestrator and the clients** (`crates/session-orchestrator`,
  `crates/voice-chat-client`, `crates/voice-chat-core`, `crates/voice-chat-ffi`, `crates/text-chat-client`): the
  server consumes `PlaybackEvent` when it advertises `playback_events` (and falls back to `TtsDone.total_samples`
  otherwise); every `SessionStateEvent` carries its `reason`; `SESSION_STATE_PLAYING` lies between `TtsDone` and the
  client's `DRAINED` (S3). The clients stop echoing discovery values in `SessionStart`
  (`crates/voice-chat-client/src/runtime.rs:1148`).
- [ ] *Plan 6:* **`AppServerInfo`**: fill `capabilities` (`playback_events`, `barge_in_modes`, `wake_word` as they
  apply), `barge_in_mode` and `face_layouts`; honour `SessionStart.barge_in_mode` when you advertise
  `barge_in_modes`.
- [ ] *Plan 6:* **`turn_sample_index` in TTS and egress** (`crates/tts-api`, `crates/audio-egress/src/service.rs`): 0
  at the turn's first TTS sample, continuous across the turn's segments; egress rescales it to the downlink rate.
- [ ] *Plan 6:* **the VAD position** (`crates/stt-api/src/sherpa.rs`, `mock.rs`): `stream_position_ms` is ms since the
  first sample of the session's input audio stream; send `speech_probability` only from a VAD that has one (sherpa
  sends the constants 0.95/0.05: leave it absent).
- [ ] *Plan 6:* **the observer** (`crates/realtime-observation/src/main.rs`): drop the `SessionConfigUpdate` and
  `SttRequest` decoders; decode `PlaybackEvent`.

**7. Tests**
- [ ] *Plan 4/6:* the `tcn-wire` and `common-types` tests against `test/golden/*.hex`, in particular
  `conversation_msgs_SessionStart.hex` and `pcpd_msgs_Parameter.hex` (`ENUM` = 9); check the mirrors against
  `share/tcn_schema/types.json` (optional since Plan 1).

## 3. 0.11.x, decided

Additive changes decided for after 0.11.0 (decisions §7; not in any 0.11 plan). Each is appended and `@optional` (E8),
so it needs no lockstep cut-over.

- **#36 (IVA):** F4 `FaceAnimationChunk` and the F5 rules, with the capability `face_animation`; per-session voice and
  language overrides (#36 §6).
- **#37 (tcnart):** T2 `calibration_wanted`; T4's `FOV_ANGLES` case of `CameraIntrinsics`, and with it T7's XR case
  (`XRViewInfo`) as trailing optionals of `EyeStreamInfo`; T12 (`MeshStreamV2Message` in the IDL, the format-A
  type-name fix); T17 (covered by O7). Not placed by tcnart, so unchanged: T1 on `SRCalibration`, and T3
  (`CalibrationProvenance` reuses T1's shape for `CameraSensor` only).
- **#38 (HTA):** the surfel layout and the eye, hand and IMU descriptor fields; A4's typed calibration RPC
  (`CalibrationSetRequest`/`Reply`, `CalibrationChangedNotification`); A8's `StreamAnnouncementReply` fields and the
  canonical parameter JSON mapping; A9 `SRNodeRole` and `CoordinateSystem` presets (then `PersonStreamInfo.up` is
  deprecated); dynamic SQE targets from HumanPose (engine only).
- **Plan 5's V3:** the generated key table.
- **Joint sets:** closing the `SMPL_24` (joints 22–23) and `SMPLX_55` parent verification; a changed parent bumps that
  set's `version`.
- **At 0.11.0:** `kProtocolBreak` is set to the engine's B-number for the break (99 until then).
