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
        EXTRA_INSTALL "msg/JointSets.md"
        )

