# Migration to tcn_schema 0.10.4

**Additive. Nothing to change unless you publish or read `EyeTrackingMessage`, or want to send IMU samples.** No
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
