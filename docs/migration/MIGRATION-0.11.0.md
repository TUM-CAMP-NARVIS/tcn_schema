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
| 1 | Layout and tooling: docs, out-of-tree generation, package table, `types.json`/`enums.json`, lint, round-trip tests | **none** | in review (branch `0.11-plan1-layout`) |
| 2 | ROS 2 tree regenerated from Jazzy, `@final`, service/action types, `ros2_types.json` | ROS types only (`Range.variance`) | planned |
| 3 | Moves and deletions: `sqe_msgs`, `tcn_types`, parameter RPC → `pcpd_msgs`, `custom/` → `tcn/`, deleted types | type names, removals | planned |
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

## 2. Per consumer

Each list collects the items of §1 that need action from that consumer, plus the decided changes still to come (with
their plan), so that each team can prepare.

### SQE engine (spatial_query_engine_rs, sqe-calibrate)

- *Plan 1:* optional:
  - check the Rust codec against `test/golden/*.hex`;
  - start checking the `messages.rs` registry against `share/tcn_schema/types.json`.

### sqe_client_cpp

- *Plan 1:* nothing to do.

### tcnart (v3)

- *Plan 1:*
  - Check that no C++ code uses `operator==` on the 14 parameter and service-info types listed in §1 (it compares pointers).
  - Optional: point the C# codec tests at `test/golden/*.hex`.

### HTA (holoscan-tcn-applications)

- *Plan 1:*
  - Check that no C++ code uses `operator==` on the 14 parameter and service-info types listed in §1 (it compares pointers).
  - Requiring `fast-cdr` yourself is no longer necessary (it is harmless to keep).

### tcn web controller

- *Plan 1:* optional:
  - check `@tcn-web/cdr` against `test/golden/*.hex`;
  - start generating `packages/sqe/src/schemas.ts` from `types.json` and `enums.json` (O7). The fields you asked for (member order, extensibility, `@optional`, bounds, union labels, fully qualified names) are in `types.json`.

### IVA (realtime conversational pipeline)

- *Plan 1:* optional:
  - check the `tcn-wire` mirrors against `types.json`;
  - the parameter types in `tcn-wire` are not affected by the C++ `operator==` defect.

## 3. Coming, decided (not yet on `main`)

Summary of what the later plans bring, so teams can plan their work. The details are in the decisions and will be
written into §1 and §2 as each plan lands.

- **Plan 2, ROS 2:**
  - the ROS copies are regenerated from Jazzy and marked `@final`;
  - they keep XCDR1 (`00 01`);
  - the file names follow upstream (`builtin_interfaces/msg/Time.idl`, not `Time_.idl`; include paths in IDL change
    accordingly);
  - service and action types are complete;
  - `share/tcn_schema/ros2_types.json` holds the ROS names and RIHS01 hashes.
- **Plan 3, moves:**
  - SQE types move to `sqe_msgs`, shared types to `tcn_types`;
  - the parameter RPC moves to `pcpd_msgs`;
  - `src/dds/custom/` becomes `src/dds/tcn/`;
  - the deleted packages and types are listed in decisions §4.

  C++ namespaces, include paths and wire type names change.
- **Plan 4, encoding:**
  - every tcn type is XCDR2 `@appendable`, with encapsulation `00 09`;
  - the unknown-enumerator and unknown-union-case rules (E6) and the default rule (E8) are normative;
  - fields nobody writes are removed (decisions §4);
  - the #35 SQE changes land.
- **Plan 5:**
  - the #36, #37 and #38 changes land;
  - HumanPose and the joint-set table;
  - `0.11.0-rc1`.
