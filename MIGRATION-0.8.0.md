# Migration to tcn_schema 0.8.0

One trailing field, appended to two messages. Additive; nothing changes
position.

**Every component that publishes an `SREdge` naming a stream entry, or that
consumes `SISPatternSegment` and routes stream entries itself, must be
rebuilt.** CDR is positional, so an un-rebuilt peer reads a short message and
defaults the field to `false` — the old "no entry named" meaning. A peer that
never named an entry therefore sees no change at all and needs nothing.

---

## B72 — entry 0 is a real entry

```idl
struct SREdge {
  ...
  boolean pinned;
  boolean stream_item_bound;      // appended
};

struct SISPatternSegment {
  ...
  double frame_rate;
  boolean stream_item_bound;      // appended
};
```

## Why

`stream_item_id` (0.5.0, B53) names which entry of a multi-entry payload feeds a
relation — which target of a `TargetTrackingMessage`, which item of a rigid
transform map. `0` was doing double duty: **"entry 0"** and **"no entry named,
take any"**. Those are different things.

The collision is not hypothetical. Entry ids here are marker ids, and **0 is the
first id every ArUco dictionary hands out** — so the one marker most likely to
be in the scene was the one that could not say it was there.

What went wrong depended on which side read it:

- **The engine** filtered on `stream_item_id != 0`. A relation bound to marker 0
  was read as unbound and fed by *every* entry on its topic — N relations and N
  entries becomes N×N writes, each buffer keeping whichever arrived last.
- **A caller that composes hops itself** (the calibration tool builds its whole
  rig from `SISPatternSegment` and nothing else) matched entries by exact id.
  That made marker 0 right by accident and made the *unbound* case wrong: a hop
  that named no entry only ever received entry 0, never "any entry".

On a single-entry stream neither showed. That is why it survived: a stream with
one entry cannot distinguish "this entry" from "any entry".

## What it means now

`stream_item_bound` says whether `stream_item_id` binds the relation to one
entry. Nothing infers it from the id any more:

- the engine skips an entry only when `stream_item_bound && stream_item_id != id`;
- marker discovery sets it `true` whenever it can recover a marker id, so a
  discovered marker 0 is bound like any other;
- a consumer routing entries treats *unbound* as "every entry", not "entry 0".

## To adopt

**A publisher that names no entry:** nothing. Leave both at their defaults.

**A publisher that names a concrete entry:** set `stream_item_bound = true`,
entry 0 or not. This is the one combination whose meaning changed — a non-zero
`stream_item_id` with `stream_item_bound` left `false` used to mean "bound to
that entry" and now means "unbound, take any entry". Nothing rejects it; it is a
legal message. That silence is the reason this needs a rebuild rather than a
shrug.

**A consumer of `SISPatternSegment` that routes entries itself:** route on the
flag, not on `stream_item_id != 0`. Treating unbound as "entry 0" drops every
other entry on a multi-entry topic.

**A consumer that only reads relations:** nothing, beyond the rebuild.

---

## Not a break

`stream_item_id`'s own comment in `SpatialRelations.idl` said "appended last".
It had not been last since 0.6.0 — `display_name`, `description` and `pinned`
were appended after it. The note is corrected here; no field moved.
