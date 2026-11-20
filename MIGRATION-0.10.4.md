# Migration to tcn_schema 0.10.4

**Additive. Nothing to change unless you publish or read `EyeTrackingMessage`, or want to send IMU samples or person tracks.** No
other type, field, enum value or key changes, and the SQE encoding annotation stays `;tcn_schema=0.10`. 0.10.4
builds on 0.10.3 and includes its changes.

## `tcnart_msgs::msg::EyeTrackingMessage` (EyeTracking.idl)

Appended after `gaze_space`:

| field | type | meaning |
|---|---|---|
| `scene_point` | Vector2 | the gaze point in the image of the head-mounted scene (world-facing) camera, normalized [0,1], +x right, +y down (image convention) |

with the bit `EYE_VALID_SCENE_POINT` (4096). Eye trackers with a scene camera (Tobii Pro Glasses 2/3, Pupil) report
it directly; it is what a gaze overlay on the scene video draws.

**Wire:** the CDR layout of this message changes again (one Vector2 appended): a 0.10.3 reader cannot decode a
0.10.4 message reliably; update both sides.

## `tcnart_msgs::msg::ImuMessage` (Imu.idl, new)

| field | type | meaning |
|---|---|---|
| `header` | std_msgs Header | stamp: the sample's time on the publisher's clock; frame_id: the IMU's frame |
| `acc_device_ns`, `acc` | int64, Vector3 | the accelerometer sample's device time (ns) and value (m/s^2) |
| `gyro_device_ns`, `gyro` | int64, Vector3 | the gyroscope sample's device time (ns) and value (rad/s) |
| `valid` | uint32 | `IMU_VALID_ACC` (1), `IMU_VALID_GYRO` (2); a field whose bit is clear carries zeros |

The layout follows holoscan-tcn-applications' recording type `tcn.ImuSample` (Azure Kinect IMU, Tobii glasses), so
recorded IMU samples convert one to one.

## `tcnart_msgs::msg::PersonTrackingMessage` (PersonTracking.idl, new; a later revision of 0.10.4)

Person tracks from a multi-camera skeleton tracker (holoscan-tcn-applications, design 65): `header` (stamp: capture time
of the frame group), `world_frame`, `up`, `camera_count` (<= 8), `persons` (<= 16) and `camera_reprojection_px` (per
camera, median residual, pixels). Each `Person` has `id`, `state` (`PERSON_STATE_TENTATIVE`/`CONFIRMED`/`PREDICTED`),
`confidence`, `age_s`, `last_seen_ns`, `root`, 17 `PersonJoint`s in COCO-17 order (position m, confidence, `observed`),
16 `bone_lengths` (m, NaN until frozen), per-camera `PersonCameraView`s (box, joints used), `role` and
`role_confidence` (-1 / 0 while unassigned) and the reserved, normally empty body-model fields (`body_model`,
`body_shape`, `body_rotations`, `body_translation`).

Purely additive: no existing type changes, the SQE encoding annotation stays `;tcn_schema=0.10`. It ships as a new
Conan revision of 0.10.4 (no version change); a consumer needs the revision that contains `PersonTracking.idl`.
