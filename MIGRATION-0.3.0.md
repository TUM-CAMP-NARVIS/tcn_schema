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

Field order in 0.3.0: `name, node_type, scope, coord, active, uuid,
stream_descriptor_topic, custom_data`.

### `SREdgeTemporalType` — renamed enumerators

| 0.2.0 | 0.3.0 |
| --- | --- |
| `SRG_EDGE_TEMPORAL_NONE` | *removed* |
| `SRG_EDGE_TEMPORAL_STATIC` | `SRG_EDGE_STATIC` |
| `SRG_EDGE_TEMPORAL_STATIC_CALIBRATED` | `SRG_EDGE_STATIC_CALIBRATED` |
| `SRG_EDGE_TEMPORAL_DYNAMIC` | `SRG_EDGE_DYNAMIC` |
| `SRG_EDGE_TEMPORAL_DYNAMIC_INFERRED` | `SRG_EDGE_DYNAMIC_INFERRED` |

`_NONE` removal shifts every ordinal down by one. **Wire change.**

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
| — | `string stream_descriptor_topic` | **new**, from `SISRelationMessage.tracker_stream_descriptor_topic` |
| — | `sequence<SRCalibration> calibration` | **new**, 0 or 1 entries |

Field order in 0.3.0: `name, source, target, temporal_type, transform, weight,
stream_descriptor_topic, calibration`.

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

### `SISTopicTranslationStartRequest`

0.2.0:

```idl
string src_topic; string dst_topic;
string src_client;   // empty src_client AND src_node meant "world coordinates"
string dst_client;
SISComponentRef src_node;
SISComponentRef dst_node;
```

0.3.0:

```idl
string src_topic; string dst_topic;
SISEndpoint src;
SISEndpoint dst;
```

where

```idl
struct SISFrameRef { string client; string node; };
enum SISEndpointKind { SIS_ENDPOINT_FRAME, SIS_ENDPOINT_RAW };
union SISEndpoint switch(SISEndpointKind) {
case SIS_ENDPOINT_FRAME: SISFrameRef frame;
case SIS_ENDPOINT_RAW:   boolean unused;
};
```

**Porting rule:** where 0.2.0 left a client and node empty, send
`SIS_ENDPOINT_RAW`. Where it set both, send `SIS_ENDPOINT_FRAME` with the same
two strings. Setting one and not the other was already invalid and is now
unrepresentable.

### `SISTopicTranslationStatusReply` → `SISTopicTranslationStatus`

| 0.2.0 | 0.3.0 |
| --- | --- |
| `SIS_TTSTATUS_ACTIVE` | `SIS_TT_ACTIVE` |
| `SIS_TTSTATUS_INACTIVE` | `SIS_TT_INACTIVE` |
| `SIS_TTSTATUS_UNAVAILABLE` | splits into `SIS_TT_NO_PATH`, `SIS_TT_UNKNOWN_FRAME`, `SIS_TT_CLOCK_DOMAIN_MISMATCH`, `SIS_TT_REJECTED` |

**Porting rule:** a client that retried on `UNAVAILABLE` must now distinguish.
`SIS_TT_NO_PATH` and `SIS_TT_UNKNOWN_FRAME` are permanent for the current graph
— retrying without changing the graph will never succeed. Only transient
conditions warrant a retry.

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
6. Convert translation-start endpoints to `SISEndpoint`.
7. Handle the four replacement status values; stop retrying on permanent ones.
8. Drop `target_count` from target-tracking publishers.
9. Optionally declare `sensor_latency_ns` and `clock_domain`.
10. Send `SISLeaveRequest` on shutdown instead of relying on the timeout.

## Wire-level ordinal shifts

Two enums lost their leading `_NONE` value, so every remaining enumerator
shifts down by one. A peer built against 0.2.0 that somehow decodes 0.3.0 bytes
would misread these silently:

- `SRNodeType` — `SRG_NODE_NONE` removed
- `SREdgeTemporalType` — `SRG_EDGE_TEMPORAL_NONE` removed

Every other change is either a source-level rename or a structural change that
fails to decode outright.
