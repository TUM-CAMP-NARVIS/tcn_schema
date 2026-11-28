# tcn_schema

The wire types of the TCN ecosystem: IDL for every message exchanged over Zenoh by tcnart, the spatial query engine
(SQE), the HTA applications, the tcn web controller and the IVA voice pipeline. They are compiled to C++ with
fastddsgen and fast-cdr, and shipped as a conan package. The repository also holds the header-only SQE contract
library (`tcn::sqe`).

> **0.11 is a breaking release; this is its release candidate, `0.11.0-rc1`** (CMake `0.11.0`, conan
> `0.11.0-rc1`, wire annotation `tcn_schema=0.11`).
> - What changes and why: [docs/proposals/0.11-decisions.md](docs/proposals/0.11-decisions.md).
> - What each consumer has to do, as a checklist per team:
>   [docs/migration/MIGRATION-0.11.0.md](docs/migration/MIGRATION-0.11.0.md) §2.

## Packages

| Package | Family | Content |
|---|---|---|
| 29 ROS 2 packages in tiers: `base` (builtin_interfaces, std_msgs, geometry_msgs, sensor_msgs, service_msgs) and `core` (nav_msgs, tf2_msgs, visualization_msgs, std_srvs, action_msgs, rcl_interfaces, …) built by default; `manipulation`, `navigation`, `perception` on request | ros2 | Generated from **ROS 2 Jazzy** by `tools/ros2`, with services and actions complete, `@final` (XCDR1, byte-compatible with ROS 2), and `share/tcn_schema/ros2_types.json` (ROS names, DDS names, RIHS01 hashes). See [src/dds/ros2/UPSTREAM.md](src/dds/ros2/UPSTREAM.md). |
| `tcn_types` | tcn | Shared types: geometry (`Vector3`, `Quaternion`, `RigidTransform`, `CoordinateSystem`, …), `UUID`, tracking targets and markers, frame references (`SRFrameRef`), clocks (`ClockDomain`), camera intrinsics, and `StreamHeader`, the first member of every stream message. |
| `sqe_msgs` | tcn | The spatial query engine protocol: the graph (`msg/Graph.idl`, `msg/Transforms.idl`) and the requests and replies (`rpc/Session`, `Mutations`, `Streams`, `Queries`, `Targets`, `Operator`, `Sample`). |
| `tcnart_msgs` | tcn | Streams (video, poses, eye/hand/person tracking, IMU), media types, the typed stream descriptor, presence, time reference, HumanPose and the joint-set enum. |
| `pcpd_msgs` | tcn | Camera sensors and calibration, mesh bitstreams, the service-controller RPC (discovery, process info, stream announcements) and the parameter RPC (`rpc/Parameters.idl`). |
| `conversation_msgs` | tcn | The realtime conversational pipeline's protocol. See below. |
| `sqe_contract`, `sqe_session` | header | The SQE contract library, `src/sqe/include/tcn/sqe/`. See below. |

- **Build order and dependencies:** every package, its family and its direct dependencies are listed in one table,
  [cmake/tcn_packages.json](cmake/tcn_packages.json), read by both CMake and `conanfile.py`. The dependencies follow
  the IDL `#include`s, and a test checks that.
- **Conan components:** each package is a component of `tcn_schema` (`tcn_schema::tcnart_msgs`, …).
- **IDL style:** [docs/idl-style.md](docs/idl-style.md).

## Build

```
conan install . -s build_type=Release --build missing
cmake --preset conan-release
cmake --build build/Release -j
```

- fastddsgen writes the generated code to `build/<config>/generated/<pkg>/<msg|rpc|action>/`, never into the source
  tree. The installed package has the headers at `include/<pkg>/<kind>/<File>.h`.
- Building needs Java (fastddsgen, via conan) and Python 3 (standard library only, for the tools below).
- ROS 2 tiers: `-DTCN_SCHEMA_ROS2_TIERS="base;core"` (default), or conan `-o tcn_schema/*:ros2_tiers=…`.

**Tests** (off by default):

```
cmake --preset conan-release -DTCN_SCHEMA_BUILD_MSG_TESTS=ON -DTCN_SCHEMA_BUILD_SQE_TESTS=ON
cmake --build build/Release -j
ctest --test-dir build/Release
```

| Test | What it checks |
|---|---|
| `roundtrip` | Every struct and union of every package survives encode → decode. It is generated from `types.json`. |
| `golden` | Fixed values encode to exactly the bytes in [test/golden/](test/golden/). These files are the shared contract for all hand-written codecs: XCDR1 for a ROS type, XCDR2 for tcn types, and two versions of one appendable type (`evo_v1_Thing`, `evo_v2_Thing`, from [test/idl/evo/](test/idl/evo/)) that prove decoding across versions in both directions, and a union pair (`evo_v1_Boxed`, `evo_v2_Boxed`) that proves a reader skips a union case it does not know. |
| `tcnart_msgs_*` | Hand-written round trips (PersonTracking, HumanPose, TimeRef). |
| `joint_sets` | The generated `tcn/joint_sets.h` agrees with the `JointSet` enum, and `find` refuses an unknown (set, version) pair. |
| `install_smoke`, `install_smoke_files` | The whole tree installs (`cmake --install`, what conan's `package()` runs), and the joint-set files are in place. |
| `idl_lint` | The IDL style rules, against the shrinking baseline `tools/idl/lint_baseline.txt`. |
| `idl_tools` | The Python tools' unit tests: `python3 -m unittest discover -s tools -p 'test_*.py' -t .` |
| `sqe_contract`, `sqe_must_not_compile.*` | The SQE contract library (`TCN_SCHEMA_BUILD_SQE_TESTS`). |

## Tools

All tools are standard-library Python and run under `python3 -I`.

| Command | Purpose |
|---|---|
| `python3 -I tools/idl_cli.py tables --dds-root src/dds --version <v> --out <dir>` | Writes `types.json` and `enums.json`: every declaration with its fully qualified name, members, extensibility, union cases and enumerator ordinals. They are installed to `share/tcn_schema/`, so other repositories can generate mirrors from them instead of keeping them by hand. |
| `python3 -I tools/idl_cli.py lint --dds-root src/dds --baseline tools/idl/lint_baseline.txt` | The IDL style rules of [docs/idl-style.md](docs/idl-style.md). `--update-baseline` rewrites the baseline, which may only shrink. |
| `python3 -I tools/idl_cli.py roundtrip --types <types.json> --package <pkg> --out <file.cpp>` | Generates a package's round-trip test (used by the build). |
| `python3 -I tools/idl_cli.py joint-sets --table src/dds/tcn/tcnart_msgs/msg/joint_sets.yaml [--check]` | Generates the joint-set files from the table (below); `--check` fails if a committed file differs, `--previous <old.yaml>` refuses a changed set whose `version` was not bumped. |
| `python3 -I tools/idl_cli.py moves --old <old src/dds> --new src/dds --spec <spec.json>` | Proves that a change of the IDL tree is exactly the one the spec lists (new, moved, reshaped, deleted types and removed members), and writes the type maps of `docs/migration/`. |
| `python3 -I tools/check_links.py .` | Checks the relative links in all Markdown files. |
| `python3 -I tools/ros2_cli.py refresh` / `check` | Regenerates the ROS 2 tree from the pinned Jazzy sources / fails if the committed tree differs (needs network). |

The installed package also contains `share/tcn_schema/packages.json` (the package table), the IDL itself under
`share/tcn_schema/dds/`, the 0.11 type maps (`type_map_0_11.json`, `type_map_0_11_sqe.json`,
`type_map_0_11_streams.json`) and the joint-set files (below).

## Streams: header, descriptor, joint sets

- **`StreamHeader`** (`tcn_types/msg/Stream.idl`) is the first member of the ten stream messages:
  `VideoStreamMessage`, `Pose6DMessage`, `TargetTrackingMessage`, `EyeTrackingMessage`, `HandTrackingMessage`,
  `ImuMessage`, `PersonTrackingMessage`, `HumanPoseMessage`, `HumanShapeMessage` (`tcnart_msgs`) and
  `MeshBitstreamMessage` (`pcpd_msgs`). It carries `capture_time_ns`, an optional `device_time_ns`, a gapless
  `sequence_number`, an optional `group_id` (one capture group across streams), an optional `clock` (absent: the
  descriptor's clock, never the deployment clock) and the `frame` (`SRFrameRef`).
- **The stream descriptor** (`tcnart_msgs/msg/StreamDescriptor.idl`), announced latched beside each stream:
  `StreamDescriptorMessage` is a common header (`stream_topic`, `stream_topic_schema`, an optional `revision`,
  `frame`, `clock`, `frame_rate`, `sensor_latency_ns`, `calib_topic`) plus `StreamInfo info`, a union over
  `StreamKind` with one `*StreamInfo` case per kind (video, tracker, persons, mesh; surfels, eye, hand and IMU are
  empty until 0.11.x), and `properties` for vendor extensions only. A changed `revision` means changed content;
  absent means compare the content.
- **Joint sets:** the only source is `src/dds/tcn/tcnart_msgs/msg/joint_sets.yaml`. `tools/idl_cli.py joint-sets`
  generates `tcnart_msgs/msg/JointSet.idl` (the enum), `tcnart_msgs/msg/JointSets.md` (the documentation) and the
  arrays, installed as `include/tcn/joint_sets.h` (C++, `tcn::joint_sets::find(set, version)`; on the include path
  of every component), `share/tcn_schema/JointSets.cs` (C#, `Tcn.Schema.JointSets.Find`) and
  `share/tcn_schema/joint_sets.json` (every other language). A consumer refuses an unknown
  (`joint_set`, `joint_set_version`) pair.

## The SQE wire contract

- **Types:** the spatial query engine's message types (SQE, spatial_query_engine_rs) are in
  `src/dds/tcn/sqe_msgs/` (`msg/Graph.idl`, `msg/Transforms.idl`, `rpc/*.idl`).
- **Contract library:** `src/sqe/include/tcn/sqe/` holds the header-only, zenoh-free `tcn::sqe` library: keys, wire
  type names, validation, relation requests, handles, leases. Its design note is
  [docs/design/2026-08-31-sqe-contract-library-design.md](docs/design/2026-08-31-sqe-contract-library-design.md).
- **Protocol description:** the canonical description is the engine's `docs/reference/protocol.md`, and each change
  is a B-entry in its `docs/reference/compatibility-breaks.md`. Since 0.11 the contract library knows one protocol
  floor, `tcn::sqe::kProtocolBreak` (`protocol.h`), instead of the 0.10 B-levels: 99 in rc1, provisional until
  0.11.0 sets it to the engine's B-number.
- **Replies:** every reply and notification starts with `SISReplyHeader` (`status: SISStatus`, `reason: SISReason`,
  `detail`, `engine_id`, `engine_epoch`); `tcn::sqe::reply_header(reply)` reads it, and a zeroed reply is not OK
  (every enum starts with `…_UNKNOWN`).
- **Frame references:** `tcn_types::msg::SRFrameRef { scope, owner, name, selector }` on the wire, and one text form,
  `local:<fragment>/<name>`, `global:<name>`, `domain:<domain>/<name>`, each optionally + `[instances]`
  (`tcn::sqe::FrameRef::parse`/`format`). The shared test vectors, `src/sqe/vectors/frame_ref.tsv` (installed as
  `share/tcn_schema/sqe/frame_ref.tsv`), are **mandatory** for every implementation, the engine's Rust side
  included.
- **Capabilities and scope:** the join reply, `op/info` and the unscoped `sqe/{engine}/info` carry
  `SISEngineCapabilities` (`protocol_break`, `capabilities` such as `sample`, `domain`, `rational_lens` and
  `marker_domain`, and the `client_scope`); a client gates a feature on `Capabilities::has(...)`, not on a number.
- **Key families:** `/sis/node/{update,remove}`, `/sis/edge/{update,remove}`, `/sis/stream/{start,stop,sample}`,
  `/sis/graph/query`, `/sis/pattern/query`, `/sis/target/{query,update}`, `/sis/{join,leave,alive}`,
  `{client}/cfg/marker/{device}/{marker_id}`, `sqe/{engine}/{alive/{epoch},info,op/*}`. A start reply names the data,
  status and plan topics (`SISStreamHandle`); a client never composes them. The data topic carries
  `SISRelationOutput`.
- **Moving from 0.10:** [docs/migration/MIGRATION-0.11.0.md](docs/migration/MIGRATION-0.11.0.md), Plan 5 (with the
  [type map](docs/migration/0.11-sqe-type-map.md) and the [enum map](docs/migration/0.11-sqe-enum-map.md)) and Plan 6
  (the tracker descriptor: `marker_system` and `marker_domain`; `StreamHeader` in `TargetTrackingMessage`).
- **Version check:** every SQE payload's encoding annotation carries the release, `;tcn_schema=0.11`
  (`tcn::sqe::kWireSchemaVersion`). Peers compare major.minor, so a minor bump is a lockstep rebuild of every peer.
- **C++ clients** use sqe_client_cpp, which builds on this library.
- **Since 0.11** these types are in their own package, `sqe_msgs`; the shared ones are in `tcn_types` (decisions §3).

## conversation_msgs: the voice-client protocol

- **What it is:** `src/dds/tcn/conversation_msgs/` is the realtime conversational pipeline's protocol:
  `conversation_msgs::msg` (common, control, audio, stt, llm, tts, metrics) and `conversation_msgs::rpc` (discovery,
  `AppServerInfo`). Type names, field names and field order are the pipeline's; since 0.11 enumerators carry the enum-name prefix and
  start with `…_UNKNOWN` (E6, O5), so their values differ from the pipeline's own enums.
- **Encoding:** since 0.11 XCDR2 with the encapsulation header, like every tcn type (decisions E4). Up to 0.10 they
  travelled as plain CDR without the header.

## Encoding

The encoding belongs to the package family (decisions E1–E3):

| Family | Extensibility | Encoding | Encapsulation (LE) |
|---|---|---|---|
| tcn (`tcn_types`, `sqe_msgs`, `tcnart_msgs`, `pcpd_msgs`, `conversation_msgs`) | `@appendable`; `@final` only for `Vector2`, `Vector3`, `Quaternion`, `RigidTransform`, `SRPoint2D` | XCDR2 (`D_CDR2`) | `00 09 00 00` |
| ros2 | `@final` | XCDR1 (`CDR`), byte-compatible with ROS 2 | `00 01 00 00` |

- **fast-cdr writers** of tcn types: `CdrVersion::XCDRv2`, then `set_encoding_flag(EncodingAlgorithmFlag::DELIMIT_CDR2)`,
  then `serialize_encapsulation()`.
- **Readers** pick the version from the encapsulation id, decode into a fresh object, skip trailing members they do
  not know, and give missing members their defaults. An unknown enumerator reads as the first one (E6); fast-cdr does
  not check this, so C++ readers range-check enums themselves.
- **New members after 0.11.0** are appended and `@optional`, unless 0, `""` or empty is a true reading (E8).
- **The details for hand-written codecs** are in
  [docs/migration/MIGRATION-0.11.0.md](docs/migration/MIGRATION-0.11.0.md) (Plan 4).

## Releases and migration notes

- All migration notes are indexed in [MIGRATION.md](MIGRATION.md), newest first. Each note says what changed on the
  wire and in the generated code, and what each consumer has to do.
- Recent releases:
  - **0.11.0-rc1** (break, lockstep): the release candidate of 0.11; see the checklist in
    [MIGRATION-0.11.0.md](docs/migration/MIGRATION-0.11.0.md) §2.
  - **0.10.7** (engine B94, additive): `SRG_SCOPE_DOMAIN` (`SR_NODE_SCOPE_DOMAIN` from 0.11), domain nodes `@{domain}::{name}`.
  - **0.10.6** (B93, additive): the relation sample.
  - **0.10.5:** person tracking for avatars, `HumanPoseMessage`, `TimeRef`.
  - **0.10.0** (B90): a wire break; evaluation modes and plans.
- **Design discussions and decisions:** [docs/proposals/](docs/proposals/), with the 0.11 decisions in
  [docs/proposals/0.11-decisions.md](docs/proposals/0.11-decisions.md).
- **Implementation plans:** [docs/superpowers/plans/](docs/superpowers/plans/).
