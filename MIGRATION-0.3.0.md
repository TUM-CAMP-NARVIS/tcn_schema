# Migration to tcn_schema 0.3.0

A breaking change to the spatial-relations and SIS message families. Every
component that speaks SIS, publishes target tracking, or publishes a stream
descriptor must be rebuilt and, in most cases, edited.

This document lists every change, field by field, so a port can be done in one
pass. **Nothing outside the four files listed here changed.**
`RigidTransformMap`, `RigidTransformMapItem`, `Pose6DMapMessage`,
`Pose6DMessage`, `Target`, `Marker`, and every ROS 2 message are untouched.

---

## Why

Two IDL files described the same graph. `msg/SpatialRelations.idl` had
`SRNode` / `SREdge` / `SRGraph` — the spatial relationship graph as the
literature defines it — while `rpc/SIS.idl` had `SISComponentMessage` /
`SISRelationMessage` / `SISJoinMessage`, which was the same graph again, less
abstractly. Consumers implemented the second and ignored the first.

0.3.0 keeps the abstract model and reduces SIS to RPC envelopes over it.

---

## 1. `msg/SpatialRelations.idl`

### `CoordinateAxis` — enumerators renamed

| 0.2.0 | 0.3.0 |
| --- | --- |
| `X` | `COORD_AXIS_X_POS` |
| `XNegative` | `COORD_AXIS_X_NEG` |
| `Y` | `COORD_AXIS_Y_POS` |
| `YNegative` | `COORD_AXIS_Y_NEG` |
| `Z` | `COORD_AXIS_Z_POS` |
| `ZNegative` | `COORD_AXIS_Z_NEG` |

Ordinal values are unchanged, so the wire encoding is identical — this is a
source-level rename only. The unprefixed names were a name clash waiting to
recur in a shared module.

### `CoordinateSystem`

| Field | 0.2.0 | 0.3.0 |
| --- | --- | --- |
| `dimensions` | `int8` | `uint8` |

Wire-compatible (both one byte); the value was never meaningfully negative.

### `SRNodeType` — one enumerator removed

`SRG_NODE_NONE` is gone. **Every following enumerator shifts down by one**, so
this *is* a wire change. `SRG_NODE_GENERIC` is now 0. A node always has a type;
`GENERIC` is the unremarkable case.

### `SRNodeScope` — new

```idl
enum SRNodeScope { SRG_SCOPE_LOCAL, SRG_SCOPE_GLOBAL };
```

Moved from `rpc::SISScope`, whose values map one to one
(`SIS_SCOPE_LOCAL` → `SRG_SCOPE_LOCAL`).

### `SRNode` — restructured

| 0.2.0 | 0.3.0 | Note |
| --- | --- | --- |
| `SRNodeType node_type` | `SRNodeType node_type` | moved after `name` |
| `UUID node_uuid` | `UUID uuid` | renamed |
| `uint32 node_id` | *removed* | edges now reference nodes by name |
| `string node_name` | `string name` | renamed; now the identity |
| `string custom_data` | `string custom_data` | unchanged |
| — | `SRNodeScope scope` | **new**, from `SISComponentMessage.scope` |
| — | `CoordinateSystem coord` | **new**, from `SISComponentMessage.coord` |
| — | `boolean active` | **new**, from `SISComponentMessage.active` |
| — | `string stream_descriptor_topic` | **new**, from `SISComponentMessage.tracker_stream_descriptor_topic` |
| — | `sequence<SRNodeTemplate> node_template` | **new**, see Templates below |
| — | `string materialized_from` | **new** |
| — | `uint32 materialized_id` | **new** |

Field order in 0.3.0: `name, node_type, scope, coord, active, uuid,
stream_descriptor_topic, node_template, materialized_from, materialized_id,
custom_data`.

### Templates — replacing `is_template` / `template_type` / `template_match_id`

Those three properties existed only in the database schema, never in the IDL,
and were written as hardcoded constants. They are replaced by a modelled
concept.

```idl
struct SRNodeTemplate {
    MarkerType marker_type;             // what kind of observation matches
    sequence<uint32> match_ids;         // empty = any id an observer reports
    string instance_name_pattern;       // "aruco_{id}" -> "aruco_7"
};
```

A node carrying a non-empty `node_template` is a **blueprint**, not a frame of
reference. It is never traversed. When an observation matches its rule, the
template node **and every edge attached to it** are materialized into concrete
copies: the new node takes its name from `instance_name_pattern`, and both the
node and the edges record `materialized_from` (the template's name) and, on the
node, `materialized_id` (the id that matched).

This models what a client genuinely cannot know at announcement time — which
ArUco ids are in the room, which instruments will be brought in — instead of
leaving it to be discovered by side effect.

**Porting:** a client that previously relied on the engine fabricating a node
the first time a tracker reported an unknown marker id should now declare a
template node with an edge from the observing camera. The old implicit rule is
reproduced by `match_ids` empty, `instance_name_pattern` set to the marker
prefix plus `{id}`, and a single `SRG_EDGE_DYNAMIC` edge from the observer.

### `SREdgeTemporalType` — renamed enumerators

| 0.2.0 | 0.3.0 |
| --- | --- |
| `SRG_EDGE_TEMPORAL_NONE` | *removed* |
| `SRG_EDGE_TEMPORAL_STATIC` | `SRG_EDGE_STATIC` |
| — | `SRG_EDGE_STATIC_UNCALIBRATED` (**new**) |
| `SRG_EDGE_TEMPORAL_STATIC_CALIBRATED` | `SRG_EDGE_STATIC_CALIBRATED` |
| `SRG_EDGE_TEMPORAL_DYNAMIC` | `SRG_EDGE_DYNAMIC` |
| `SRG_EDGE_TEMPORAL_DYNAMIC_INFERRED` | `SRG_EDGE_DYNAMIC_INFERRED` |

`_NONE` removal shifts every ordinal down by one, and the new
`SRG_EDGE_STATIC_UNCALIBRATED` sits between `SRG_EDGE_STATIC` and
`SRG_EDGE_STATIC_CALIBRATED`, shifting the two dynamic values up again.
**Wire change.** Final order: `STATIC`, `STATIC_UNCALIBRATED`,
`STATIC_CALIBRATED`, `DYNAMIC`, `DYNAMIC_INFERRED`.

`SRG_EDGE_STATIC_UNCALIBRATED` is the second kind of unknown: both endpoints
are known, but their relation has not been measured. Such an edge is **not
traversable** — a path may not compose a value that does not exist yet. It is
the work queue an external calibration tool reads, and the tool promotes it to
`SRG_EDGE_STATIC_CALIBRATED` with an `SRCalibration` entry once solved.

### `SRTransform` — new union

```idl
union SRTransform switch(SRTransformType) {
case SRG_TRANSFORM_POSE6D:        RigidTransform pose6d;
case SRG_TRANSFORM_TRANSLATION3D: Vector3 translation3d;
};
```

Moved from `rpc::SISTransformation`, unchanged in shape. Discriminator values
map one to one (`SIS_TRANSFORM_POSE6D` → `SRG_TRANSFORM_POSE6D`).

### `SRCalibration` — new

```idl
struct SRCalibration {
    string method;
    double residual;                          // RMS error in metres, negative if unreported
    builtin_interfaces::msg::Time computed_at;
};
```

Optional provenance for `SRG_EDGE_STATIC_CALIBRATED` edges. Producers that do
not calibrate leave the sequence empty.

### `SREdge` — restructured

| 0.2.0 | 0.3.0 | Note |
| --- | --- | --- |
| `string edge_name` | `string name` | renamed |
| `uint32 node_source_id` | `string source` | **now a node name** |
| `uint32 node_destination_id` | `string target` | **now a node name** |
| `SREdgeTemporalType temporal_type` | same | unchanged |
| `string data_type` | *removed* | had no defined meaning |
| `string uri` | *removed* | had no defined meaning |
| — | `SRTransform transform` | **new**, from `SISRelationMessage.transform` |
| — | `double weight` | **new**, from `SISRelationMessage.weight` |
| — | `boolean is_invertible` | **new**, see below |
| — | `string stream_descriptor_topic` | **new**, from `SISRelationMessage.tracker_stream_descriptor_topic` |
| — | `sequence<SRCalibration> calibration` | **new**, 0 or 1 entries |
| — | `string materialized_from` | **new**, set on edges created by a template |
| — | `sequence<string> derived_from` | **new**, set on edges published by a relation stream |

Field order in 0.3.0: `name, source, target, temporal_type, transform, weight,
is_invertible, stream_descriptor_topic, calibration, materialized_from,
derived_from`.

> **Derived edges.** A running relation stream publishes its own result back
> into the graph as a `SRG_EDGE_DYNAMIC` edge from observer to target, with
> `stream_descriptor_topic` set to its output topic and a weight below the sum
> of the edges it walked. Pathfinding therefore prefers it automatically, and a
> relation computed once becomes a subpath anything else can reuse.
>
> `derived_from` names the edges that stream's path walked. It exists because
> the weight rule steers into cycles: a stream asked to compute a relation it
> already publishes would otherwise route through its own output and sample the
> buffer it is filling. Compiling a stream excludes every derived edge that
> transitively depends on the edge being compiled.
>
> A producer that only declares its own graph never sets this. A consumer
> reading the graph should treat a non-empty `derived_from` as "computed by the
> engine, not measured".

> **`is_invertible` and reversibility.** A relation is declared in one
> direction, `source -> target`, and a path resolver walks it the other way by
> composing its inverse. That is what makes a single forward declaration enough
> to answer a query in either direction, and it is why the graph needs only one
> edge per relation rather than two.
>
> Not every relation has an inverse. One that discards information — a
> projection into an image plane, for instance — cannot be reversed, and
> composing something called its inverse would invent the dimension it lost.
> Set `is_invertible` to `false` for those; pathfinding will route around them
> rather than through them backwards.
>
> **Porting:** rigid transforms are invertible, so `true` reproduces 0.2.0
> behaviour, where every edge was walked in both directions unconditionally.

> **`name` now carries meaning.** Parallel edges between the same pair of nodes
> are legal and are distinguished by name. Edge removal takes a name and removes
> exactly one edge — 0.2.0 had no edge identity, so removal took a pair of
> endpoints and removed all of them.

### `SRGraph`

| 0.2.0 | 0.3.0 |
| --- | --- |
| `string graph_name` | `string name` |
| — | `string origin` (**new**, from `SISJoinMessage.origin.name`) |
| `sequence<SRNode> nodes` | unchanged |
| `sequence<SREdge> edges` | unchanged |

---

## 2. `rpc/SIS.idl`

### Removed outright

`SISScope`, `SISTransformationType`, `SISTransformation`, `SISComponentRef`,
`SISRelationRef`, `SISComponentMessage`, `SISRelationMessage`,
`SISJoinMessage`, `SISTopicTranslationStatusReply`.

Their content moved into `msg/SpatialRelations.idl` as described above.

### Request types

| 0.2.0 | 0.3.0 | Change |
| --- | --- | --- |
| `SISJoinMessage { origin, components, relations }` | `SISJoinRequest { SRGraph graph }` | origin moves into the graph |
| `SISComponentMessage` | `SISNodeUpdateRequest { SRNode node }` | |
| `SISComponentRef` (as remove) | `SISNodeRemoveRequest { string name }` | one less level of nesting |
| `SISRelationMessage` | `SISEdgeUpdateRequest { SREdge edge }` | |
| `SISRelationRef` (as remove) | `SISEdgeRemoveRequest { string name }` | **by edge name, not endpoint pair** |
| — | `SISLeaveRequest { string client }` | **new** |

### Topic translation becomes relation streams

`SISTopicTranslationStartRequest`, `SISTopicTranslationStopRequest` and
`SISTopicTranslationStatusReply` are **removed**. Nothing replaces them
field-for-field; the concept changed.

0.2.0 asked the engine to take a pose stream and republish it re-expressed:

```idl
string src_topic; string dst_topic;
string src_client;   // empty src_client AND src_node meant "world coordinates"
string dst_client;
SISComponentRef src_node;
SISComponentRef dst_node;
```

0.3.0 asks it for a spatial relation instead:

```idl
struct SISFrameRef { string client; string node; };

enum SRStreamTriggerKind {
    SRG_TRIGGER_FIXED_RATE, SRG_TRIGGER_ON_STREAM, SRG_TRIGGER_ON_ANY
};

union SRStreamTrigger switch(SRStreamTriggerKind) {
case SRG_TRIGGER_FIXED_RATE: double hz;
case SRG_TRIGGER_ON_STREAM:  string topic;
case SRG_TRIGGER_ON_ANY:     boolean unused;
};

struct SISRelationStreamStartRequest {
    SISFrameRef observer;
    SISFrameRef target;
    string output_topic;
    SRStreamTrigger trigger;
    double freshness_bias;      // [0,1]; 0 reuses shortcuts, 1 walks only measured edges
};

struct SISRelationStreamStopRequest { string output_topic; };
```

**Why the reshape.** A source topic was doing two unrelated jobs: naming the
payload to copy, and implicitly deciding when the translation fired. Splitting
them is what makes a fixed output rate expressible. It also makes a purely
static relation streamable — a calibrated edge maintained at runtime can be
published with no tracker anywhere on the path, which topic translation could
not express.

**Porting rule.** A 0.2.0 translation

```
src_topic = T, src_client = A, src_node = N,
            dst_client = B, dst_node = M, dst_topic = U
```

becomes

```
observer = { client: B, node: M },
target   = { client: A, node: N },
output_topic = U,
trigger = SRG_TRIGGER_ON_STREAM { topic: T }
```

Note the **inversion**: translation named the frame poses came *from* first,
whereas a relation names the frame poses are expressed *in* first. `observer` is
the old destination; `target` is the old source.

The old "both client and node empty means world coordinates" case has no
equivalent and needs none — there is no world frame any more, and a relation
between two named frames is what it was approximating.

### Repeated requests

Streams are keyed by `output_topic`, and the same application launched twice will
ask for the same relation twice. That is expected, not an error:

| State | Result |
| --- | --- |
| no stream on that topic | created, `SIS_RS_ACTIVE` |
| identical observer, target, trigger and bias | no-op, `SIS_RS_ACTIVE` |
| different properties | `SIS_RS_REJECTED` |

A client must not assume its properties won. Reconfiguring in place would change
the meaning of a stream a third party is already consuming, so the second request
is refused rather than silently applied.

Stopping a stream removes its derived edge, and any other stream whose path
walked it is recompiled onto the longer route. A consumer may therefore see a
stream's latency change when an unrelated stream stops.

### Freshness bias

A relation already computed by another stream is available as a derived edge
weighted below the path it replaces, so it is normally preferred. It is also
strictly staler by one publish-ingest-buffer round. `freshness_bias` in `[0,1]`
lets the consumer choose: `0` takes every shortcut, `1` ignores derived edges and
walks only measured relations, and values between move a derived edge's effective
weight toward the path it replaced. Applied at compile time.

### Trigger modes

| Kind | Fires on | Every edge sampled at |
| --- | --- | --- |
| `SRG_TRIGGER_FIXED_RATE` | a timer at `hz` | now — every edge extrapolated to the present |
| `SRG_TRIGGER_ON_STREAM` | a sample on `topic` | that sample's stamp, latency-corrected |
| `SRG_TRIGGER_ON_ANY` | any input on the path | the arriving sample's stamp |

There is no default; the discriminator is always present. The three differ in
more than rate, and picking one for a client would be picking whether its poses
are interpolated or extrapolated.

### Template targets

If `target` names a node carrying a `node_template`, the stream publishes one
entry per instance materialized from it, all observed at the same instant. This
is how a multi-marker tracker stream survives the move away from topic copying:
"every ArUco marker this camera sees, in the headset's frame" is one request and
one message, not one per marker.

### Status

`SISTopicTranslationStatusReply` becomes `SISRelationStreamStatus`:

| 0.2.0 | 0.3.0 |
| --- | --- |
| `SIS_TTSTATUS_ACTIVE` | `SIS_RS_ACTIVE` |
| `SIS_TTSTATUS_INACTIVE` | `SIS_RS_INACTIVE` |
| `SIS_TTSTATUS_UNAVAILABLE` | splits into `SIS_RS_NO_PATH`, `SIS_RS_UNKNOWN_FRAME`, `SIS_RS_CLOCK_DOMAIN_MISMATCH`, `SIS_RS_REJECTED` |

**Porting rule:** a client that retried on `UNAVAILABLE` must now distinguish.
`SIS_RS_NO_PATH` and `SIS_RS_UNKNOWN_FRAME` are permanent for the current graph
— retrying without changing the graph will never succeed.

---

## 3. `msg/PoseTracking.idl`

Only `TargetTrackingMessage` changes.

| 0.2.0 | 0.3.0 |
| --- | --- |
| `header` | unchanged |
| `target_origin` | unchanged |
| `uint32 target_count` | **removed** — duplicated `targets.length()` |
| `sequence<Target> targets` | unchanged |
| `uint32 system_status` | `uint32 status_flags` |

Named constants replace the comment:

```idl
const uint32 TRACKER_STATUS_RUNNING = 0x01;
const uint32 TRACKER_STATUS_VALID   = 0x02;
const uint32 TRACKER_STATUS_ERROR   = 0x04;
```

Bit meanings and positions are unchanged.

`Pose6DMessage` and `Pose6DMapMessage` are **not** changed. `Target` and
`Marker` in `Types.idl` are **not** changed.

---

## 4. `msg/StreamDescriptor.idl`

Two fields appended to `StreamDescriptorMessage`:

```idl
      // nanoseconds between the measurement and the header stamp; 0 if unknown
      int64 sensor_latency_ns;

      // empty means the default synchronized domain
      string clock_domain;
```

Both may be left at their defaults (`0` and `""`), which reproduces 0.2.0
behaviour exactly. A publisher that knows its capture-to-stamp delay should
declare it: a consumer interpolating across sensors subtracts it before use, and
two sensors with different delays otherwise correlate at the wrong instants —
an error that presents as a small fixed offset rather than as a fault.

---

## Porting checklist

1. Rename `CoordinateAxis` enumerators (source only; wire unchanged).
2. Replace `SISComponentMessage` / `SISRelationMessage` construction with
   `SRNode` / `SREdge`, moving `scope`, `coord`, `active`, `weight`,
   `transform` and the tracker topic into them.
3. Give every edge a **name**. This is the one field with no 0.2.0 source; pick
   a scheme that is unique within the graph. `"{source}_{target}"` reproduces
   the old implicit identity if you have no better name.
4. Reference nodes by **name** rather than `uint32` id.
5. Set `SRGraph.origin` from what was `SISJoinMessage.origin.name`.
6. Convert every topic translation into a relation stream, remembering that
   `observer` is the old *destination* and `target` the old *source*, and
   choosing a trigger explicitly.
7. Handle the four replacement status values; stop retrying on permanent ones.
8. Drop `target_count` from target-tracking publishers.
9. Optionally declare `sensor_latency_ns` and `clock_domain`.
10. Send `SISLeaveRequest` on shutdown instead of relying on the timeout.
11. Declare a **template node** for anything whose identity is unknown at
    announcement time, rather than relying on the engine to fabricate nodes when
    an unknown marker id appears.
12. Mark relations awaiting calibration as `SRG_EDGE_STATIC_UNCALIBRATED`
    rather than declaring an identity transform and hoping.

## Wire-level ordinal shifts

Two enums lost their leading `_NONE` value, so every remaining enumerator
shifts down by one. A peer built against 0.2.0 that somehow decodes 0.3.0 bytes
would misread these silently:

- `SRNodeType` — `SRG_NODE_NONE` removed
- `SREdgeTemporalType` — `SRG_EDGE_TEMPORAL_NONE` removed and
  `SRG_EDGE_STATIC_UNCALIBRATED` inserted

Every other change is either a source-level rename or a structural change that
fails to decode outright.

---

## 5. Graph fragments, addendum: `SRNode.pinned`, `SIS_RS_PENDING`, status notifications

The engine is moving from one graph per client to a single merged graph
assembled from fragments a client announces and can later retract. This
addendum adds the three fields that later work in that migration depends on.
It is layered onto 0.3.0 rather than given its own version number; treat it as
part of the same port.

### `SRNode.pinned` — **new, wire break for CDR publishers of `SRNode`**

```idl
struct SRNode {
    ...
    string stream_descriptor_topic;

    // Survives fragment retraction. Set by whatever created the node
    // outside a fragment's lifecycle -- a surveyed fiducial, a
    // calibrated rig. The engine never sets this; it only honours it,
    // so no tool needs the engine's permission to keep its own data.
    boolean pinned;

    sequence<SRNodeTemplate> node_template;
    ...
};
```

Inserted immediately after `stream_descriptor_topic`, before `node_template`.
Field order in 0.3.0+: `name, node_type, scope, coord, active, uuid,
stream_descriptor_topic, pinned, node_template, materialized_from,
materialized_id, custom_data`.

**This is a wire break.** `SRNode` is a fixed-size-field struct nested as a
`sequence<SRNode>` inside `SRGraph`, and CDR has no field names or
self-describing lengths — every field is positional. A publisher built
against pre-addendum 0.3.0 (no `pinned`) and a reader built against this
addendum disagree about where one `SRNode` ends and the next begins. On a
graph with exactly one node this happens to still decode, because the reader
simply runs out of input at the same point the writer stopped; on a graph
with two or more nodes, every node after the first is read starting from the
wrong offset — a garbage graph with no decode error. **Every CDR publisher of
`SRNode` (directly, or via `SRGraph`, `SISJoinRequest`, or
`SISNodeUpdateRequest`) must be rebuilt against this addendum before any
reader is.** A node created by anything other than the engine itself
(surveyed fiducials, calibration rigs) should set `pinned = true`; the engine
never sets it and only honours it.

### `SISRelationStreamStatus::SIS_RS_PENDING` — appended, not a wire break

```idl
enum SISRelationStreamStatus {
    SIS_RS_ACTIVE,
    SIS_RS_INACTIVE,
    SIS_RS_NO_PATH,
    SIS_RS_UNKNOWN_FRAME,
    SIS_RS_CLOCK_DOMAIN_MISMATCH,
    SIS_RS_REJECTED,
    SIS_RS_PENDING           // new; appended last so no existing ordinal moves
};
```

`SIS_RS_PENDING` covers a relation stream that is accepted but not yet
producing — either because its path runs through a
frame that fragment retraction withdrew and which is not `pinned` — distinct
from `SIS_RS_NO_PATH` (no path ever existed) because a parked stream may
resume with no reconfiguration if the fragment reappears. Appended at the end
so every existing ordinal (`SIS_RS_ACTIVE` through `SIS_RS_REJECTED`) is
unchanged; a peer that has not been rebuilt simply never sees this value.

### `SISRelationStreamStatusNotification` — new

```idl
// Published on "{output_topic}/status" when a stream's status changes.
//
// A consumer otherwise cannot tell "the engine says there is no path" from
// "the engine died", and only the first is recoverable by waiting.
struct SISRelationStreamStatusNotification {
    string output_topic;
    SISRelationStreamStatus status;
    string detail;                  // human-readable reason; may be empty
};
```

A new type, so introducing it breaks nothing that does not already publish or
subscribe to it. The engine publishes one on `"{output_topic}/status"`
whenever a relation stream's `SISRelationStreamStatus` changes — including
into and out of `SIS_RS_PENDING`. A consumer that previously inferred stream
health only from the presence or absence of output on `output_topic` should
subscribe to the status topic instead: silence on the data topic is
ambiguous between "the engine has nothing new to say" and "the engine is no
longer running the stream", and only the notification disambiguates it.

### Porting checklist (addendum)

13. Rebuild every `SRNode` / `SRGraph` CDR publisher before any reader that
    expects `pinned` — the field ordering makes this a hard dependency, not a
    courtesy.
14. Set `pinned = true` on any node whose lifetime a tool manages itself,
    independent of the fragment that first announced it.
15. Handle `SIS_RS_PENDING` alongside the existing statuses; unlike
    `SIS_RS_NO_PATH` and `SIS_RS_UNKNOWN_FRAME`, it is not necessarily
    permanent for the current graph.
16. Subscribe to `"{output_topic}/status"` for `SISRelationStreamStatusNotification`
    rather than inferring stream health from silence on `output_topic`.
