# Joint sets and person contracts

The registry of skeleton joint sets shared by `PersonTrackingMessage` (`PersonTracking.idl`: tracks from a
multi-camera tracker, holoscan-tcn-applications) and `HumanPoseMessage` (`HumanPose.idl`: tcnart's stabilised persons
for avatars). `joint_set` names the order of every person's `joints` and `bone_lengths` (and of
`joint_rotations`); a producer sends one joint set per stream, and the stream's descriptor names it too. Both messages
bound joints at 32, so one consumer buffer fits either. A new set gets its section here with its first producer.

| set | joints | root | first producer |
|---|---|---|---|
| `coco17` | 17 | mid-hip (11, 12) | holoscan-tcn pose tracking |
| `halpe26` | 26 | mid-hip (11, 12) | holoscan-tcn pose tracking (design 70, phase 5; opt-in) |
| `tcnart_body_v1` | 20 | pelvis (0) | tcnart `human_pose` |

## Coordinates, time, smoothing (both messages)

- **Positions:** metres, right-handed, in the message's frame, a Global SQE node name: `world_frame` in
  `PersonTrackingMessage` (the node of the rig calibration's world, which the reconstruction's mesh uses too),
  `header.frame_id` in `HumanPoseMessage`. `up` (`PersonTrackingMessage` only) is the unit vector pointing up in
  that frame; it differs per site (each site's calibration world) and is sent in every message and in the
  descriptor. `HumanPoseMessage` has no `up`: hosts take it from the frame (the SQE, the rig), and its rotations do
  not depend on it.
- **Time:** `PersonTrackingMessage.header.stamp` is the capture time of the frame group, in the same clock as the mesh
  and the camera streams of the same capture; `HumanPoseMessage.header.stamp` is the deployment clock time of the
  interpolated pose. Each stream's descriptor names its clock domain.
- **Rotations:** `Quaternion` (x, y, z, w) in the message's frame, relative to the joint set's rest pose (below).
- **Prediction:** a `PERSON_STATE_PREDICTED` person's joints move with its root (the last observed pose carried
  along the root's prediction), every joint `observed = false`, `age_s` the time since that joint was last observed.
- **Ids:** never reused within a stream. When a track continues another one under a new id, `previous_id` names the
  old id from the frame the link is made on.

## The descriptor of a `PersonTrackingMessage` stream (`StreamDescriptor`, `buffer_info.properties`)

| property | value |
|---|---|
| `joint_set` | `coco17` / `halpe26` |
| `world_frame` | the SQE node |
| `handedness`, `units` | `right`, `m` |
| `up` | `x,y,z` |
| `clock_domain` | `capture:<host>` (live: the host whose clock stamps the capture) or `recording:<id>` (replay) |
| `smoothing` | `one_euro` or `off`; with `one_euro` also `min_cutoff_hz`, `beta`, `d_cutoff_hz`, `bone_tol` |
| `rate_hz` | the rate the stream is published at (one message per frame group) |

## `coco17`

Joints (17): 0 nose, 1 left eye, 2 right eye, 3 left ear, 4 right ear, 5 left shoulder, 6 right shoulder, 7 left
elbow, 8 right elbow, 9 left wrist, 10 right wrist, 11 left hip, 12 right hip, 13 left knee, 14 right knee, 15 left
ankle, 16 right ankle. The root is the mid-point of the hips (11, 12).

Bones (16), `bone_lengths[i]` joins joints (a, b):

| i | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| (a,b) | 0,1 | 0,2 | 1,3 | 2,4 | 5,6 | 5,7 | 7,9 | 6,8 | 8,10 | 5,11 | 6,12 | 11,12 | 11,13 | 13,15 | 12,14 | 14,16 |

## `halpe26`

Joints (26): 0–16 as `coco17`, then 17 head top, 18 neck, 19 hip centre, 20 left big toe, 21 right big toe, 22 left
small toe, 23 right small toe, 24 left heel, 25 right heel. The hip centre (19) is the model's own pelvis joint; the
track's root is the mid-point of the hips (11, 12), as for `coco17`.

Bones (26), `bone_lengths[i]` joins joints (a, b): 0–15 as `coco17`, then

| i | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 | 24 | 25 |
|---|---|---|---|---|---|---|---|---|---|---|
| (a,b) | 17,18 | 18,19 | 11,19 | 12,19 | 15,20 | 15,22 | 15,24 | 16,21 | 16,23 | 16,25 |

(head top–neck, neck–hip centre, left and right hip–hip centre, left ankle–big toe, –small toe, –heel, the same on the
right). A `coco17` consumer reads a `halpe26` message by its first 17 joints and 16 bones. First producer: HTA design 70 phase 5 (opt-in, `pose_tracking --joint-set halpe26`).

## `tcnart_body_v1`

Joints (20): 0 pelvis, 1 spine, 2 chest, 3 neck, 4 head, 5 head top, 6 left shoulder, 7 right shoulder, 8 left elbow,
9 right elbow, 10 left wrist, 11 right wrist, 12 left hip, 13 right hip, 14 left knee, 15 right knee, 16 left ankle,
17 right ankle, 18 left toe, 19 right toe. The root is the pelvis (0).

Bones (19). `bone_lengths[i]` joins joints (a, b); a is the parent. The bone's rotation is the rotation of joint a.

| i | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| (a,b) | 0,1 | 1,2 | 2,3 | 3,4 | 4,5 | 3,6 | 6,8 | 8,10 | 3,7 | 7,9 | 9,11 | 0,12 | 12,14 | 14,16 | 16,18 | 0,13 | 13,15 | 15,17 | 17,19 |

The bones in words:
- pelvis → spine → chest → neck → head → head top;
- neck → shoulder → elbow → wrist, on each side;
- pelvis → hip → knee → ankle → toe, on each side.

The neck is the mid-point of the shoulders, so the neck–shoulder bones are half the shoulder width.

**Rest pose.** It is a T-pose in a canonical body frame B, right-handed: +x is the person's left, +y is up, +z is
forward (the direction the person faces). At rest every joint's frame equals B. The rest direction of each bone in B:

| bones | direction in B |
|---|---|
| 0-1, 1-2, 2-3, 3-4, 4-5 (spine, neck, head) | +y |
| 3-6, 6-8, 8-10 (left shoulder and arm), 0-12 (left hip) | +x |
| 3-7, 7-9, 9-11 (right shoulder and arm), 0-13 (right hip) | −x |
| 12-14, 14-16, 13-15, 15-17 (legs) | −y |
| 16-18, 17-19 (feet) | +z |

**Rotations.** A joint's `rotation` q is the orientation of that joint's frame in the message's frame, so the joint's
outgoing bone points along `q · d_rest` there. All identity rotations put the T-pose with B's axes on the frame's axes.
A z-up frame therefore needs a non-identity base rotation, so rotations never depend on `up`.

A joint with no rotation information has `HUMAN_JOINT_HAS_ROTATION` cleared, and its rotation is identity and
ignored. Pelvis, chest and head carry rotations derived from the positions (hips, shoulders, nose and ears). Limb
twist and feet stay without rotation until a source with rotations exists.

## Rotations

`PersonTrackingMessage`: `root_orientation` and `joint_rotations` are sent only from a body-model fit with temporal
smoothing; until then `has_root_orientation` is false and the sequences are empty. `HumanPoseMessage`: a joint's
`rotation` is valid when `HUMAN_JOINT_HAS_ROTATION` is set. The rest pose (bone axes per joint set) is defined here
together with each set's first producer of rotations.
