# Migration to tcn_schema 0.10.7

**Additive. Nothing to change unless you declare one physical frame from several processes.** 0.10.7 adds one enum
value, `SRG_SCOPE_DOMAIN`, for the SQE's domain nodes (engine B94), and contract-library helpers for them. No type
gains or loses a field, no key changes, and the encoding annotation stays `;tcn_schema=0.10`, so every 0.10.x peer
talks to every other unchanged. 0.10.7 builds on 0.10.6 and includes its changes.

## What is new

`tcnart_msgs/msg/SpatialRelations.idl`:

| type | change |
|---|---|
| `SRNodeScope` | **appended** `SRG_SCOPE_DOMAIN` (ordinal 2) |

A **domain node** is shared by exactly the fragments that declare it in the same domain. It is meant for one physical
thing several processes declare, such as a rig camera's colour frame that the rig process and the IR-tracker process
both name, without claiming the name engine-wide as a `Global` node would.

- **A domain** is a key-prefix-shaped string with the client-id alphabet and without `::`, e.g. `tcn/ksv1/rig01`.
  Matching is exact: `tcn/ksv1/rig01` is not `tcn/ksv1`. A B94 engine refuses a domain outside its client scope.
- **The id is the name.** An `SRG_SCOPE_DOMAIN` node's `SRNode.name` is its whole id, `@{domain}::{name}`, e.g.
  `@tcn/ksv1/rig01::camera01_color`. Edges name it the same way (`SREdge.source` / `target`). A `SISFrameRef` names it
  with an **empty client** and that id as `node`. The leading `@` keeps it apart from a `Local` id
  `{fragment}::{name}`, even when the domain equals a client id. No client id can start with `@`.
- **Unchanged:** `Global` (engine-wide) and `Local` (one fragment).

The contract library adds:
- `tcn::sqe::check_domain`, `domain_node_id(domain, name)`, `parse_domain_node_id` and `kDomainNodeSigil`
  (`validate.h`). `kNodeIdSeparator` moved there from `node_name.h`, which includes it, so includes keep working.
- `DomainNodeId` (`node_name.h`).
- `FrameRef::domain(domain, name)` and `FrameRef::is_domain()` (`relation.h`).
- `marker_node_name(kind, id, marker_system)`: see below. The two-argument form is unchanged.

## What a B94 engine does differently (heads-up, not a schema change)

- **Unregistered markers are qualified by `marker_system`.** With a non-empty `StreamDescriptorMessage.marker_system`,
  marker 7 becomes `{prefix}{system}___sis_marker__7`, with `:` and any non-identifier byte in the system replaced by
  `.`. So `aruco:DICT_4X4_50` gives `_mtarucom_aruco.DICT_4X4_50___sis_marker__7`, and the same id in two dictionaries
  is two nodes. An empty `marker_system` keeps the old name. Registered markers still become the registered target's
  `Global` node. Anything that composes marker names itself should call
  `marker_node_name(kind, id, marker_system)`.
- **A kind conflict refuses the join.** When two fragments declare one shared (`Global` or domain) node with two
  different specific `node_type`s, e.g. `TRACKABLE` and `SENSOR`, the later join is refused `SIS_MU_REJECTED`.
  `GENERIC` gives way to either.

## To adopt

- **Declaring a shared device frame:** declare it with `SRG_SCOPE_DOMAIN` and `name = domain_node_id(domain, name)`
  in every process that names it. Use the same domain everywhere (e.g. the rig's own id prefix), the same convention,
  and the same specific `node_type`.
- **Readers of `SRGraph`** (graph queries, the web controller, Unity tooling): a reader built before 0.10.7 receives
  ordinal 2 for a domain node. Treat an unknown scope as **shared, not local**, and show the name as is. C++ and C#
  keep the integer. A decoder that rejects unknown enum values must be rebuilt against 0.10.7 before domain nodes
  are declared.
- **Engines:** an engine before B94 cannot decode `SRG_SCOPE_DOMAIN` and refuses such a join, which is loud and never
  silent. Declare domain nodes only against a B94 engine (`op/info` `protocol_break >= 94`).

## Planned: a 0.11 schema refactoring

0.10.7 carries the domain inside the name, so that it stays a micro release. The field-level design is deferred to a
deliberate 0.11 refactoring, together with other breaking changes. It would be:
- an `SRNode.domain` field, with the name bare again;
- a marker domain on template claims, for an engine that serves several rooms;
- and whatever else is queued for 0.11.

See the engine's `docs/superpowers/specs/2026-11-26-node-domains-design.md`.
