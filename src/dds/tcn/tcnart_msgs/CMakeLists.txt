tcn_add_idl_package(
        NAME tcnart_msgs
        MSG
            "msg/MediaTypes.idl"
            "msg/DataflowBufferInfo.idl"
            "msg/EyeTracking.idl"
            "msg/HandTracking.idl"
            "msg/HumanPose.idl"
            "msg/JointSet.idl"
            "msg/Imu.idl"
            "msg/PersonTracking.idl"
            "msg/PoseTracking.idl"
            "msg/Presence.idl"
            "msg/StreamDescriptor.idl"
            "msg/TimeRef.idl"
            "msg/VideoStream.idl"
        EXTRA_INSTALL "msg/JointSets.md"
        )
