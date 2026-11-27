# tcnart v3: inventory of the tcn_schema elements it uses

2026-11-27. This inventory goes with `2026-11-27-0.11-tcnart-proposal.md`. It covers tcnart v3 on branch
`v3/sqe-b94`, which pins tcn_schema 0.10.7 (an IDL snapshot in `v3/third_party/tcn_schema_idl`, 144 files) and uses the
wire annotation `;tcn_schema=0.10`. sqe_client_cpp 0.6.3 is a second consumer of the schema inside tcnart.

**Paths** are relative to the tcnart repository:

| Shorthand | Path |
| --- | --- |
| `F` | `v3/features/src` |
| `S` | `v3/sqe` |
| `E` | `v3/engine/src` |
| `W` | `v3/wire` |
| `T` | `v3/tools/tcnart_wire_peer` |
| `C` | `v3/capi` |

"tests" means `v3/tests/**`.

**Generated code.** tcnart's generator (idlgen) always generates whole packages: builtin_interfaces, std_msgs,
geometry_msgs, tf2_msgs, tcnart_msgs, pcpd_msgs, and conversation_msgs (registered). It produces JSON codecs, JSON
Schemas, a type registry and C#. So **any** IDL change in those packages reaches tcnart, even for types its C++ never
names (§1.8, §3).

**Counts:**
- about 75 tcnart_msgs/pcpd_msgs types and about 90 enumerators are used by hand-written C++;
- about 125 more travel only through the generated codecs and C#;
- about 45 `tcn::sqe` symbols are used;
- about 20 key families are used;
- 24 sites work around a schema limitation (§5).

## 1. Types by IDL file

### 1.1 `tcnart_msgs/msg/StreamDescriptor.idl`

**StreamDescriptorMessage** is published, subscribed, and used as the `cfg/desc/*` descriptor. About 55 files use it,
about 30 of them tests.

- **Writers:**
  - XR (`F/xr/xr_wire.cpp`, `xr_eye_wire.cpp`): header.frame_id, sensor_type, stream_topic, stream_topic_schema,
    image_compression=Raw, h26x_profile=None, buffer_info (is_valid, is_fixed_size, properties), sensor_latency_ns,
    clock_domain.
  - Markers (`F/camera/marker_publish_op.cpp`): the XR fields, plus marker_type, marker_system and frame_rate.
  - Video (`W/src/video.cpp`): header.frame_id, stream_topic, stream_topic_schema, sensor_type, frame_rate,
    image_width/height/step/format/compression, h26x_profile, h26x_bitrate, buffer_info.properties, clock_domain,
    sensor_latency_ns.
- **Readers:** `F/pose/pose_wire.cpp` (properties `target/...`), `F/pose/pose_receive.cpp`,
  `F/video/stream_lookup.cpp` and `F/video/zenoh_video_ops.cpp`. The geometry lookup reads sensor_type (SURFEL,
  MESH_BITSTREAM) and stream_topic_schema.
- **Wire plumbing:** `W/include/tcnart/wire/descriptor.hpp` (announcer and follower); the SQE link announces
  descriptors (`S/.../{client_port,parts}.hpp`).
- **Untouched:** calib_topic, calib_topic_schema, aac_profile, audio_channels, pose, and image sizes outside video.

### 1.2 `tcnart_msgs/msg/PoseTracking.idl`

- **TargetTrackingMessage** is published and subscribed (about 28 files).
  - Published by xr_publish (`F/xr/xr_wire.cpp`), markers (`F/camera/marker_publish_op.cpp`) and the test peer
    (`T/*`).
  - Read by `F/pose/pose_wire.cpp` and by the SQE link as the relation pose sink.
  - Fields: header (stamp, frame_id), target_origin (identity), and targets[] of Target (target_id, pose,
    pose_covariance, quality, is_tracked), plus status_flags.
  - Constants: TRACKER_STATUS_RUNNING and _VALID; XR_POSE_HMD, _CTRL_LEFT_GRIP, _CTRL_RIGHT_GRIP, _HAND_LEFT_BASE,
    _HAND_RIGHT_BASE and _GAZE.
- **Pose6DMessage** and **Pose6DMapMessage** are read only, by `F/pose/pose_wire.cpp` (message type "auto"): header,
  value, pose_covariance, and poses[].item_id/pose/pose_covariance.

### 1.3 `tcnart_msgs/msg/SpatialRelations.idl`

These are built in `S/src/{space_anchors,control_thread,parts}.cpp`, `F/xr/xr_sqe_part.cpp`,
`F/camera/camera_sqe_part.cpp`, `F/xr/marker_registration_op.cpp` and `F/xr/user_frames.cpp`. They are sent through
sqe_client_cpp's join, update_edge and update_node calls.

- **SRGraph** (the join): name, origin, nodes[], edges[].
- **SRNode**: name, node_type, scope, coord, active, pinned, stream_descriptor_topic.
- **SREdge**: name, source, target, temporal_type, transform (union member `pose6d` only), weight, is_invertible,
  pinned, calibration[], description, stream_descriptor_topic, stream_item_id, stream_item_bound.
- **SRCalibration** is written (method, residual, computed_at) and read back (pose6d, calibration[0].method).
- **CoordinateSystem / CoordinateAxis:** always OpenCV on write; all axes are read when mapping a survey space.
- **Enumerators used:**
  - SRNodeType: GENERIC, SENSOR, TRACKABLE;
  - SRNodeScope: LOCAL, GLOBAL, DOMAIN;
  - SREdgeTemporalType: STATIC, DYNAMIC, STATIC_CALIBRATED, STATIC_UNCALIBRATED.
- **Tests only:** SRNodeView, SREdgeView.
- **Unused:** SRIntrinsics, SRPoint2D, the other SRTransformType cases, SRNodeTemplate,
  SRPose6DWithCovariance, SRTranslation3DWithCovariance.

### 1.4 `tcnart_msgs/rpc/SIS.idl`

Most SIS types are used through sqe_client_cpp and `tcn::sqe::RelationStreamRequest`. These are named directly:

- **SISRelationStreamStatusNotification** is subscribed. It is read for status, detail, route_epoch, engine_epoch and
  blocked_by[]. The link also writes a synthetic one, with output_topic, status, detail,
  route_epoch=0xFFFFFFFF and blocked_by.
- **SISRelationStreamStatus**: ACTIVE, INACTIVE, PENDING, NO_PATH, UNKNOWN_FRAME, CLOCK_DOMAIN_MISMATCH, REJECTED.
  Tokens are built by stripping `SIS_RS_` from the status name.
- **SISBlockingRelation** (source/target client and node, name, temporal_type) and **SISFrameRef** (client, node).
- **SISRelationSampleRequest:**
  - Built in `S/src/sqe_link.cpp` with observer, target, freshness_bias, instant, stamp_ns, wait_ms and
    include_plan=false.
  - SISRelationSampleInstant: LATEST, NOW, AT.
- **SISRelationSampleReply:**
  - Read: status, relation, relation_detail, blocked_by[0].name, stamp_ns, targets (count and [0].pose),
    waited_ms, detail.
  - Not read: engine_id, engine_epoch, clock_domain, inputs, cached.
  - SISRelationSampleStatus: OK, NO_ROUTE, NO_VALUE, REJECTED, TOO_OLD, UNAVAILABLE. A future value is clamped
    to UNAVAILABLE.
- **SISGraphQueryRequest / Reply:** kind=FRAGMENT and client; status and edges[].edge. Uses SIS_GQ_OK and UNAVAILABLE.
  NEIGHBOURS and TRUNCATED appear in tests only.
- **SISEngineInfoRequest / Reply** on `sqe/{engine}/op/info`: who.requester; status and protocol_break.
- **Registry and mutations:**
  - SISGlobalTarget (name) and SISTargetChange (target.name) feed the registry watch. SISTargetAlias appears in tests.
  - SISMutationStatus: UNKNOWN_CLIENT, UNAVAILABLE (and REJECTED in tests).
  - SISOperatorRef.
- **Through sqe_client_cpp only:** SISJoinRequest, SISRelationStreamStartRequest/Reply, SISEvaluationMode, the plan
  types.
- **Never used:** the pattern, explain, transaction-log, fragment-list and operator-mutation types.

### 1.5 `tcnart_msgs/msg/{EyeTracking,HandTracking}.idl`

- **EyeTrackingMessage** is published (`F/xr/xr_eye_wire.cpp`, `xr_publish.cpp`).
  - Fields: header, center, left_eye, right_eye (Ray), vergence_distance, openness, squeeze, wide, pupil_diameter,
    pupil_position, valid, viewpoint, gaze_space=LOCAL, scene_point (always zero).
  - Flags: EYE_VALID_* for CENTER, the left side, VERGENCE and the pupils. The right-side flags appear in the test
    peer only.
- **HandTrackingMessage** is published.
  - Fields: header, head_pose (eye, target, up), gaze_valid, gaze, hands[] of HandState (hand, tracked, aim_valid,
    joints[] of HandJointPose: pose, radius, accuracy).
  - Enumerators: HANDEDNESS_LEFT and _RIGHT; JointPoseAccuracy_High.

### 1.6 `tcnart_msgs/msg/Presence.idl`

**TcnartPresenceMessage** is published by `E/membership.cpp` on `P/str/presence`. It carries the header (frame_id = the
RPC root), heart_beat_counter, and services[] of ServiceInfo (service_name, topic_prefix, service_info).

### 1.7 `tcnart_msgs/msg/Types.idl` and others

- **RigidTransform, Vector2, Vector3, Quaternion, Ray, Target.**
- **SensorType:** NONE, TRACKING_POSE, TRACKING_EYE, TRACKING_HAND, VIDEO_COLOR, VIDEO_DEPTH, VIDEO_IR, SURFEL,
  MESH_BITSTREAM.
- **MarkerType:** ARUCO_MARKER, ARUCO_BOARD, ARUCO_FRACTAL planned (B94 slice, in progress); HMD_POSE in tests.
- **VideoPixelFormat:** BGRA32, L16, L8, NV12, RGBA32. **VideoImageCompression:** Raw, H26x, Zdepth.
  **VideoH26xProfile:** None, H264 Main, H265 Main.
- **XRGazeSpaceType:** LOCAL.
- **VideoStreamMessage** is published and subscribed with **hand-encoded CDR** (zero-copy, `W/src/video.cpp`): header,
  pose, camera_focal_length, camera_principal_point, camera_radial_distortion[3], camera_tangential_distortion[2],
  image_bytes, image.
- **BufferInfo / BufferInfoProperty:** is_valid, is_fixed_size, properties[] (name, value, user_data[].value).
- **CameraModel / CameraModelType:** Unity and Unreal, via pcpd DeviceContextReply.
- **TimeRefRequest / Reply** are in the IDL but unused: `W/src/timeref.cpp` hand-codes a 28-byte reply.

### 1.8 `tcnart_msgs/rpc/Requests.idl` (engine membership and parameters)

| Verb | Request | Reply |
|---|---|---|
| `rpc/ping` | NullRequest | BoolReply |
| `rpc/services` | StringListRequest | StringListReply |
| `rpc/info` | NullRequest | pcpd ProcessInfoReply |
| `rpc/graph` | NullRequest | pcpd GraphReply |
| `rpc/streams` | NullRequest | pcpd StreamAnnouncementReply |
| `rpc/service/<n>/describe` | NullRequest | pcpd ServiceInfoReply |
| `…/parameters/get` | GenericParameterRequest | GenericParameterReply |
| `…/parameters/describe` | (any) | ParameterDescriptorReply |
| `…/parameters/set`, `set_ex` | GenericParameterRequest | ParameterSetReply |

- **Also used:**
  - ParameterChanged (published);
  - ParameterList, Parameter, ParameterValue (PVT_*), ParameterDescriptor(List), EnumEntry;
  - ParameterAccess (PA_READ_ONLY, READ_WRITE_LIVE, READ_WRITE_RESTART);
  - ParameterSetResult and ParameterSetStatus (PSS_*);
  - ParameterListSchema and ParameterSchema;
  - RPCResponseStatus (SUCCESS, ERROR).
- **Unused:** the String/Int/UInt/Float request and reply pairs, BoolRequest, ParameterSchemaReply.
- **Excluded from tcnart's generator:** `tcnart_msgs::rpc::{SRNodeRef, SRGraphPublish*, SRGraphQuery*}`, because
  `rpc/SpatialRelations.h` is shipped as a copy of `msg/SpatialRelations.h`.

### 1.9 `pcpd_msgs`

- **`rpc/ServiceController.idl`:**
  - Published by the engine: ServiceInfoReply (name, service_type, schema); ProcessInfoReply/ProcessInfo (app, node,
    host, pid, started_at, version, plan_hash, generation, rpc_root); GraphReply (graph_json, schema, status).
  - StreamAnnouncement and its Reply (op, topic, payload, role, encoding, mode).
  - DeviceContextReply is a subscribed RPC reply on `<rpc_root>/sensor/<name>/describe`: name, is_valid,
    depth_units_per_meter, frame_rate, and value (a CameraSensor with the color/depth descriptor topics and
    parameters, camera_pose, color2depth_transform). It is read by Unity and Unreal rig code and produced by the test
    peer.
- **`msg/MeshBitstream.idl`:** MeshBitstreamMessage is subscribed (header and CDR blob) and published by the test
  peer.
- **Unused:** Instance, RpcTypesEnum, ServiceType, RecorderStatus, BufferInfoReply, ComponentInfoReply.

### 1.10 ros2 packages

- sensor_msgs: PointCloud2 and PointField (geometry decode and encode); Image in tests.
- std_msgs: Header everywhere; Int32, Float32 and UInt8MultiArray in tests.
- geometry_msgs: Point and PoseStamped (docs and tests).
- builtin_interfaces: Time (Unity).

### 1.11 `conversation_msgs` (plain CDR, no encapsulation header)

- Types: AudioOpusFrame, SttPartial, SttFinal, LlmPartial, LlmFinal, SessionStart, SessionStop, SessionStateEvent,
  SessionState, AudioFormat, MessageHeader, ErrorCode, AppServerInfo.
- Enumerators: AUDIO_CODEC_OPUS and _PCM_F32; CHANNEL_LAYOUT_MONO and _STEREO; STREAM_DIRECTION_UPLINK;
  SESSION_STATE_*; ERROR_*.
- Discovery: `_global/discovery/v1/app-server`.

## 2. `tcn::sqe` contract library

| Symbols | Where |
|---|---|
| `FrameRef` (`parse`, `domain`) | `S/src/frame_text.cpp`, tests |
| `ClientId::parse` | engine config check, camera and xr parts, SQE link |
| `RelationStreamRequest::make`, `StreamTrigger::{on_any,on_reference,fixed_rate}`, `TriggerKind`, `EvaluationMode`, `kDefaultFreshnessBias` | `F/sqe/sqe_relations.cpp`, SQE link |
| `Status`, `status_from_ordinal`, `status_name`, `is_acceptance` | relations feature, control thread |
| `marker_node_name` (3-arg), `MarkerKind` | control thread, marker refs, camera, xr, vision |
| `domain_node_id`, `parse_domain_node_id`, `kDomainNodeSigil`, `kNodeIdSeparator`, `check_domain` | `S/src/domains.cpp`, `frame_text.cpp`, `F/xr/*` |
| `NodeName::parse` | space anchors |
| `descriptor_key`, `tracker_stream_key` (tests) | camera and xr parts |
| `engine_operator_key`, `kOperatorInfo` | SQE link (op/info) |
| `kWireSchemaVersion`, `kCdrMediaType`, `kSchemaVersionParam`, `Error::SchemaVersionMismatch`, `describe` | `W/src/wire_type.cpp`: tcnart re-implements `check_wire_type` |
| `BytesView`, `QueryReply`, `cdr_encoding<T>`, `check_declared_type<T>`, `Error::{Timeout,NoTransport,…}`, `kDefaultQueryTimeoutMs` | SQE link |

Headers: `tcn/sqe/{tracker,validate,relation,status,client_id,wire_type,node_name}.h`.

From sqe_client_cpp, in the SQE link only: SqeClient (open, join, update_edge, remove_edge, update_node,
remove_node, watch_target, latest_status, sample_relation_async, leave), plus RelationStream, TargetWatch,
EngineEvent, RejoinResult, PoseAttachment, LocalInput, CdrCodec and ZenohTransport.

## 3. Generated code in hosts

- **Wire profiles:** CDR (fastcdr through fastddsgen) with `application/cdr;<type>;tcn_schema=0.10` (kTcn), or the type
  in the attachment (kArtekmed). conversation_msgs uses plain CDR.
- **Unity C#** (`hosts/unity/Runtime/Types`, generated): 321 types (tcnart_msgs 225, std_msgs 30, geometry_msgs 29,
  pcpd_msgs 16, conversation_msgs 14, tf2_msgs 5, builtin_interfaces 2).
  - Named by hand-written Unity code: StringListRequest/Reply, RigidTransform, Vector3, Quaternion, CameraModel(Type),
    GenericParameterRequest, ParameterSetReply, ParameterChanged, ParameterDescriptorReply, ParameterValue, PVT_*,
    PSS_*, pcpd DeviceContextReply, builtin_interfaces Time.
  - The generic typed publish/subscribe/query takes any generated class.
- **Unreal:** works on JSON through the registry by type name. Enum values are hard-coded as numbers (§5.20).
- **C ABI:** typed publish/subscribe/query in JSON or CDR. SQE results are flat C structs, not IDL.

## 4. Key expressions

| Key | Notes |
|---|---|
| `P/cfg/desc/<name>` (kTcn) or `P/cfg/dsc/<name>` (kArtekmed), `P/cfg/cal/<name>`, `P/str/<kind>/<name>` (trk, vid, pcl, aud, tri, mesh_bitstream, gen), `P/str/presence`, `P/rpc/...` | `W/src/topics.cpp` |
| `{client}/cfg/desc/{device}` | SQE descriptor (`descriptor_key`) |
| `{client}/cfg/marker/{device}/{marker}` | marker token (the qualified name) |
| `{client}/sis/alive` | liveliness |
| `sqe/{engine}/op/info` | engine generation |
| `{handle}/status`, `{handle}/plan` | via sqe_client_cpp |
| `tcn/time/ref/<name>`, `tcn/time/ntp/<name>` | time sync |
| liveliness `**/rpc`, `**/rpc/streams` | discovery |
| `str/aud/voice`, `_global/discovery/v1/app-server` | voice |
| `<rpc_root>/sensor/<name>/describe` | rig describe (Unity, Unreal) |

## 5. Workarounds (candidates for 0.11)

1. Marker refs `@marker/<kind ordinal>/<system>/<id>` are encoded in SRNode.name and SREdge.source/target, and
   resolved before every join (`S/src/marker_ref.cpp`, `control_thread.cpp`).
2. Domain nodes are the text `@<domain>::<name>`, and frames the text `client/node` (`S/src/frame_text.cpp`,
   `domains.cpp`).
3. `marker_system` is the string `aruco:<DICT>` / `fractal:<config>`; it is both a descriptor field and part of the
   marker name.
4. `vision::MarkerType` and `tcn::sqe::MarkerKind` are pinned to the IDL MarkerType ordinals by static_asserts.
5. SIS_SI_* is mapped by name (the numbering differs). A sample status above UNAVAILABLE is clamped.
6. "Calibration needed" is inferred from `blocked_by[].temporal_type == STATIC_UNCALIBRATED`. Relation tokens are built
   by stripping `SIS_RS_` from the status name.
7. Descriptor `buffer_info.properties` carry structured data: `target/<name>` = XR target id,
   `view/fov_{left,right,up,down}_urad`, `eye/source`, `eye/pupils`.
8. The property `h26x_codec_data` (user_data `data`) carries SPS/PPS.
9. Video:
   - a mono8 mask is sent as SENSOR_TYPE_NONE;
   - depth is inferred from the stream name;
   - float depth is sent as MONO16;
   - legacy RGB24/BGR24 is tolerated.
10. XR poses:
    - an untracked target is an identity pose with is_tracked=false;
    - gaze is a wire-only target;
    - `scene_point` is never computed.
11. The descriptor `clock_domain` string is hashed (FNV) to an id; `""` is the deployment clock.
12. Hand-coded encodings outside the generated codecs:
    - VideoStreamMessage is hand-encoded CDR at fixed offsets;
    - TimeRef is a hand-coded 28-byte reply (the IDL types are unused);
    - MeshStream v2 is the "TMS2" binary, outside the IDL.
13. One sender names the format-A mesh type `tcnart_msgs::msg::MeshBitstreamMessage` (it is in pcpd_msgs).
    PointCloud2 is accepted by SURFEL or by schema text.
14. JSON in strings:
    - GraphReply.graph_json;
    - ServiceInfoReply flattened;
    - enum parameters as PVT_STRING;
    - uint as PVT_INT64;
    - PVT_STRUCT unsupported;
    - ServiceInfo.service_info is a constant string.
15. StreamAnnouncement.role/mode are strings, and ProcessInfo.started_at is ISO-8601 text.
16. SRCalibration.method and SREdge.description are free text naming the tool. A residual of -1 means "not
    measured".
17. Edge and node names follow conventions:
    - `xr_origin_<target>`, `<camera>:::<marker>`, qualified marker names;
    - stream_item_id is the XR target id with stream_item_bound;
    - a template claim is made through stream_descriptor_topic.
18. Anchors are Global GENERIC nodes pinned by tcnart. Calibration tools find uncalibrated edges by convention
    (`SIS_GQ_SEARCH` on STATIC_UNCALIBRATED).
19. conversation_msgs: the ERROR_TIMEOUT and ERROR_CANCELLED enumerators clash with Windows macros; the encoding is
    plain CDR with no annotation.
20. Unreal hard-codes the CameraModelType and ParameterSetStatus ordinals (including "2 reserved").
21. The legacy `tcnart_msgs::rpc` SpatialRelations types are excluded because their header is a copy. idlgen selects
    whole packages, so an unmappable construct breaks configure.
22. protocol_break is read with an extra SISEngineInfoRequest per engine epoch; 0 or no answer means unknown.
23. The presence header.frame_id carries the RPC root, and the video descriptor header.frame_id the stream name.
24. Every node declares coord=OpenCV, converted on tcnart's side; survey poses are converted to the engine's
    convention.
