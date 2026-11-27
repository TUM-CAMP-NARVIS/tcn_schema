tcn_add_idl_package(
        NAME tcnart_msgs
        MSG
            "msg/CameraModel.idl"
            "msg/DataflowBufferInfo.idl"
            "msg/Logging.idl"
            "msg/PoseTracking.idl"
            "msg/EyeTracking.idl"
            "msg/Imu.idl"
            "msg/PersonTracking.idl"
            "msg/HumanPose.idl"
            "msg/TimeRef.idl"
            "msg/HandTracking.idl"
            "msg/XRInput.idl"
            "msg/XRRuntime.idl"
            "msg/AudioStream.idl"
            "msg/VideoStream.idl"
            "msg/Presence.idl"
            "msg/StreamDescriptor.idl"
            "msg/SpatialRelations.idl"
            "msg/Types.idl"
        RPC
            "rpc/GenericManager.idl"
            "rpc/Requests.idl"
            "rpc/SpatialRelations.idl"
            "rpc/SIS.idl"
        DEPS builtin_interfaces std_msgs geometry_msgs
        EXTRA_INSTALL "msg/JointSets.md"
        )

# Round-trip tests of the generated message types (need the generated library, hence the option).
option(TCN_SCHEMA_BUILD_MSG_TESTS "Build the tcnart_msgs CDR round-trip tests" OFF)
if(TCN_SCHEMA_BUILD_MSG_TESTS)
  enable_testing()
  add_executable(tcnart_msgs_person_tracking_test test/test_person_tracking.cpp)
  target_link_libraries(tcnart_msgs_person_tracking_test PRIVATE tcn_schema_tcnart_msgs
          tcn_schema_builtin_interfaces tcn_schema_std_msgs tcn_schema_geometry_msgs ${DDS_LIBRARIES})
  set_target_properties(tcnart_msgs_person_tracking_test PROPERTIES CXX_STANDARD 17)
  add_test(NAME tcnart_msgs_person_tracking COMMAND tcnart_msgs_person_tracking_test)
  add_executable(tcnart_msgs_human_pose_test test/test_human_pose.cpp)
  target_link_libraries(tcnart_msgs_human_pose_test PRIVATE tcn_schema_tcnart_msgs
          tcn_schema_builtin_interfaces tcn_schema_std_msgs tcn_schema_geometry_msgs ${DDS_LIBRARIES})
  set_target_properties(tcnart_msgs_human_pose_test PROPERTIES CXX_STANDARD 17)
  add_test(NAME tcnart_msgs_human_pose COMMAND tcnart_msgs_human_pose_test)
endif()
