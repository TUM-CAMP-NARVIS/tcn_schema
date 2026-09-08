# Migration to tcn_schema 0.7.0

Four trailing fields, spread across three messages. All additive; nothing
changes position.

**Every component that publishes or consumes `SREdge`, `SRGraph`, or a pattern
query must be rebuilt.** CDR is positional, so an un-rebuilt peer reads a short
message — it does not misread one, which is what the previous draft of B67
would have caused and is why that shape was abandoned.

---

## Why

All four come from running the calibration tool against a real engine for the
first time. Each is a case where the schema described the graph correctly and
left a consumer unable to act on it.

---

## B66 — a relation can be pinned

```idl
struct SREdge {
  ...
  boolean pinned;                 // appended
};
```

`SRNode` has carried `pinned` since 0.3.0: *survives fragment retraction, set
by whatever created the node outside a fragment's lifecycle*. A relation could
not, and the consequence is worse than an asymmetry.

**A relation is currently destroyed at the moment nobody wants it destroyed.** A
fragment's retraction deletes every relation whose two endpoints that fragment
links, once no other fragment still declares it. So a tool that writes a
calibrated edge and then exits takes the calibration with it.

Pinning the *endpoints* does not help. It guarantees the deletion: linking both
nodes is exactly what brings the relation into the "wholly within this
fragment's nodes" test that removes it. There is no arrangement of the previous
primitives that produces a durable relation written by a short-lived tool.

### The case this exists for

**External measurement.** Someone surveys a few visible points on a device or in
a room — a total station, a CAD model, a manufacturer's drawing — and pushes
those positions into the graph so that anything can afterwards be calibrated
*into that coordinate space*.

A landmark is two things, and before this field only one of them could be made
durable:

| | what it is | survived its author? |
| --- | --- | --- |
| the landmark's frame | an `SRNode` | yes — `pinned` |
| **where it is** | a static `Translation3D` relation | **no** |

A survey pushed by a tool that then exited left behind pinned nodes with
nothing saying where any of them were: a graph that looks like it holds
landmarks and has lost every position. Strictly worse than an empty one.

The same gap costs a calibrated edge its durability, which is how it was found.

### What an engine must also do

**A `pinned` relation needs a way to be removed deliberately, and the previous
removal path cannot reach one.** A relation between two `Global` nodes matches
nothing in a name-scoped, `Local`-anchored removal and is a visible no-op. An
engine that honours this flag without providing such a path creates relations
nothing can ever delete, which is a worse failure than the one being fixed.

The material for it exists: relation declarers are already reference-counted,
which is what a retraction reads to decide whether it was the last one. The
rule that follows is *a declarer may withdraw its own declaration, and the row
goes when the last one does* — the same rule retraction already applies,
reached deliberately instead of by departing.

`false` is the value every peer written before this field sends, and it is
exactly today's behaviour.

---

## B67 — a space expansion carries its duplicates' paths

```idl
struct SISPatternExpansionInput {          // new
  uint32 instance;                         // which duplicate
  uint32 input;                            // which of the pattern's inputs
  SISPatternInput binding;                 // that duplicate's own path
};

struct SISPatternInstance {
  ...
  sequence<SISPatternRole> space_instances;              // unchanged
  ...
  sequence<SISPatternExpansionInput> space_inputs;       // appended
};
```

**A space expansion is its duplicates.** Absolute orientation over three
surveyed landmarks is three correspondences at one instant — one per landmark —
and a consumer needs a pipeline per landmark per input to evaluate them.

0.6.0 carried the duplicated **roles** and nothing else. The matcher had already
resolved each duplicate's paths and then discarded them, so a consumer could
compile only the base binding: a session over three landmarks would have been a
calibration from one of them. Determined, plausible, and wrong.

A tool that noticed this refused the candidate outright rather than estimating
from a third of the data. That is the correct behaviour and it is not what
anyone wants — it made the surveyed-landmark workflow, which is the whole point
of pinned landmarks, unreachable.

### Why this is flat rather than nested

The obvious shape is a sequence of duplicates each holding its own sequence of
inputs. It was written that way first and **does not compile**: the reply would
nest four levels of unbounded sequence (`instances` → `duplicates` → `inputs` →
`segments`), and the generator computes a worst-case CDR size that overflows
the `uint32` it stores it in.

So each duplicate's inputs are carried flat and tagged by index. That has an
independent merit: `space_instances` keeps its meaning and its type, which
makes this an **append** rather than a change in the middle of the struct.

Indexing is unambiguous because exactly one role of a pattern is expandable — a
matcher meeting two would have to decide how to pair their duplicates, and no
catalogue entry asks for that. Each entry of `space_instances` is therefore one
duplicate.

### What a consumer must do

`space_instances` non-empty with `space_inputs` empty means a **pre-0.7.0
engine**. Refuse the candidate; do not estimate from the base binding alone.
That is exactly the failure this release exists to remove, and it is silent.

---

## B68 — a segment carries what a subscriber cannot obtain

```idl
struct SISPatternSegment {
  ...
  int64  sensor_latency_ns;       // appended
  double frame_rate;              // appended
};
```

Both come from the stream's descriptor, and **a descriptor is published once.**
A consumer that joins later never sees one: there is nothing to subscribe to
and nothing to query. The engine read it at adoption and is the only party
still holding it, so a reply that omitted these left a caller with no way to
obtain them at all.

**`sensor_latency_ns`** — nanoseconds between the measurement and the header
stamp. A sample stamped `T` was measured at `T − latency`, and an engine
subtracts it on ingest. A consumer that skipped it interpolates every hop to
instants that disagree with every engine stream by a constant: invisible while
everything is still — which is why a *discrete* calibration survived without it
— and wrong the moment anything moves. `0` means unstated.

**`frame_rate`** — this hop's declared rate in Hz, `0` when unstated. It rolls
up per input into `SISPatternInput::frame_rate`, which is what a session picks
its **reference input** by. Without it every session fell back to its *first*
input instead of its slowest, and interpolating toward the sparser signal is
always the larger invention.

### A related fix with no schema change

`SISPatternSegment::stream_topic` must be the topic a subscriber can actually
read poses on. A relation carries the *descriptor* topic it was declared with,
and an engine that passed that through sent subscribers to a key carrying
announcements and no poses — where they waited with empty buffers while every
message they sent looked correct.

An engine holds the resolution in its adoption state and must apply it here. A
stream that has not been adopted resolves to **empty**, never to the descriptor
topic: "this hop is not subscribable" is a state a consumer can act on, and a
key that exists and carries the wrong thing is not.

---

## Upgrading

1. Rebuild every publisher and consumer of `SREdge`, `SRGraph`, `SRNode` (via
   `SRGraph`), and of the pattern query.
2. **Set `SREdge::pinned` on anything meant to outlive its author** — surveyed
   landmarks, calibrated relations, hand-measured mounts. Nothing sets it for
   you, by design.
3. If you implement the engine side: honour `pinned` in retraction beside the
   node flag, **and** provide the deliberate removal path B66 describes. The
   first without the second is a regression.
4. Fill `space_inputs` beside `space_instances`. A consumer that sees
   duplicates named but not realised is talking to an older engine and should
   refuse the candidate rather than estimate from one of them.
5. Fill `sensor_latency_ns`, `frame_rate` and the resolved `stream_topic` from
   adoption state.

Nothing here changes a stored value. The three previous migrations still apply
in order; none is superseded.
