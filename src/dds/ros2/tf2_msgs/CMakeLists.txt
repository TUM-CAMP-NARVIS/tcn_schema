tcn_add_idl_package(
        NAME tf2_msgs
        MSG
            "msg/TF2Error.idl"
            "msg/TFMessage.idl"
        ACTION
            "action/LookupTransform.idl"
        DEPS builtin_interfaces std_msgs geometry_msgs
        )
