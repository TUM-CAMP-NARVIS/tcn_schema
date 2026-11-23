# Migration to tcn_schema 0.10.5

**One type changes (`PersonTrackingMessage`), three are new (`HumanPoseMessage`, `TimeRefRequest`, `TimeRefReply`).
Nothing to change unless you publish or read person tracks.** The SQE encoding annotation stays `;tcn_schema=0.10`,
so 0.10.x peers talk to each other unchanged. 0.10.5 builds on 0.10.4 and includes its changes.

A joint release of holoscan-tcn-applications (design 70, person tracking that can drive avatars) and tcnart (roadmap
4.6: the persons its `human_pose` feature publishes; time-sync: the time-reference payloads). The joint sets both
person messages use, their units, frames, clocks and rotations are in one registry:
[src/dds/custom/tcnart_msgs/msg/JointSets.md](src/dds/custom/tcnart_msgs/msg/JointSets.md) (installed next to the
IDL).

## `tcnart_msgs::msg::PersonTrackingMessage` (PersonTracking.idl), extended in place

The message added in 0.10.4 (revision `c27d6fff`) grows the fields avatars need. No consumer was bound to the first
layout, so it changes in place instead of becoming a second type.

| change | field |
|---|---|
| added | `PersonTrackingMessage.group` (uint64: frame-group number), `joint_set` (`string<64>`: `coco17`, `halpe26`) |
| bounded | `world_frame` → `string<128>` (the SQE node of the rig calibration's world) |
| added | `Person.previous_id` (uint32, 0 none), `ambiguous` (boolean), `has_root_orientation`, `root_orientation` (Quaternion), `joint_rotations` (`sequence<Quaternion, 32>`), `joint_rotation_confidence` (`sequence<float, 32>`) |
| changed | `Person.joints` `PersonJoint[17]` → `sequence<PersonJoint, 32>`, `bone_lengths` `float[16]` → `sequence<float, 32>` (joint set order) |
| added | `PersonJoint.age_s` (float: s since the joint was last observed) |
| removed | `body_model`, `body_shape`, `body_rotations`, `body_translation` (SMPL parameters are not sent; a fit's result goes into the rotations) |

**Wire:** the CDR layout changes: a 0.10.4 reader cannot decode a 0.10.5 person message. Update writers and readers
together (holoscan-tcn-applications' converters; tcnart's generated codecs).

## `tcnart_msgs::msg::HumanPoseMessage` (HumanPose.idl, new)

The persons of one site as tcnart's `human_pose` authority sees them: stabilised, merged from source tracks, linked to
session participants; what Unity and Unreal avatars read.

| field | type | meaning |
|---|---|---|
| `header` | std_msgs Header | stamp: deployment clock time of the poses; frame_id: a Global SQE node |
| `joint_set` | `string<64>` | e.g. `tcnart_body_v1` |
| `source` | `string<64>` | the source topic (e.g. holoscan-tcn's persons stream) |
| `persons` | `sequence<HumanPose, 16>` | |

`HumanPose`: `person` (uint32, stable, never reused within a session), `participant` (`string<32>`, empty:
anonymous), `state` (`HUMAN_POSE_TRACKED`/`COASTING`/`LOST`), `source_ids` (`sequence<uint32, 8>`, newest first),
`joints` (`sequence<HumanJoint, 32>`), `bone_lengths` (`sequence<float, 32>`). `HumanJoint`: `position` (Vector3, m),
`rotation` (Quaternion, relative to the rest pose), `confidence`, `flags` (`HUMAN_JOINT_OBSERVED` 1, `DERIVED` 2,
`DEFAULTED` 4, `HAS_ROTATION` 8).

## `tcnart_msgs::msg::TimeRefRequest`, `TimeRefReply` (TimeRef.idl, new)

The payloads of the time-reference query (`tcn/time/ref/<name>`): request `{int64 t1}`, reply
`{int64 t1, t2, t3}`, ns since the Unix epoch of each sender's `CLOCK_REALTIME`; CDR with the encapsulation header,
12 and 28 bytes. Until now tcnart wrote them by hand; the bytes are unchanged (tested against the protocol's examples).

## Picking it up

Conan: `tcn_schema/0.10.5@artekmed/stable`. tcnart: refresh `v3/third_party/tcn_schema_idl` with `sync_idl.py`
(`--version 0.10.5`) and add `HumanPoseMessage` (and the TimeRef types) to its host type list.
