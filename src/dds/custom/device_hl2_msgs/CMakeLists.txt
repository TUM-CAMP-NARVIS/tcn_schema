tcn_add_idl_package(
        NAME device_hl2_msgs
        MSG
            "msg/Hololens2Enums.idl"
            "msg/Hololens2Sensors.idl"
        RPC
            "rpc/Hololens2RemoteControl.idl"
            "rpc/Hololens2SpatialMapping.idl"
            "rpc/Hololens2SceneUnderstanding.idl"
            "rpc/Hololens2VoiceInput.idl"
            "rpc/Hololens2Manager.idl"
        )
