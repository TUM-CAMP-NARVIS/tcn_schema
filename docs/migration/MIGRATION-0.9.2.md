# Migration to tcn_schema 0.9.2

**Additive. Nothing to change unless you write an operator tool.** 0.9.2 adds
the types of the SQE operator surface (engine B82). No existing type, field,
enum value or key changes. The encoding annotation stays `;tcn_schema=0.9`
(`kWireSchemaVersion` carries major.minor).

## What is new

`tcnart_msgs/rpc/SIS.idl`, section "B82 (0.9.2): the operator surface", for
keys in the engine's own namespace, `sqe/{engine_id}/op/*`:

| key | request → reply |
|---|---|
| `op/info` | `SISEngineInfoRequest` → `SISEngineInfoReply` |
| `op/fragments` | `SISFragmentListRequest` → `SISFragmentListReply` |
| `op/graph/query` | `SISOperatorGraphQueryRequest` → `SISGraphQueryReply` (existing) |
| `op/update` | `SISOperatorMutationRequest` → `SISMutationReply` (existing) |
| `op/changes` | `SISGraphChangeNotification`, published |

Supporting types: `SISOperatorRef`, `SISStoreHealth`, `SISReadLimits`,
`SISFragmentSummary`, `SISGraphChangeKind`, `SISOperatorMutationKind` and the
union `SISOperatorMutation`.

The contract library (`src/sqe/`) registers the wire names of all of them
(`wire_type.h`) and builds the keys: `engine_operator_key(engine_id,
kOperatorInfo)` and the other `kOperator*` suffixes (`topics.h`).

## What it is for

An operator tool (the SRG frontend) reads and edits the graph through the
engine instead of the database: list the fragments, read them, edit a
fragment it does not own, and follow changes. Answered only inside the
engine's client scope. The engine's `docs/reference/protocol.md` §10 has the
semantics; its `fixtures/wire/` has golden CDR bytes for every type.

## To adopt

- **Clients (tcnart, HTA, sqe_client_cpp users):** nothing. Moving the
  requirement to 0.9.2 is harmless and optional.
- **Operator tools:** require `tcn_schema/0.9.2@artekmed/stable`.
