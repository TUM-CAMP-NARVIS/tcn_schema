# TCNART DDS Interfaces

Most idls are copied from ROS2 (12/2022) for basic interoperability.
The interface is then augmented with pcpd specific topic types.

### Build:

```
conan install . -s build_type=Debug --build missing --build outdated
cmake --preset conan-debug
cmake --build ./build/Debug --config Debug
```

### conversation_msgs: the voice-client protocol

`src/dds/custom/conversation_msgs/` is the realtime conversational pipeline's protocol (its IDL, formerly module
`nv::voice::v1`): `conversation_msgs::msg` (common, control, audio, stt, llm, tts, metrics) and
`conversation_msgs::rpc` (discovery, `AppServerInfo`). Type names, field names and order and enum order are the
pipeline's. **These types travel as plain CDR**: OMG CDR v1 little-endian *without* the 4-byte encapsulation
header, zenoh encoding `application/cdr`; every other package keeps the encapsulation. Since 0.10.2
([MIGRATION-0.10.2.md](MIGRATION-0.10.2.md)).
`tcnart_msgs::msg::EyeTrackingMessage` carries pupil, eye-expression and gaze-space fields and documented
`EYE_VALID_*` bits since 0.10.3 ([MIGRATION-0.10.3.md](MIGRATION-0.10.3.md)), the scene-image gaze point since
0.10.4; `tcnart_msgs::msg::ImuMessage` (accelerometer and gyroscope) is new in 0.10.4
([MIGRATION-0.10.4.md](MIGRATION-0.10.4.md)); `tcnart_msgs::msg::PersonTrackingMessage` (person
tracks) joined 0.10.4 as a later revision. 0.10.5 extends it for avatars (joint sets, per-joint age, `previous_id`,
orientations) and adds tcnart's `HumanPoseMessage` and the `TimeRefRequest`/`TimeRefReply` payloads
([MIGRATION-0.10.5.md](MIGRATION-0.10.5.md); joint sets:
[tcnart_msgs/msg/JointSets.md](src/dds/custom/tcnart_msgs/msg/JointSets.md)).

### The SQE wire contract

`src/dds/custom/tcnart_msgs/rpc/SIS.idl` holds the spatial query engine's (SQE, spatial_query_engine_rs) message
types, and `src/sqe/include/tcn/sqe/` the header-only, zenoh-free `tcn::sqe` contract library: keys, wire type
names, validation, relation requests, handles, leases. The canonical description of the protocol is the engine's
`docs/reference/protocol.md`; each change is a B-entry in its `docs/reference/compatibility-breaks.md`.

**Current: 0.10.0** (`tcn_schema/0.10.0@artekmed/stable`, engine B90). Every SQE payload's encoding annotation
carries the release, `;tcn_schema=0.10` (`tcn::sqe::kWireSchemaVersion`), and peers compare major.minor: a 0.10
engine refuses every 0.9-annotated message, pose streams included, and a 0.9 peer refuses every 0.10 reply
(`SchemaVersionMismatch`). **0.10.0 is a wire break: rebuild every peer together with the engine.**

What recent releases added:

- **0.10.7** (B94, additive): `SRG_SCOPE_DOMAIN`, domain nodes `@{domain}::{name}`, shared by exactly the
  fragments that name them in one domain; contract helpers `domain_node_id`, `DomainNodeId`, `FrameRef::domain`
- **0.10.6** (B93, additive): the relation sample, `{client}/sis/relation/sample`
  (`SISRelationSampleRequest` → `SISRelationSampleReply`): one value of a relation at one instant (`AT` a stamp,
  `NOW`, `LATEST`), with no stream started.
- **0.10.1** (B92, additive): the transaction log, `sqe/{engine_id}/op/txlog`.
- **0.10.0** (B90): a relation stream's evaluation mode (`SISEvaluationMode`: `SIS_EV_ENGINE`, the default;
  `SIS_EV_LOCAL`, the client composes the relation from the engine's plan with `sqe_local`; `SIS_EV_BOTH`), the
  plan in the start reply and on `{handle}/plan` (`tcn::sqe::plan_topic_for`), and `engine_epoch` on status
  notifications and plans.
- **0.9.3** (B89, additive): explain, `sqe/{engine_id}/op/explain`, and the plan types (`SISTranslationPlan`).

Migration notes, newest first: [MIGRATION-0.10.7.md](MIGRATION-0.10.7.md), [MIGRATION-0.10.6.md](MIGRATION-0.10.6.md), [MIGRATION-0.10.1.md](MIGRATION-0.10.1.md), [MIGRATION-0.10.0.md](MIGRATION-0.10.0.md) (with the steps for tcnart and the
holoscan apps), [MIGRATION-0.9.3.md](MIGRATION-0.9.3.md), and the older `MIGRATION-*.md` beside them. C++ clients
use sqe_client_cpp (0.4.0 for tcn_schema 0.10.0; 0.6.0 adds the relation sample, for 0.10.6), which builds on this library.
