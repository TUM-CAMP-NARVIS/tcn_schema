# Migration to tcn_schema 0.6.0

Two additive RPC types, one RPC field pair, and a set of trailing string
fields across the spatial-relations messages.

**Every component that publishes or consumes `SRNode`, `SREdge`, `SRGraph` or
`SRCalibration` must be rebuilt** — those gained trailing fields, and CDR is
positional. The new RPC types are additive: a component that never sends a
pattern query is unaffected by B63.

---

## Why

Three things, all from the same place: a calibration tool is being built, and
the graph is about to be read and driven by people rather than only by the
engine.

---

## B63 — a pattern query

A new key, `{client_id}/sis/pattern/query`, answering *"how could this edge be
produced?"*

The model is Pustka, Huber, Bauer & Klinker, *Spatial Relationship Patterns*: a
pattern is a small graph over roles, some of whose edges are inputs that must
already exist and some outputs its algorithm produces. A caller names the edge
it wants — two frames and a transform type — and gets back the ways the graph
could produce it, each with its roles bound to real frames, its inputs bound to
concrete paths, and how it would have to be expanded.

```idl
struct SISPatternQueryRequest {
  SISFrameRef from; SISFrameRef to;
  tcnart_msgs::msg::SRTransformType wanted;
  uint32 radius;
  sequence<string> only_patterns;
  uint32 max_candidates;
};
```

**A new key rather than a fifth `SISGraphQueryKind`.** `SISGraphQueryRequest`'s
own comment argues for one key on the grounds that four keys mean four
rate-limit buckets. That argument is respected rather than overturned: this key
**shares the graph read's bucket**, because it is the same cost against the
same database. What it does not share is the reply shape — a pattern instance
is bound roles, bound path segments, an expansion kind and a measurement count,
none of which belongs in a reply otherwise made of nodes and edges.

**Matching is anchored on the output.** Naming the wanted edge binds two of the
pattern's roles before the search starts, which is what makes an otherwise
NP-complete subgraph search tractable enough to run inside the engine.

**`SISPatternSegment` is self-contained** — direction, stream topic, stored
transform, and whether the hop is computed — so a caller can build a pipeline
from one round trip. A calibration composes these itself rather than asking for
a relation stream per input, because a correspondence relates *k* measurements
at **one** instant and two streams have two independent triggers.

**Empty `instances` is an `SIS_GQ_OK` answer.** "Nothing in this graph produces
that edge" is what an under-instrumented setup looks like, and it tells an
operator to add a landmark rather than to retry.

## B64 — `SIS_GQ_PATH` stops being reserved

The variant has shipped since the graph-query service existed, declared and
refused, on the grounds that a path read must answer *which* predicate it
walked and a plain traversal answers none of them.

**The objection is resolved rather than dropped: the request now names the
predicate.**

```idl
enum SISPathPredicate {
  SIS_PP_WALKABLE,         // refuses uncalibrated relations
  SIS_PP_CONNECTABLE,      // the relaxed shape rule a pending stream probes with
  SIS_PP_WALKABLE_STATIC   // walkable, and refusing anything that moves
};
```

`SISGraphQueryRequest` gains trailing `predicate` and `radius`;
`SISGraphQueryReply` gains trailing `path` and `radius_exhausted`. The second
objection — that an honest path read means an unbounded walk — is answered by
`radius`, and `radius_exhausted` distinguishes *"not connected"* from *"not
within the radius you asked for"*. Those are different answers, and a caller
that cannot tell them apart widens forever or gives up too early.

## B65 — a graph a person can read

`name` is an identifier and is constrained to be one. `SRNode` additionally had
`custom_data`, described as opaque and application-defined; `SREdge`,
`SRGraph` and `SRCalibration` had nothing at all.

| struct | new trailing fields |
| --- | --- |
| `SRNode` | `display_name`, `description` |
| `SREdge` | `display_name`, `description` |
| `SRGraph` | `display_name`, `description` |
| `SRCalibration` | `description` |

All optional, all empty by default, and nothing behaves differently when they
are. A reader falls back to `name`.

`SRCalibration` gets only a `description`, deliberately: it already has
`method`, which names an algorithm and is matched on. The description is free
text about *this run* — which landmarks, whose survey, what the operator
noticed — and is never matched on.

`SRNodeView` and `SREdgeView` embed `SRNode` and `SREdge` wholesale, so a read
carries the labels a declaration set without either view changing.

---

## To adopt

1. **Rebuild anything touching `SRNode`, `SREdge`, `SRGraph` or
   `SRCalibration`.** These are appended fields on positional CDR, so an
   un-rebuilt publisher and a rebuilt consumer disagree about where the *next*
   element of a sequence starts — and `SRNode`/`SREdge` are both nested in
   `SRGraph` as sequences, which is where that goes wrong fastest.
2. **Nothing is required to be set.** Empty display names and descriptions are
   the ordinary case; a component with nothing to say behaves exactly as it
   did.
3. **`SIS_GQ_PATH` callers must set `predicate`.** The zero value is
   `SIS_PP_WALKABLE`, which is the right default — it is what a running stream
   composes by — but choosing it deliberately is the point of the field.
4. **B63 needs no adoption.** A component that never sends a pattern query is
   unaffected.
