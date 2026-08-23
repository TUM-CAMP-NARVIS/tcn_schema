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

Field order in 0.3.0: `name, source, target, temporal_type, transform, weight,
is_invertible, stream_descriptor_topic, calibration, materialized_from`.

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
