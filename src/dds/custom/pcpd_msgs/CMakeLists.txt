tcn_add_idl_package(
        NAME pcpd_msgs
        MSG
            "msg/Instance.idl"
            "msg/CameraSensor.idl"
            "msg/MeshBitstream.idl"
        RPC
            "rpc/ServiceController.idl"
        DEPS builtin_interfaces tcnart_msgs
        )
