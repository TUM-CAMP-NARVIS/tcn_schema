# Migration to tcn_schema 0.10.6

**Additive. Nothing to change unless you want one value of a relation without starting a stream.** 0.10.6 adds the
types of the SQE's relation sample (engine B93). No existing type, field, enum value or key changes. The encoding
annotation stays `;tcn_schema=0.10`, so every 0.10.x peer talks to every other unchanged. 0.10.6 builds on 0.10.5
and includes its changes.

## What is new

`tcnart_msgs/rpc/SIS.idl`, section "Relation sample (engine B93)":

| key | request → reply |
|---|---|
| `{client}/sis/relation/sample` | `SISRelationSampleRequest` → `SISRelationSampleReply` |

| type | what it is |
|---|---|
| `SISRelationSampleInstant` | `SIS_SI_AT` (the request's `stamp_ns`), `SIS_SI_NOW` (what a `FIXED_RATE` trigger samples at: host now less the system and the route's sensor latency), `SIS_SI_LATEST` (the newest instant every input has data for; never extrapolates) |
| `SISRelationSampleRequest` | `observer`, `target`, `freshness_bias`, `instant`, `stamp_ns`, `wait_ms` (0 = 500, capped at 5000), `include_plan` |
| `SISRelationSampleStatus` | `SIS_SA_OK`, `SIS_SA_NO_ROUTE`, `SIS_SA_NO_VALUE`, `SIS_SA_TOO_OLD`, `SIS_SA_REJECTED`, `SIS_SA_UNAVAILABLE` |
| `SISSampleInput` | per route input: `topic`, `oldest_ns`/`newest_ns` buffered, and how far the instant was `extrapolated_ns` past the newest |
| `SISRelationSampleReply` | `status`, `detail`, `engine_id`/`engine_epoch`, the `relation` status a start would get (with `relation_detail`, `blocked_by`), the instant `stamp_ns`, `clock_domain`, `targets` (a `tcnart_msgs::msg::Target` per resolved instance, exactly as a stream publishes them), `inputs`, `cached`, `waited_ms`, and the `plan` when asked for |

The engine compiles the route a start would, evaluates it once at the instant from the same buffered samples and
with the same interpolation a stream uses (so a sample at t equals a stream's message at t), and answers. It starts
nothing: no handle, no status topic, no derived edge, no transaction-log record. It keeps the route and its inputs
warm for a short time after each sample, so a repeat is cheap. History is the engine's pose buffer (about 2 s, and
only while something reads the input): an `AT` further back is `SIS_SA_TOO_OLD`. The full semantics are the
engine's `docs/reference/protocol.md` §13.

The contract library adds `tcn::sqe::keys::kRelationSample`, `ClientId::relation_sample_key()`, the five wire names,
and the request key (`RequestKey<SISRelationSampleRequest>`: a `Query`, answered with `SISRelationSampleReply`), so
`SisSession::query_request` sends it.

## To adopt

Nothing, unless a client wants a one-off value: rebuild against 0.10.6 and send the request (sqe_client_cpp 0.6.0:
`Client::sample_relation`). An engine older than B93 does not answer the key, so the query closes with no reply; a
client that must tell that from an absent engine reads `op/info` for `protocol_break >= 93`. The engine's
`sqe-op sample` does the same from a terminal.
