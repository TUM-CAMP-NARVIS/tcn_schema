# Migration to tcn_schema 0.4.0

A breaking change to the **target-tracking and pose message families**. Every
component that publishes or consumes `Target`, `Marker`, `Pose6DMessage` or
`RigidTransformMapItem` must be rebuilt.

**Nothing outside `msg/Types.idl` and `msg/PoseTracking.idl` changed.** The
spatial-relations and SIS families are untouched by this release.

---

## Why

A measurement can now say how well it is known.

Uncertainty was expressible nowhere in the tracking messages. The nearest
thing was `quality`, a vendor's own tracking-confidence scalar normalised to
0..1 — useful, but not a metric statement and not composable. A consumer
asking "how certain is this marker's position, given the calibration and the
two trackers between it and the world?" had no input to work from.

0.4.0 adds an optional covariance per item.

---

## What changed

Four structs gain one trailing field each.

| struct | new field | length |
| --- | --- | --- |
| `Target` | `pose_covariance` | 0 or 21 |
| `Pose6DMessage` | `pose_covariance` | 0 or 21 |
| `RigidTransformMapItem` | `pose_covariance` | 0 or 21 |
| `Marker` | `position_covariance` | 0 or 6 |

### The packing

A pose covariance is the **upper triangle of a 6x6, row-major**: 21 doubles,
in the order `(tx, ty, tz, rx, ry, rz)` — translation first — with units m^2
on the translational block and rad^2 on the rotational one.

The triangle rather than all 36, for two reasons. It is 168 bytes instead of
288, which matters multiplied by markers times frame rate. More importantly a
6x6 sent as 36 numbers lets a *subtly asymmetric* matrix arrive, which then
fails a Cholesky decomposition somewhere far from whatever produced it.
Sending the triangle makes asymmetry unrepresentable.

A marker's covariance is the upper triangle of a 3x3: 6 doubles, in m^2. A
marker is a 3D point, so this is a 3x3 and **not** a sub-block of any pose's
6x6.

### Empty means "not stated"

An empty sequence is not a zero covariance. Empty means the producer did not
say; zero would assert the value is known exactly, which is a strong claim and
almost never true of a measurement. Consumers must keep them distinct.

Empty costs the 4-byte sequence length — about 2-4% of a `Target`. A mandatory
covariance would have made every pose roughly 3x larger whether or not anyone
could fill it in.

### Why per item, and not a parallel message type

A marker map is routinely **mixed**: some markers are fused or well-observed
and carry a real covariance while others are raw and carry none. A
covariance-aware variant of the whole message would force a producer to
fabricate a covariance for the bare items or discard it for the stated ones.
Per-item optionality represents what actually happens.

### `quality` is unchanged and unrelated

`Target::quality` and `Marker::quality` stay exactly as they were. They are a
vendor's own tracking-confidence scalar; a covariance is a metric statement.
**Neither is derivable from the other**, and code that converts between them is
inventing numbers.

---

## Porting

**A publisher that has no covariance to state** leaves the sequence empty and
is otherwise unchanged. This is the common case.

**A publisher that has one** packs the upper triangle in the order above.

**A consumer** must treat a wrong-length sequence as absent rather than
truncating it: a length that is neither 0 nor the expected count means the
sender and this build disagree about the schema, and the honest reading of a
covariance you cannot parse is that there isn't one.

**Every publisher must be rebuilt.** CDR is positional, so a peer built
against 0.3.0 sending a `Target` to a 0.4.0 consumer is short by one sequence
length, and the consumer reads past the end of the message rather than
failing cleanly. There is no wire-compatible subset; this is a coordinated
upgrade.
