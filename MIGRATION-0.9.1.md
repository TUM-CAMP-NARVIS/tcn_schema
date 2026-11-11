# Migration to tcn_schema 0.9.1

**A re-release, no wire change against what you most likely already use.**
0.9.1 is `main` as the `tcn_schema/0.9.0@artekmed/stable` Conan package has
shipped it since 2026-11-06 (recipe revision `da414803…`), under a version
number of its own and with a git tag that matches it.

## Why

The git tag `v0.9.0` (2026-11-01) was cut **before** the SQE protocol work
landed. Four recipe revisions were then uploaded under the same version
number, so "0.9.0" named two incompatible things:

| what you have | contains | against the current engine |
|---|---|---|
| git tag `v0.9.0`, or Conan revision `9650c755…` | Part 1 of MIGRATION-0.9.0 only | **wire-incompatible**: no B73–B78 types (`SIS.idl`), mutations still puts |
| Conan revision `da414803…` (latest 0.9.0) | Parts 1, 2, 2a and C1 of MIGRATION-0.9.0 | compatible |
| **0.9.1** | identical to `da414803…` | compatible |

Conan resolves a bare `tcn_schema/0.9.0@artekmed/stable` to its latest
revision, so most builds already got the right one. A lockfile, a pinned
revision, or a build from the git tag did not.

## What changed since tag `v0.9.0`

Everything is already described in [MIGRATION-0.9.0.md](./MIGRATION-0.9.0.md):

- **Part 2** — SQE protocol, engine B73–B78 (`SIS.idl`, `StreamDescriptor.idl`,
  the contract library under `src/sqe/`).
- **Part 2a** — engine B79–B80: behaviour only, no IDL change.
- **C1** — `CompressionType_JpegLs` appended to `VideoImageCompression`.
- `status.h`: the comment on `SIS_RS_CLOCK_DOMAIN_MISMATCH` (emitted since
  engine B77).

Nothing was added for 0.9.1 itself. The encoding annotation stays
`;tcn_schema=0.9` (`wire_type.h`, `kWireSchemaVersion`): it carries
major.minor, and the wire did not change.

## To adopt

- Require `tcn_schema/0.9.1@artekmed/stable` (or `[>=0.9.1 <0.10]`) instead of
  `0.9.0`. No code change.
- Drop any lockfile or pinned recipe revision of 0.9.0 that predates
  2026-11-06.
- If you built from the git tag `v0.9.0`: read MIGRATION-0.9.0 Part 2. Your
  build is the incompatible one.
