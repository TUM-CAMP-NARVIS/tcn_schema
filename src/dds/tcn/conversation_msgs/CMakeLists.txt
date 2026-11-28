tcn_add_idl_package(
        NAME conversation_msgs
        MSG
            "msg/Common.idl"
            "msg/Control.idl"
            "msg/Audio.idl"
            "msg/Stt.idl"
            "msg/Llm.idl"
            "msg/Tts.idl"
            "msg/Metrics.idl"
            "msg/Face.idl"
        RPC
            "rpc/Discovery.idl"
        )
