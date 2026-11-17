# Migration to tcn_schema 0.10.3

**Additive for every type but one. Nothing to change unless you publish or read `EyeTrackingMessage`.** No
other type, field, enum value or key changes, and the SQE encoding annotation stays `;tcn_schema=0.10`, so 0.10.x
peers talk to each other unchanged. 0.10.3 builds on 0.10.2 and includes its changes.

## `tcnart_msgs::msg::EyeTrackingMessage` (EyeTracking.idl)

These fields are appended after `valid`:

| field | type | meaning |
|---|---|---|
| `left_pupil_diameter`, `right_pupil_diameter` | float | mm; 0 when not valid |
| `left_pupil_position`, `right_pupil_position` | Vector2 | normalized [0,1] in the eye sensor area, +y up |
| `left_squeeze`, `right_squeeze`, `left_wide`, `right_wide` | float | [0,1] |
| `gaze_space` | XRGazeSpaceType | the space the rays are in (LOCAL: the header's frame_id) |

`valid` is now a documented bit mask, `EYE_VALID_*` in the same file. A field whose bit is clear carries zeros:

| constant | value | field(s) |
|---|---|---|
| `EYE_VALID_CENTER` | 1 | `center` |
| `EYE_VALID_LEFT_GAZE` / `EYE_VALID_RIGHT_GAZE` | 2 / 4 | `left_eye` / `right_eye` |
| `EYE_VALID_LEFT_OPENNESS` / `EYE_VALID_RIGHT_OPENNESS` | 8 / 16 | `left_openness` / `right_openness` |
| `EYE_VALID_VERGENCE` | 32 | `vergence_distance` |
| `EYE_VALID_LEFT_PUPIL_DIAMETER` / `EYE_VALID_RIGHT_PUPIL_DIAMETER` | 64 / 128 | pupil diameters |
| `EYE_VALID_LEFT_PUPIL_POSITION` / `EYE_VALID_RIGHT_PUPIL_POSITION` | 256 / 512 | pupil positions |
| `EYE_VALID_LEFT_EXPRESSION` / `EYE_VALID_RIGHT_EXPRESSION` | 1024 / 2048 | squeeze and wide |

**Wire:** the CDR layout of this one message changes. Before 0.10.3 nothing published it (the HoloLens 2 stack put
gaze into `HandTrackingMessage.gaze`), so no peer is known to break. A 0.10.2 reader cannot decode a 0.10.3 message
reliably; update both sides. The first publisher is tcnart v3's `xr_publish` eye stream (VIVE Focus Vision).
