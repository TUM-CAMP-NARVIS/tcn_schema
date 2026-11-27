# Migration to tcn_schema 0.9.3

**Additive. Nothing to change unless you want to ask the engine how a relation
is computed.** 0.9.3 adds the types of the SQE's explain query (engine B89). No
existing type, field, enum value or key changes. The encoding annotation stays
`;tcn_schema=0.9`.

## What is new

`tcnart_msgs/rpc/SIS.idl`, section "B89 (0.9.3): explain":

| key | request → reply |
|---|---|
| `sqe/{engine_id}/op/explain` | `SISExplainRequest` → `SISExplainReply` |

- `SIS_EX_RELATION` is a dry run. It compiles an observer/target pair exactly
  as `SISRelationStreamStartRequest` would, and starts nothing.
- `SIS_EX_STREAM` takes a handle and returns the route that stream is running
  now, plus what the engine knows about its inputs (`SISPlanLive`).

The answer is a `SISTranslationPlan`:
- **`instances`:** the op chains (`SISPlanOp`: static transform, live input,
  convention swap, projection), one per materialized target;
- **`inputs`:** the live inputs, with everything needed to read them (topic,
  payload type, device convention, entry, sensor latency, clock domain);
- **`hops`:** where each hop's value comes from: relation, temporal type,
  derived-from, weight and stated covariance.

Composing the ops, with every live input sampled at one instant, gives what the
engine publishes for that instant. The plan format is versioned
(`plan_version`, now 1), and a later release will run plans on the device as
well.

The contract library registers every new wire name (`wire_type.h`) and the key
suffix `kOperatorExplain` (`topics.h`).

## To adopt

- **Clients and operator tools that want explain:** require
  `tcn_schema/0.9.3@artekmed/stable`. The query is open to every client, scoped
  to the engine's client scope, and charged to the operator bucket of
  `who.requester`.
- **Everyone else:** nothing. Moving the requirement to 0.9.3 is harmless and
  optional.
