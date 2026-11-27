# Migration to tcn_schema 0.10.0

**A wire break.** Every peer must be rebuilt against 0.10.0 and every engine moved to B90 at the same time.

The encoding annotation is now `;tcn_schema=0.10`. B76 compares major.minor across the whole release, so a 0.10
engine refuses **every** 0.9-annotated message, and a 0.9 peer refuses every 0.10 reply. That includes sensor pose
streams, descriptors, joins and mutations from peers that never start a stream. The refusal is loud on both sides:
the engine answers a query `REJECTED` with "payload declares tcn_schema 0.9 but this engine reads 0.10.x; …" and logs
a pose stream once per topic as "Rejecting payloads on '<topic>': …", and a client's `check_declared_type` fails with
`SchemaVersionMismatch` ("encoding declares another tcn_schema version"). Nothing decodes silently wrong.

What changed is one feature, the evaluation mode of a relation stream (engine B90, the local-translation design's
phase 3), plus an engine epoch where route epochs travel.

## What changed

`tcnart_msgs/rpc/SIS.idl`:

| type | change |
|---|---|
| `SISEvaluationMode` | **new** enum: `SIS_EV_ENGINE` (0, default), `SIS_EV_LOCAL`, `SIS_EV_BOTH` |
| `SISRelationStreamStartRequest` | **appended** `SISEvaluationMode evaluation` |
| `SISRelationStreamReply` | **appended** `SISTranslationPlan plan` |
| `SISRelationStreamStatusNotification` | **appended** `uint64 engine_epoch` |
| `SISTranslationPlan` | **inserted** `uint64 engine_epoch` after `route_epoch`; `plan_version` is now 2 |

The plan types (`SISTranslationPlan` and its parts) moved up in the file, ahead of `SISRelationStreamReply`; their
names and the explain types are otherwise unchanged. The contract library has
`tcn::sqe::EvaluationMode` and `RelationStreamRequest::make(..., freshness_bias, evaluation)`. The evaluation mode is
one of the request's terms (`same_terms_as`). It also has `plan_topic_for(output_topic)` (`{handle}/plan`) and
`kWireSchemaVersion = "0.10"`.

### The evaluation mode

| mode | the engine | the client |
|---|---|---|
| `SIS_EV_ENGINE` | composes the relation and publishes it on the handle, as before | consumes the handle |
| `SIS_EV_LOCAL` | routes and publishes the **plan**; runs no pipeline, subscribes no input, publishes no derived edge, publishes nothing on the handle | composes the relation from the plan's inputs (`sqe-local`, `sqe_local.h`) |
| `SIS_EV_BOTH` | both | either, e.g. local for latency with the engine's output as a reference |

- **The mode is part of the stream's identity**, like `trigger` and `freshness_bias`. A second request for the same
  relation in another mode is refused (`SIS_RS_REJECTED`, naming the conflict), not attached.
- **A `LOCAL` stream usually sends `freshness_bias` 1.** The route then uses only measured relations, so every input
  is a sensor stream. With 0, an input may be another engine stream's output: a hybrid route.
- **Where the plan arrives:**
  - in the start reply's `plan`, when the stream has a route;
  - then on `{handle}/plan` after every route change (a re-route, a parked stream resuming). A parked stream
    publishes an empty plan (no instances), and its status says why;
  - a get on `{handle}/plan` answers the last one, like `{handle}/status`.
- **Ordering:** by `(engine_epoch, route_epoch)`. `route_epoch` restarts at 0 when the engine restarts; the new
  `engine_epoch` on the status notification and in the plan is the engine's alive-key epoch.

## For tcnart (v3)

1. **Dependencies:** move `tcn_schema` to `0.10.0@artekmed/stable` and `sqe_client_cpp` to `0.4.0@artekmed/stable`
   (it carries 0.10.0).
2. **Wire layer** (`v3/wire`): it takes the version from `tcn::sqe::kWireSchemaVersion`, so a rebuild moves it to
   `0.10`. Check the tests that spell `0.9` (`tests/wire/test_wire_type.cpp`, `test_discovery.cpp`) and the
   comments in `wire_type.hpp` that name "tcn_schema 0.9 encoding rules".
3. **Unity C# types:** regenerate them from the IDL (`v3/tools/idlgen`, `sync_idl.py` then `gen_cs.py`).
   `SISRelationStreamStartRequest.cs`, `SISRelationStreamReply.cs` and `SISRelationStreamStatusNotification.cs`
   change, `SISEvaluationMode.cs` is new, and the plan types come along with the reply. Hand-written code that
   builds a start request must now set `evaluation` (`SIS_EV_ENGINE` for today's behaviour).
4. **`tcnart::sqe` relations** (`features/src/sqe/sqe_relations.cpp`): requests go through sqe_client_cpp, so they
   keep working with the default `Engine`. To evaluate locally, pass `EvaluationMode::Local` (with
   `freshness_bias` 1) and feed `sqe_local`; see sqe_client_cpp's `MIGRATION-0.4.0.md`.
5. **Status:** a listener that orders notifications by `route_epoch` should now order by
   `(engine_epoch, route_epoch)`; tcnart's restart handling (B74) already treats a new engine epoch as a new
   stream.
6. **All of tcnart's publishers** (cameras, markers, XR poses) are refused by a 0.10 engine until rebuilt. Roll
   tcnart and the engine out together.

## For the holoscan apps (HTA)

1. **Dependencies:** `tcn_schema/0.10.0@artekmed/stable` and `sqe_client_cpp/0.4.0@artekmed/stable` in
   `conanfile.py`. If you run the embedded engine (`sqe_engine`), use a release built on 0.10.0.
2. **`sqe_link`** goes through sqe_client_cpp's `acquire_relation`, so it builds unchanged and keeps the `Engine`
   mode. The new fields have defaults; nothing else in it names the changed types.
3. **The marker camera** (`operators/sqe/sqe_camera`) and every other publisher annotate with the contract
   library's version, so they move to `0.10` with the rebuild. Un-rebuilt, they are refused by a 0.10 engine.
4. **`python/holoscan_tcn/sqe_e2e.py`** starts `/opt/tcn/sqe/bin/sqe-daemon`. Stage a B90 daemon there with the
   rebuild (`deploy/test-engine/install.sh` in the engine repo), or the end-to-end tests see every request refused.
5. **The home-lab overlay** (the reason for the break): acquire the camera-to-marker relation with
   `EvaluationMode::Local` and `freshness_bias` 1. Subscribe to the plan's input topics on the app's own session,
   and sample `sqe_local` at each video frame's exposure stamp. sqe_client_cpp 0.4.0 does the plan handling; the
   full integration is in its migration note.

## For other peers

- **Rust:** the engine's hand-written binding (`crates/sqe-core/src/messages.rs`) is the reference.
- **Browser:** `tcn_web_controller`'s `@tcn-web/cdr`. Re-run it against the engine's `fixtures/wire/` golden
  files.
- **tcn_tracking** and any sensor-only publisher: rebuild against 0.10.0. No code change.
