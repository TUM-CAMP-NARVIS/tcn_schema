# Migration to tcn_schema 0.5.0

A breaking change to the **spatial-relations family**. Every component that
publishes or consumes `SREdge`, `SRTransform` or `SRCalibration` must be
rebuilt.

**Nothing outside `msg/SpatialRelations.idl` changed.** The tracking and SIS
families are untouched by this release; 0.4.0 was their turn.

---

## Why

Two facts had nowhere to live.

**How well a relation is known.** 0.4.0 gave a *measurement* a covariance. A
graph *edge* still had none, so a chain of edges could not state its
uncertainty even when every link's was known — and a chain is exactly where
the question is asked. "How certain is this marker's position with respect to
the world, given the calibration and the two trackers between them?" needs a
covariance on each hop, not on the last one.

**A camera.** The graph could express `world -> camera` and stop. Projecting
into an image plane — the operation every 2D–3D calibration and every overlay
depends on — was not expressible at all.

---

## What changed

### `SRTransformType` gains four discriminants

```idl
enum SRTransformType {
  SRG_TRANSFORM_POSE6D,             // 0, unchanged
  SRG_TRANSFORM_TRANSLATION3D,      // 1, unchanged
  SRG_TRANSFORM_POSE6D_COV,         // 2, new
  SRG_TRANSFORM_TRANSLATION3D_COV,  // 3, new
  SRG_TRANSFORM_PROJECTION,         // 4, new
  SRG_TRANSFORM_POINT2D             // 5, new
};
```

**Appended, never inserted.** CDR is positional and a union's discriminant is
its first four bytes, so renumbering an existing case would make an un-rebuilt
publisher misread rather than fail. For the same reason the covariance is a
*new case* rather than a field appended to case 0: appending to an existing
case is the change that reads as valid and means something else.

An unrecognised discriminant fails the decode, which is the loud outcome. A
0.4.0 consumer receiving case 2 rejects the sample; it does not read it as a
pose.

### `SRCalibration` gains a trailing `covariance`

```idl
sequence<double, 21> covariance;   // 0 or 21
```

`residual` stays. It is what a human reads first and is what this schema has
always promised; the covariance is what a consumer propagating uncertainty
through a chain actually needs. Neither is derivable from the other.

### `SREdgeTemporalType` gains `SRG_EDGE_PROJECTION` (= 5)

A projection is its own temporal value rather than a flag because it changes
what a path *is*:

- It may only be a path's **last hop**. After it the composed value is a 2D
  image point, and a further hop has no depth to use.
- It is **never walked backwards**. Inverting a projection would invent the
  depth it discarded. This is a separate rule from `is_invertible`: an edge
  may be declared invertible and still never be walked past.

A client requesting a path *through* a camera now receives a rejection where
it previously received nothing, because no such edge could exist.

### Two new structs behind the projection cases

```idl
struct SRIntrinsics {
  double fx; double fy;
  double cx; double cy;
  string distortion_model;        // "" | "opencv_radtan" | "opencv_fisheye"
  sequence<double, 8> distortion; // radtan: k1 k2 p1 p2 [k3]
  uint32 image_w; uint32 image_h; // 0 = unknown
};

struct SRPoint2D {
  double u; double v;
  sequence<double, 3> covariance; // 0 or 3: upper triangle of the 2x2, px^2
};
```

**`SRIntrinsics` carries no pose, deliberately.** A camera's extrinsics are
the ordinary `world -> camera` edge, and the projection is the edge after it,
`camera -> image_plane`, with the image plane as its own node. So:

- A camera that **moves** is a dynamic pose edge like any tracked thing. It
  needs no "dynamic projection" and no second mechanism for the same fact.
- **A calibration updates one thing.** Extrinsic calibration writes the pose
  edge, intrinsic calibration writes the intrinsics. Neither re-derives the
  other, and neither can silently disagree with it.

A pre-multiplied 3x4 was considered and rejected for the same reason: it fuses
the two facts, and cannot be decomposed back without ambiguity.

**Distortion travels with the intrinsics** so a projection edge is
self-contained — a consumer holding the edge can undistort without fetching
anything else, and a lens change is one write rather than two that can
disagree. Note that the *uncertainty* propagation through a projection
currently ignores distortion in its Jacobian: applying distortion is cheap and
exact, differentiating it is neither, and its effect on a covariance is small
near the principal point. Carrying the coefficients now means the Jacobian can
be extended later without a second schema break.

---

## The packing, once

Every covariance in this schema is the **upper triangle, row-major**:

| shape | doubles | units | ordering |
| --- | --- | --- | --- |
| 6x6 pose | 21 | m^2 / rad^2 | `(tx, ty, tz, rx, ry, rz)` — translation first |
| 3x3 point | 6 | m^2 | `(x, y, z)` |
| 2x2 image point | 3 | px^2 | `(u, v)` |

Empty means **"not stated"**, which is different from a zero covariance
meaning "known exactly". A length the reader does not expect reads as "not
stated" rather than being truncated.

The triangle rather than the full matrix makes an asymmetric covariance
unrepresentable — the failure that otherwise surfaces as a Cholesky
decomposition failing far from whatever produced it.

---

## To adopt

1. **Rebuild.** A publisher built against 0.4.0 emits and accepts only
   discriminants 0 and 1; it rejects the new cases rather than misreading
   them.
2. **Nothing is required to change.** Cases 0 and 1 are byte-identical, and
   `SRCalibration.covariance` may stay empty. A component that has no
   uncertainty to state and no camera to describe compiles and behaves exactly
   as it did.
3. **State a covariance where you have one.** A calibration tool publishing a
   calibrated edge should use case 2 with the covariance its solver produced,
   not case 0 with a residual alone.
4. **Do not synthesise one where you have none.** An empty sequence is the
   honest value. A fabricated covariance propagates through a chain and comes
   out the far end looking like a measurement.
