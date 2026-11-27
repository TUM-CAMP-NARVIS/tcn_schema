# Migration to tcn_schema 0.11.0 (running log)

**Status: in preparation.** This is the log of every 0.11 change that a consumer has to follow. It is updated with each
0.11 pull request, so it is always current on `main`. When `0.11.0` is released, it becomes that release's migration
note.

- **What was decided and why:** [docs/proposals/0.11-decisions.md](../proposals/0.11-decisions.md). Where this log and
  the decisions differ, the decisions win; please report the difference.
- **Order of work:** five plans, each one or more PRs (decisions §10). Plan 1's plan file is
  [docs/superpowers/plans/2026-11-27-0.11-plan-1-layout-and-tooling.md](../superpowers/plans/2026-11-27-0.11-plan-1-layout-and-tooling.md).

| Plan | Content | Wire change | Status |
|---|---|---|---|
| 1 | Layout and tooling: docs, out-of-tree generation, package table, `types.json`/`enums.json`, lint, round-trip tests | **none** | merged (#41) |
| 2 | ROS 2 tree regenerated from Jazzy, `@final`, service/action types, `ros2_types.json`, tiers | ROS types only (`Range.variance`) | merged (#42) |
| 3 | Moves and deletions: `sqe_msgs`, `tcn_types`, parameter RPC → `pcpd_msgs`, `custom/` → `tcn/`, deleted types | type names, removals | in review (branch `0.11-plan3-moves`) |
| 4 | XCDR2 (E1–E9), field removals (F1–F5), #35 SQE changes | **yes** | planned |
| 5 | #36, #37, #38 changes, HumanPose and joint sets; `0.11.0-rc1` | **yes** | planned |

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
  - `tcnart_msgs_SRGraph`.

  The values that produced them are in `test/golden/golden_test.cpp`. Today they are XCDR1 (`00 01`). Plan 4
  regenerates them, deliberately, when tcn types move to XCDR2; Plan 4 also adds an appendable evolution pair. Point
  your codec tests at these files.
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
  `tcnart_msgs/rpc/Requests.h` are gone. Include the new ones from the type map. Most code includes a message
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

## 2. Per consumer

Each list collects the items of §1 that need action from that consumer, plus the decided changes still to come (with
their plan), so that each team can prepare.

### SQE engine (spatial_query_engine_rs, sqe-calibrate)

- *Plan 3:*
  - 82 moved type names in 14 files, mostly the `messages.rs` wire registry: `tcnart_msgs::rpc::SIS*` → `sqe_msgs::rpc::*` (74), `tcnart_msgs::msg::SR*` → `sqe_msgs::msg::*` (5), shared types → `tcn_types::msg::*` (3);
  - drop the `Pose6DMapMessage` tracker payload (`messages.rs:2081-2112`, decided) and the `CameraInfoMessage` test mention;
  - generate the renames from `share/tcn_schema/type_map_0_11.json`.
- *Plan 2:* nothing to do. Optional: `ros2_types.json` gives ROS names and hashes for showing ROS frames later.
- *Plan 1:* optional:
  - check the Rust codec against `test/golden/*.hex`;
  - start checking the `messages.rs` registry against `share/tcn_schema/types.json`.

### sqe_client_cpp

- *Plan 1:* nothing to do.

### tcnart (v3)

- *Plan 3:*
  - 131 moved type names in 69 files under `v3/`: to `sqe_msgs::rpc` (80), `pcpd_msgs::rpc` (23, the parameter RPC), `sqe_msgs::msg` (16) and `tcn_types::msg` (12);
  - idlgen's default package list gains `tcn_types` and `sqe_msgs`; the exclusion of the legacy rpc SpatialRelations types goes (the file is deleted);
  - the vendored snapshot needs the new `src/dds/tcn` layout;
  - goldens that list `device_hl2_msgs`/`device_orbbec_msgs` types need regenerating;
  - remove the two stale device opt-ins (`TcnartV3Options.cmake:27`).
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

### HTA (holoscan-tcn-applications)

- *Plan 3:*
  - 46 moved type names in 41 files: `sqe_msgs::rpc` (19), the parameter RPC `pcpd_msgs::rpc` (15), `tcn_types::msg` (7), `sqe_msgs::msg` (5);
  - HTA links `tcn_schema::tcn_schema`, so no component change is needed;
  - the FlatBuffers mirrors follow the renames.
- *Plan 2:* nothing to do. No HTA code includes the renamed headers or names the removed typedefs. `PointCloud2` and `Header` are byte-identical.
- *Plan 1:*
  - Check that no C++ code uses `operator==` on the 14 parameter and service-info types listed in §1 (it compares pointers).
  - Requiring `fast-cdr` yourself is no longer necessary (it is harmless to keep).

### tcn web controller

- *Plan 3:*
  - 36 moved type names in 16 files (`schemas.ts`, registries): `sqe_msgs::rpc` (23), `pcpd_msgs::rpc` (7), `tcn_types::msg` (6);
  - `Pose6DMapMessage` and `RigidTransformMap(Item)` are deleted; the fallback is already gone on `srg-dead-fields`.
- *Plan 2:* nothing to do. The type names in `types.json` are unchanged. ROS types are now `"extensibility": "final"` in `types.json` (XCDR1, as before).
- *Plan 1:* optional:
  - check `@tcn-web/cdr` against `test/golden/*.hex`;
  - start generating `packages/sqe/src/schemas.ts` from `types.json` and `enums.json` (O7). The fields you asked for (member order, extensibility, `@optional`, bounds, union labels, fully qualified names) are in `types.json`.

### IVA (realtime conversational pipeline)

- *Plan 3:*
  - 24 moved type names in 8 files, almost all the parameter RPC in `tcn-wire` (`tcnart_msgs::rpc::*` → `pcpd_msgs::rpc::*`, 23), plus `tcn_types::msg` (1);
  - the IDL sync takes `pcpd_msgs/rpc/Parameters.idl` instead of `tcnart_msgs/rpc/Requests.idl`, and the PascalCase `conversation_msgs` files.
- *Plan 2:* when you next sync `idl/` from tcn_schema:
  - the closure has `builtin_interfaces/msg/Time.idl` instead of `Time_.idl`;
  - update `crates/tcn-wire/tests/alignment.rs` (lines 36 and 85), which name `Time_.idl`.
- *Plan 1:* optional:
  - check the `tcn-wire` mirrors against `types.json`;
  - the parameter types in `tcn-wire` are not affected by the C++ `operator==` defect.

## 3. Coming, decided (not yet on `main`)

Summary of what the later plans bring, so teams can plan their work. The details are in the decisions and will be
written into §1 and §2 as each plan lands.

- **Plan 4, encoding:**
  - every tcn type is XCDR2 `@appendable`, with encapsulation `00 09`;
  - the unknown-enumerator and unknown-union-case rules (E6) and the default rule (E8) are normative;
  - fields nobody writes are removed (decisions §4);
  - the #35 SQE changes land.
- **Plan 5:**
  - the #36, #37 and #38 changes land;
  - HumanPose and the joint-set table;
  - `0.11.0-rc1`.
