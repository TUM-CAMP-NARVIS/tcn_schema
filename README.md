# tcn_schema

The wire types of the TCN ecosystem: IDL for every message exchanged over Zenoh by tcnart, the spatial query engine
(SQE), the HTA applications, the tcn web controller and the IVA voice pipeline. They are compiled to C++ with
fastddsgen and fast-cdr, and shipped as a conan package. The repository also holds the header-only SQE contract
library (`tcn::sqe`).

> **0.11 is in preparation: a breaking release.**
> - What changes and why: [docs/proposals/0.11-decisions.md](docs/proposals/0.11-decisions.md).
> - What each consumer has to do, updated with every 0.11 pull request:
>   [docs/migration/MIGRATION-0.11.0.md](docs/migration/MIGRATION-0.11.0.md).

## Packages

| Package | Family | Content |
|---|---|---|
| `builtin_interfaces`, `std_msgs`, `geometry_msgs`, `sensor_msgs`, `shape_msgs`, `tf2_msgs`, `rosgraph_msgs`, `statistics_msgs`, `unique_identifier_msgs` | ros2 | Copies of the ROS 2 interfaces (rolling, late 2022). They are regenerated from ROS 2 Jazzy in 0.11 (decisions §5). |
| `tcnart_msgs` | tcn | Streams (video, poses, eye/hand/person tracking, IMU), stream descriptors, presence, logging, the parameter RPC, and the SQE protocol (`SpatialRelations.idl`, `rpc/SIS.idl`). |
| `pcpd_msgs` | tcn | Camera sensors and calibration, mesh bitstreams, and the service-controller RPC (discovery, process info, stream announcements). |
| `conversation_msgs` | tcn | The realtime conversational pipeline's protocol. See below. |
| `device_hl2_msgs`, `device_orbbec_msgs` | tcn | Device control (HoloLens 2, Orbbec). Deleted in 0.11 (decisions §4). |
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

**Tests** (off by default):

```
cmake --preset conan-release -DTCN_SCHEMA_BUILD_MSG_TESTS=ON -DTCN_SCHEMA_BUILD_SQE_TESTS=ON
cmake --build build/Release -j
ctest --test-dir build/Release
```

| Test | What it checks |
|---|---|
| `roundtrip` | Every struct and union of every package survives encode → decode. It is generated from `types.json`. |
| `golden` | Fixed values encode to exactly the bytes in [test/golden/](test/golden/). These files are the shared contract for all hand-written codecs. |
| `tcnart_msgs_*` | Hand-written round trips (PersonTracking, HumanPose, TimeRef). |
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
| `python3 -I tools/check_links.py .` | Checks the relative links in all Markdown files. |

The installed package also contains `share/tcn_schema/packages.json` (the package table) and the IDL itself,
under `share/tcn_schema/dds/`.

## The SQE wire contract

- **Types:** the spatial query engine's message types (SQE, spatial_query_engine_rs) are in
  `src/dds/custom/tcnart_msgs/rpc/SIS.idl` and `msg/SpatialRelations.idl`.
- **Contract library:** `src/sqe/include/tcn/sqe/` holds the header-only, zenoh-free `tcn::sqe` library: keys, wire
  type names, validation, relation requests, handles, leases. Its design note is
  [docs/design/2026-08-31-sqe-contract-library-design.md](docs/design/2026-08-31-sqe-contract-library-design.md).
- **Protocol description:** the canonical description is the engine's `docs/reference/protocol.md`, and each change
  is a B-entry in its `docs/reference/compatibility-breaks.md`.
- **Version check:** every SQE payload's encoding annotation carries the release, `;tcn_schema=0.10`
  (`tcn::sqe::kWireSchemaVersion`). Peers compare major.minor, so a minor bump is a lockstep rebuild of every peer.
- **C++ clients** use sqe_client_cpp, which builds on this library.
- **0.11** moves these types into their own package, `sqe_msgs`; the shared ones go to `tcn_types` (decisions §3).

## conversation_msgs: the voice-client protocol

- **What it is:** `src/dds/custom/conversation_msgs/` is the realtime conversational pipeline's protocol:
  `conversation_msgs::msg` (common, control, audio, stt, llm, tts, metrics) and `conversation_msgs::rpc` (discovery,
  `AppServerInfo`). Type names, field names, field order and enum order are the pipeline's.
- **Encoding today:** these types travel as plain CDR: OMG CDR v1 little-endian *without* the 4-byte encapsulation
  header, zenoh encoding `application/cdr`. Every other package keeps the encapsulation (since 0.10.2,
  [MIGRATION-0.10.2.md](docs/migration/MIGRATION-0.10.2.md)).
- **In 0.11** they get the encapsulation header and XCDR2 like every tcn type (decisions E4).

## Releases and migration notes

- All migration notes are indexed in [MIGRATION.md](MIGRATION.md), newest first. Each note says what changed on the
  wire and in the generated code, and what each consumer has to do.
- Recent releases:
  - **0.10.7** (engine B94, additive): `SRG_SCOPE_DOMAIN`, domain nodes `@{domain}::{name}`.
  - **0.10.6** (B93, additive): the relation sample.
  - **0.10.5:** person tracking for avatars, `HumanPoseMessage`, `TimeRef`.
  - **0.10.0** (B90): a wire break; evaluation modes and plans.
- **Design discussions and decisions:** [docs/proposals/](docs/proposals/), with the 0.11 decisions in
  [docs/proposals/0.11-decisions.md](docs/proposals/0.11-decisions.md).
- **Implementation plans:** [docs/superpowers/plans/](docs/superpowers/plans/).
