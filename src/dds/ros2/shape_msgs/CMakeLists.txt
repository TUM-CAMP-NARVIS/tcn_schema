tcn_add_idl_package(
        NAME shape_msgs
        MSG
            "msg/MeshTriangle.idl"
            "msg/Mesh.idl"
            "msg/Plane.idl"
            "msg/SolidPrimitive.idl"
        DEPS builtin_interfaces geometry_msgs
        )
