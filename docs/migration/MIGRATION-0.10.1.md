# Migration to tcn_schema 0.10.1

**Additive. Nothing to change unless you want to start or stop the SQE's
transaction log.** 0.10.1 adds the types of the engine's transaction-log op
(engine B92). No existing type, field, enum value or key changes. The encoding
annotation stays `;tcn_schema=0.10`, so 0.10.0 and 0.10.1 peers talk to each
other unchanged.

## What is new

`tcnart_msgs/rpc/SIS.idl`, section "Transaction log (engine B92)":

| key | request → reply |
|---|---|
| `sqe/{engine_id}/op/txlog` | `SISTransactionLogRequest` → `SISTransactionLogReply` |

- `SIS_TL_START` with a `label` starts a log (stopping a running one first);
  `SIS_TL_STOP` stops it; `SIS_TL_STATUS` changes nothing.
- The reply says whether a log is running, its label, its file on the engine's
  host, when it started, and how many records it has written and dropped.

The log itself is a JSON Lines file the engine writes: a snapshot of the
fragments, streams and targets, then every structural change in order (graph
writes with their outcome, stream requests, replies, routes and status
transitions). Its record format is in the engine's
`docs/reference/protocol.md` §12. The engine logs only when started with
`--transaction-log-dir`; otherwise a start is answered `SIS_GQ_REJECTED`.

The contract library registers the three new wire names and the
`kOperatorTxlog` key suffix (`topics.h`).

## To adopt

Nothing, unless a tool should control the log: rebuild it against 0.10.1 and
send the request. The engine's `sqe-op txlog start|stop|status` does the same
from a terminal.
