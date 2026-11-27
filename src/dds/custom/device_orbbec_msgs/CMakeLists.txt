tcn_add_idl_package(
        NAME device_orbbec_msgs
        MSG
            "msg/OrbbecTypes.idl"
            "msg/OrbbecSensors.idl"
        RPC
            "rpc/OrbbecManager.idl"
        DEPS builtin_interfaces std_msgs geometry_msgs tcnart_msgs
        )
