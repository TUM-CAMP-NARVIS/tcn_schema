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
| 1 | Layout and tooling: docs, out-of-tree generation, package table, `types.json`/`enums.json`, lint, round-trip tests | **none** | in progress |
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
  - Delete a stale `<repo>/include/` left from older builds.
- **`tf2_msgs` LookupTransform moved from `msg/` to `action/`:** `include/tf2_msgs/action/LookupTransform.h` and
  `share/tcn_schema/dds/tf2_msgs/action/LookupTransform.idl`. This fixes the install path of an action type. None of
  the five consumers uses it.
- **Building tcn_schema:**
  - it builds in parallel again (the forced serial build is gone);
  - `ctest` works from the build root when `TCN_SCHEMA_BUILD_MSG_TESTS` or `TCN_SCHEMA_BUILD_SQE_TESTS` is on.

## 2. Per consumer

Each list collects the items of §1 that need action from that consumer, plus the decided changes still to come (with
their plan), so that each team can prepare.

### SQE engine (spatial_query_engine_rs, sqe-calibrate)

- *Plan 1:* nothing to do.

### sqe_client_cpp

- *Plan 1:* nothing to do.

### tcnart (v3)

- *Plan 1:* nothing to do.

### HTA (holoscan-tcn-applications)

- *Plan 1:* nothing to do.

### tcn web controller

- *Plan 1:* nothing to do.

### IVA (realtime conversational pipeline)

- *Plan 1:* nothing to do.

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
