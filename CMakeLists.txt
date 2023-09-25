cmake_minimum_required(VERSION 3.18)
project(tcn_schema VERSION 0.1.0 LANGUAGES CXX)


option(WITH_DDS "Compile with DDS Implementation" OFF)
option(FASTDDS_GEN_JAR_PATH "FastDDS Generator jar path")

# Find requirements
find_package(fastcdr REQUIRED)

set (DDS_LIBRARIES)
if (WITH_DDS)
  find_package(fastdds REQUIRED)
  set (DDS_LIBRARIES ${DDS_LIBRARIES})
else()
  message(STATUS "Building without FastDDS")
  set (DDS_LIBRARIES fastcdr)
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/fastrtps/include")
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FastDDSGeneratorMacros.cmake)

find_file(FASTDDSGEN_JAR
        "fastddsgen.jar"
        PATHS "${FASTDDS_GEN_JAR_PATH}")

set (FASTDDSGEN_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set (FASTDDSGEN_SRC_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/idl")
set (FASTDDS_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/idl")

## IDL FILES
set(TCN_SCHEMA_IDL_BUILTIN_INTERFACES
        "src/dds/ros2/builtin_interfaces/msg/Time_.idl"
        "src/dds/ros2/builtin_interfaces/msg/Duration_.idl"
        # "src/dds/ros2/unique_identifier_msgs/msg/UUID.idl"
        )
set(TCN_SCHEMA_IDL_ROSGRAPH
        "src/dds/ros2/rosgraph_msgs/msg/Clock.idl"
        )
set(TCN_SCHEMA_IDL_STATISTICS
        "src/dds/ros2/statistics_msgs/msg/StatisticDataType.idl"
        "src/dds/ros2/statistics_msgs/msg/StatisticDataPoint.idl"
        "src/dds/ros2/statistics_msgs/msg/MetricsMessage.idl"
        )
set(TCN_SCHEMA_IDL_UNIQUE_IDENTIFIER
        "src/dds/ros2/unique_identifier_msgs/msg/UUID.idl"
        )
set(TCN_SCHEMA_IDL_SENSOR
        "src/dds/ros2/sensor_msgs/msg/NavSatFix.idl"
        "src/dds/ros2/sensor_msgs/msg/MultiEchoLaserScan.idl"
        "src/dds/ros2/sensor_msgs/msg/Temperature.idl"
        "src/dds/ros2/sensor_msgs/msg/LaserScan.idl"
        "src/dds/ros2/sensor_msgs/msg/Image.idl"
        "src/dds/ros2/sensor_msgs/msg/Illuminance.idl"
        "src/dds/ros2/sensor_msgs/msg/JoyFeedback.idl"
        "src/dds/ros2/sensor_msgs/msg/NavSatStatus.idl"
        "src/dds/ros2/sensor_msgs/msg/PointCloud2.idl"
        "src/dds/ros2/sensor_msgs/msg/BatteryState.idl"
        "src/dds/ros2/sensor_msgs/msg/RegionOfInterest.idl"
        "src/dds/ros2/sensor_msgs/msg/LaserEcho.idl"
        "src/dds/ros2/sensor_msgs/msg/TimeReference.idl"
        "src/dds/ros2/sensor_msgs/msg/RelativeHumidity.idl"
        "src/dds/ros2/sensor_msgs/msg/MagneticField.idl"
        "src/dds/ros2/sensor_msgs/msg/JointState.idl"
        "src/dds/ros2/sensor_msgs/msg/JoyFeedbackArray.idl"
        "src/dds/ros2/sensor_msgs/msg/Joy.idl"
        "src/dds/ros2/sensor_msgs/msg/ChannelFloat32.idl"
        "src/dds/ros2/sensor_msgs/msg/CompressedImage.idl"
        "src/dds/ros2/sensor_msgs/msg/Range.idl"
        "src/dds/ros2/sensor_msgs/msg/FluidPressure.idl"
        "src/dds/ros2/sensor_msgs/msg/PointCloud.idl"
        "src/dds/ros2/sensor_msgs/msg/PointField.idl"
        "src/dds/ros2/sensor_msgs/msg/CameraInfo.idl"
        "src/dds/ros2/sensor_msgs/msg/MultiDOFJointState.idl"
        "src/dds/ros2/sensor_msgs/msg/Imu.idl"
        )
set(TCN_SCHEMA_IDL_STD
        "src/dds/ros2/std_msgs/msg/UInt8MultiArray.idl"
        "src/dds/ros2/std_msgs/msg/UInt16.idl"
        "src/dds/ros2/std_msgs/msg/Int8MultiArray.idl"
        "src/dds/ros2/std_msgs/msg/Int16MultiArray.idl"
        "src/dds/ros2/std_msgs/msg/Float64MultiArray.idl"
        "src/dds/ros2/std_msgs/msg/ByteMultiArray.idl"
        "src/dds/ros2/std_msgs/msg/Int8.idl"
        "src/dds/ros2/std_msgs/msg/Int16.idl"
        "src/dds/ros2/std_msgs/msg/Int32.idl"
        "src/dds/ros2/std_msgs/msg/String_.idl"
        "src/dds/ros2/std_msgs/msg/UInt16MultiArray.idl"
        "src/dds/ros2/std_msgs/msg/ColorRGBA.idl"
        "src/dds/ros2/std_msgs/msg/Float64.idl"
        "src/dds/ros2/std_msgs/msg/Char.idl"
        "src/dds/ros2/std_msgs/msg/UInt64.idl"
        "src/dds/ros2/std_msgs/msg/UInt64MultiArray.idl"
        "src/dds/ros2/std_msgs/msg/MultiArrayLayout.idl"
        "src/dds/ros2/std_msgs/msg/Int64MultiArray.idl"
        "src/dds/ros2/std_msgs/msg/MultiArrayDimension.idl"
        "src/dds/ros2/std_msgs/msg/Header.idl"
        "src/dds/ros2/std_msgs/msg/Float32MultiArray.idl"
        "src/dds/ros2/std_msgs/msg/UInt32.idl"
        "src/dds/ros2/std_msgs/msg/Int64.idl"
        "src/dds/ros2/std_msgs/msg/Byte.idl"
        "src/dds/ros2/std_msgs/msg/UInt8.idl"
        "src/dds/ros2/std_msgs/msg/Float32.idl"
        "src/dds/ros2/std_msgs/msg/Empty.idl"
        "src/dds/ros2/std_msgs/msg/Int32MultiArray.idl"
        "src/dds/ros2/std_msgs/msg/UInt32MultiArray.idl"
        "src/dds/ros2/std_msgs/msg/Bool.idl"
        )
set(TCN_SCHEMA_IDL_TF2
        "src/dds/ros2/tf2_msgs/msg/TF2Error.idl"
        "src/dds/ros2/tf2_msgs/msg/TFMessage.idl"
        "src/dds/ros2/tf2_msgs/action/LookupTransform.idl"
        )
set(TCN_SCHEMA_IDL_GEOMETRY
        "src/dds/ros2/geometry_msgs/msg/Point32.idl"
        "src/dds/ros2/geometry_msgs/msg/Accel.idl"
        "src/dds/ros2/geometry_msgs/msg/Vector3.idl"
        "src/dds/ros2/geometry_msgs/msg/TwistStamped.idl"
        "src/dds/ros2/geometry_msgs/msg/PoseWithCovarianceStamped.idl"
        "src/dds/ros2/geometry_msgs/msg/Quaternion.idl"
        "src/dds/ros2/geometry_msgs/msg/Point.idl"
        "src/dds/ros2/geometry_msgs/msg/Pose2D.idl"
        "src/dds/ros2/geometry_msgs/msg/Transform.idl"
        "src/dds/ros2/geometry_msgs/msg/AccelStamped.idl"
        "src/dds/ros2/geometry_msgs/msg/AccelWithCovariance.idl"
        "src/dds/ros2/geometry_msgs/msg/PolygonStamped.idl"
        "src/dds/ros2/geometry_msgs/msg/Pose.idl"
        "src/dds/ros2/geometry_msgs/msg/AccelWithCovarianceStamped.idl"
        "src/dds/ros2/geometry_msgs/msg/Inertia.idl"
        "src/dds/ros2/geometry_msgs/msg/PoseStamped.idl"
        "src/dds/ros2/geometry_msgs/msg/WrenchStamped.idl"
        "src/dds/ros2/geometry_msgs/msg/PoseArray.idl"
        "src/dds/ros2/geometry_msgs/msg/TwistWithCovariance.idl"
        "src/dds/ros2/geometry_msgs/msg/TransformStamped.idl"
        "src/dds/ros2/geometry_msgs/msg/TwistWithCovarianceStamped.idl"
        "src/dds/ros2/geometry_msgs/msg/Vector3Stamped.idl"
        "src/dds/ros2/geometry_msgs/msg/Polygon.idl"
        "src/dds/ros2/geometry_msgs/msg/PoseWithCovariance.idl"
        "src/dds/ros2/geometry_msgs/msg/PointStamped.idl"
        "src/dds/ros2/geometry_msgs/msg/QuaternionStamped.idl"
        "src/dds/ros2/geometry_msgs/msg/Twist.idl"
        "src/dds/ros2/geometry_msgs/msg/Wrench.idl"
        "src/dds/ros2/geometry_msgs/msg/InertiaStamped.idl"
        )

set(TCN_SCHEMA_IDL_SHAPE
        "src/dds/ros2/shape_msgs/msg/MeshTriangle.idl"
        "src/dds/ros2/shape_msgs/msg/Mesh.idl"
        "src/dds/ros2/shape_msgs/msg/Plane.idl"
        "src/dds/ros2/shape_msgs/msg/SolidPrimitive.idl"
        )

set(TCN_SCHEMA_IDL_PCPD
        "src/dds/custom/pcpd_msgs/msg/Types.idl"
        "src/dds/custom/pcpd_msgs/msg/Instance.idl"
        "src/dds/custom/pcpd_msgs/msg/DataflowBufferInfo.idl"
        "src/dds/custom/pcpd_msgs/msg/CameraSensor.idl"
        )


set(TCN_SCHEMA_IDL_DEVICE_HL2
        "src/dds/custom/device_hl2_msgs/msg/Hololens2EyeTracking.idl"
        "src/dds/custom/device_hl2_msgs/msg/Hololens2HandTracking.idl"
        "src/dds/custom/device_hl2_msgs/msg/Hololens2VideoStream.idl"
        "src/dds/custom/device_hl2_msgs/msg/Hololens2AudioStream.idl"
        "src/dds/custom/device_hl2_msgs/msg/Hololens2Sensors.idl"
        )

set(TCN_SCHEMA_IDL_PCPD_RPC
        "src/dds/custom/device_hl2_msgs/rpc/Types.idl"
        "src/dds/custom/device_hl2_msgs/rpc/Hololens2RemoteControl.idl"
        "src/dds/custom/device_hl2_msgs/rpc/Hololens2SpatialMapping.idl"
        "src/dds/custom/device_hl2_msgs/rpc/Hololens2SceneUnderstanding.idl"
        "src/dds/custom/device_hl2_msgs/rpc/Hololens2VoiceInput.idl"
        "src/dds/custom/device_hl2_msgs/rpc/Hololens2Manager.idl"
        )



if (BUILD_SHARED_LIBS)
  add_definitions("-DEPROSIMA_USER_DLL_EXPORT" "-DFASTCDR_DYN_LINK")
else()
  add_definitions("-DFASTCDR_STATIC_LINK")
endif()

FASTDDS_GENERATE_CPP("builtin_interfaces" "msg" TCN_SCHEMA_BUILTIN_INTERFACES_SRC TCN_SCHEMA_BUILTIN_INTERFACES_HDRS TCN_SCHEMA_BUILTIN_INTERFACES_EXPORT "" ${TCN_SCHEMA_IDL_BUILTIN_INTERFACES})
install(FILES ${TCN_SCHEMA_IDL_BUILTIN_INTERFACES} DESTINATION share/tcn_schema/dds/builtin_interfaces/msg)
install(FILES ${TCN_SCHEMA_BUILTIN_INTERFACES_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/builtin_interfaces/msg)

#
#
#
# tcn_schema_std
FASTDDS_GENERATE_CPP("std_msgs" "msg" TCN_SCHEMA_STD_SRC TCN_SCHEMA_STD_HDRS TCN_SCHEMA_STD_EXPORT "" ${TCN_SCHEMA_IDL_STD})
add_library(tcn_schema_std
        ${TCN_SCHEMA_STD_SRC}
        ${TCN_SCHEMA_STD_HDRS}
        ${TCN_SCHEMA_BUILTIN_INTERFACES_SRC}
        ${TCN_SCHEMA_BUILTIN_INTERFACES_HDRS}
        )
target_link_libraries(tcn_schema_std ${DDS_LIBRARIES})
target_include_directories(tcn_schema_std PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )
set_target_properties(tcn_schema_std PROPERTIES CXX_STANDARD 17)
target_compile_definitions(tcn_schema_std PRIVATE ${TCN_SCHEMA_STD_EXPORT})
set_target_properties(tcn_schema_std PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

install(TARGETS tcn_schema_std EXPORT tcn_schema_stdConfig
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(EXPORT tcn_schema_stdConfig DESTINATION share/tcn_schema_std/cmake)
export(TARGETS tcn_schema_std FILE tcn_schema_stdConfig.cmake)
install(FILES ${TCN_SCHEMA_STD_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/std_msgs/msg)
install(FILES ${TCN_SCHEMA_IDL_STD} DESTINATION share/tcn_schema/dds/std_msgs/msg)



#
#
#
# tcn_schema_geometry
FASTDDS_GENERATE_CPP("geometry_msgs" "msg" TCN_SCHEMA_GEOMETRY_SRC TCN_SCHEMA_GEOMETRY_HDRS TCN_SCHEMA_GEOMETRY_EXPORT "tcn_schema_std" ${TCN_SCHEMA_IDL_GEOMETRY})
add_library(tcn_schema_geometry
        ${TCN_SCHEMA_GEOMETRY_SRC}
        ${TCN_SCHEMA_GEOMETRY_HDRS}
        ${TCN_SCHEMA_BUILTIN_INTERFACES_SRC}
        ${TCN_SCHEMA_BUILTIN_INTERFACES_HDRS}
        )
target_link_libraries(tcn_schema_geometry ${DDS_LIBRARIES})
add_dependencies(tcn_schema_geometry tcn_schema_std)
target_link_libraries(tcn_schema_geometry tcn_schema_std)
target_include_directories(tcn_schema_geometry PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )
set_target_properties(tcn_schema_geometry PROPERTIES CXX_STANDARD 17)
target_compile_definitions(tcn_schema_geometry PRIVATE ${TCN_SCHEMA_GEOMETRY_EXPORT})
set_target_properties(tcn_schema_geometry PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

install(TARGETS tcn_schema_geometry EXPORT tcn_schema_geometryConfig
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(EXPORT tcn_schema_geometryConfig DESTINATION share/tcn_schema_geometry/cmake)
export(TARGETS tcn_schema_geometry FILE tcn_schema_geometryConfig.cmake)
install(FILES ${TCN_SCHEMA_GEOMETRY_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/geometry_msgs/msg)
install(FILES ${TCN_SCHEMA_IDL_GEOMETRY} DESTINATION share/tcn_schema/dds/geometry_msgs/msg)




#
#
#
# tcn_schema_pcpd
FASTDDS_GENERATE_CPP("pcpd_msgs" "msg" TCN_SCHEMA_PCPD_SRC TCN_SCHEMA_PCPD_HDRS TCN_SCHEMA_PCPD_EXPORT "tcn_schema_geometry" ${TCN_SCHEMA_IDL_PCPD})
FASTDDS_GENERATE_CPP("pcpd_msgs" "rpc" TCN_SCHEMA_PCPD_RPC_SRC TCN_SCHEMA_PCPD_RPC_HDRS TCN_SCHEMA_PCPD_RPC_EXPORT "tcn_schema_geometry" ${TCN_SCHEMA_IDL_PCPD_RPC})
add_library(tcn_schema_pcpd
        ${TCN_SCHEMA_PCPD_SRC}
        ${TCN_SCHEMA_PCPD_HDRS}
        ${TCN_SCHEMA_PCPD_RPC_SRC}
        ${TCN_SCHEMA_PCPD_RPC_HDRS}
        ${TCN_SCHEMA_BUILTIN_INTERFACES_SRC}
        ${TCN_SCHEMA_BUILTIN_INTERFACES_HDRS}
        )
target_link_libraries(tcn_schema_pcpd ${DDS_LIBRARIES})
add_dependencies(tcn_schema_pcpd tcn_schema_geometry tcn_schema_std)
target_link_libraries(tcn_schema_pcpd tcn_schema_geometry tcn_schema_std)
# probably std at some point
target_include_directories(tcn_schema_pcpd PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )
set_target_properties(tcn_schema_pcpd PROPERTIES CXX_STANDARD 17)
target_compile_definitions(tcn_schema_pcpd PRIVATE ${TCN_SCHEMA_PCPD_EXPORT})
set_target_properties(tcn_schema_pcpd PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

install(TARGETS tcn_schema_pcpd EXPORT tcn_schema_pcpdConfig
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(EXPORT tcn_schema_pcpdConfig DESTINATION share/tcn_schema_pcpd/cmake)
export(TARGETS tcn_schema_pcpd FILE tcn_schema_pcpdConfig.cmake)
install(FILES ${TCN_SCHEMA_PCPD_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pcpd_msgs/msg)
install(FILES ${TCN_SCHEMA_PCPD_RPC_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pcpd_msgs/rpc)
install(FILES ${TCN_SCHEMA_IDL_PCPD} DESTINATION share/tcn_schema/dds/pcpd_msgs/msg)
install(FILES ${TCN_SCHEMA_IDL_PCPD_RPC} DESTINATION share/tcn_schema/dds/pcpd_msgs/rpc)




#
#
#
# tcn_schema_rosgraph
FASTDDS_GENERATE_CPP("rosgraph_msgs" "msg" TCN_SCHEMA_ROSGRAPH_SRC TCN_SCHEMA_ROSGRAPH_HDRS TCN_SCHEMA_ROSGRAPH_EXPORT "tcn_schema_pcpd" ${TCN_SCHEMA_IDL_ROSGRAPH})
add_library(tcn_schema_rosgraph
        ${TCN_SCHEMA_ROSGRAPH_SRC}
        ${TCN_SCHEMA_ROSGRAPH_HDRS}
        ${TCN_SCHEMA_BUILTIN_INTERFACES_SRC}
        ${TCN_SCHEMA_BUILTIN_INTERFACES_HDRS}
        )
target_link_libraries(tcn_schema_rosgraph ${DDS_LIBRARIES})
target_include_directories(tcn_schema_rosgraph PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )
set_target_properties(tcn_schema_rosgraph PROPERTIES CXX_STANDARD 17)
target_compile_definitions(tcn_schema_rosgraph PRIVATE ${TCN_SCHEMA_ROSGRAPH_EXPORT})
set_target_properties(tcn_schema_rosgraph PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

install(TARGETS tcn_schema_rosgraph EXPORT tcn_schema_rosgraphConfig
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(EXPORT tcn_schema_rosgraphConfig DESTINATION share/tcn_schema_rosgraph/cmake)
export(TARGETS tcn_schema_rosgraph FILE tcn_schema_rosgraphConfig.cmake)
install(FILES ${TCN_SCHEMA_ROSGRAPH_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rosgraph_msgs/msg)
install(FILES ${TCN_SCHEMA_IDL_ROSGRAPH} DESTINATION share/tcn_schema/dds/rosgraph_msgs/msg)




#
#
#
# tcn_schema_sensor
FASTDDS_GENERATE_CPP("sensor_msgs" "msg" TCN_SCHEMA_SENSOR_SRC TCN_SCHEMA_SENSOR_HDRS TCN_SCHEMA_SENSOR_EXPORT "tcn_schema_rosgraph" ${TCN_SCHEMA_IDL_SENSOR})
add_library(tcn_schema_sensor
        ${TCN_SCHEMA_SENSOR_SRC}
        ${TCN_SCHEMA_SENSOR_HDRS}
        ${TCN_SCHEMA_BUILTIN_INTERFACES_SRC}
        ${TCN_SCHEMA_BUILTIN_INTERFACES_HDRS}
        )
target_link_libraries(tcn_schema_sensor ${DDS_LIBRARIES})
add_dependencies(tcn_schema_sensor tcn_schema_std)
add_dependencies(tcn_schema_sensor tcn_schema_geometry)
target_link_libraries(tcn_schema_sensor tcn_schema_std)
target_link_libraries(tcn_schema_sensor tcn_schema_geometry)
target_include_directories(tcn_schema_sensor PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )
set_target_properties(tcn_schema_sensor PROPERTIES CXX_STANDARD 17)
target_compile_definitions(tcn_schema_sensor PRIVATE ${TCN_SCHEMA_SENSOR_EXPORT})
set_target_properties(tcn_schema_sensor PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

install(TARGETS tcn_schema_sensor EXPORT tcn_schema_sensorConfig
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(EXPORT tcn_schema_sensorConfig DESTINATION share/tcn_schema_sensor/cmake)
export(TARGETS tcn_schema_sensor FILE tcn_schema_sensorConfig.cmake)
install(FILES ${TCN_SCHEMA_SENSOR_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sensor_msgs/msg)
install(FILES ${TCN_SCHEMA_IDL_SENSOR} DESTINATION share/tcn_schema/dds/sensor_msgs/msg)




#
#
#
# tcn_schema_shape
FASTDDS_GENERATE_CPP("shape_msgs" "msg" TCN_SCHEMA_SHAPE_SRC TCN_SCHEMA_SHAPE_HDRS TCN_SCHEMA_SHAPE_EXPORT "tcn_schema_sensor" ${TCN_SCHEMA_IDL_SHAPE})
add_library(tcn_schema_shape
        ${TCN_SCHEMA_SHAPE_SRC}
        ${TCN_SCHEMA_SHAPE_HDRS}
        ${TCN_SCHEMA_BUILTIN_INTERFACES_SRC}
        ${TCN_SCHEMA_BUILTIN_INTERFACES_HDRS}
        )
target_link_libraries(tcn_schema_shape ${DDS_LIBRARIES})
add_dependencies(tcn_schema_shape tcn_schema_std)
add_dependencies(tcn_schema_shape tcn_schema_geometry)
target_link_libraries(tcn_schema_shape tcn_schema_std)
target_link_libraries(tcn_schema_shape tcn_schema_geometry)
target_include_directories(tcn_schema_shape PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )
set_target_properties(tcn_schema_shape PROPERTIES CXX_STANDARD 17)
target_compile_definitions(tcn_schema_shape PRIVATE ${TCN_SCHEMA_SHAPE_EXPORT})
set_target_properties(tcn_schema_shape PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

install(TARGETS tcn_schema_shape EXPORT tcn_schema_shapeConfig
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(EXPORT tcn_schema_shapeConfig DESTINATION share/tcn_schema_shape/cmake)
export(TARGETS tcn_schema_shape FILE tcn_schema_shapeConfig.cmake)
install(FILES ${TCN_SCHEMA_SHAPE_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/shape_msgs/msg)
install(FILES ${TCN_SCHEMA_IDL_SHAPE} DESTINATION share/tcn_schema/dds/shape_msgs/msg)






#
#
#
# tcn_schema_statistics
FASTDDS_GENERATE_CPP("statistics_msgs" "msg" TCN_SCHEMA_STATISTICS_SRC TCN_SCHEMA_STATISTICS_HDRS TCN_SCHEMA_STATISTICS_EXPORT "tcn_schema_shape" ${TCN_SCHEMA_IDL_STATISTICS})
add_library(tcn_schema_statistics
        ${TCN_SCHEMA_STATISTICS_SRC}
        ${TCN_SCHEMA_STATISTICS_HDRS}
        ${TCN_SCHEMA_BUILTIN_INTERFACES_SRC}
        ${TCN_SCHEMA_BUILTIN_INTERFACES_HDRS}
        )
target_link_libraries(tcn_schema_statistics ${DDS_LIBRARIES})
target_include_directories(tcn_schema_statistics PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )
set_target_properties(tcn_schema_statistics PROPERTIES CXX_STANDARD 17)
target_compile_definitions(tcn_schema_statistics PRIVATE ${TCN_SCHEMA_STATISTICS_EXPORT})
set_target_properties(tcn_schema_statistics PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

install(TARGETS tcn_schema_statistics EXPORT tcn_schema_statisticsConfig
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(EXPORT tcn_schema_statisticsConfig DESTINATION share/tcn_schema_statistics/cmake)
export(TARGETS tcn_schema_statistics FILE tcn_schema_statisticsConfig.cmake)
install(FILES ${TCN_SCHEMA_STATISTICS_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/statistics_msgs/msg)
install(FILES ${TCN_SCHEMA_IDL_STATISTICS} DESTINATION share/tcn_schema/dds/statistics_msgs/msg)





#
#
#
# tcn_schema_tf2
FASTDDS_GENERATE_CPP("tf2_msgs" "msg" TCN_SCHEMA_TF2_SRC TCN_SCHEMA_TF2_HDRS TCN_SCHEMA_TF2_EXPORT "tcn_schema_statistics" ${TCN_SCHEMA_IDL_TF2})
add_library(tcn_schema_tf2
        ${TCN_SCHEMA_TF2_SRC}
        ${TCN_SCHEMA_TF2_HDRS}
        ${TCN_SCHEMA_BUILTIN_INTERFACES_SRC}
        ${TCN_SCHEMA_BUILTIN_INTERFACES_HDRS}
        )
target_link_libraries(tcn_schema_tf2 ${DDS_LIBRARIES})
add_dependencies(tcn_schema_tf2 tcn_schema_std)
add_dependencies(tcn_schema_tf2 tcn_schema_geometry)
target_link_libraries(tcn_schema_tf2 tcn_schema_std)
target_link_libraries(tcn_schema_tf2 tcn_schema_geometry)
target_include_directories(tcn_schema_tf2 PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )
set_target_properties(tcn_schema_tf2 PROPERTIES CXX_STANDARD 17)
target_compile_definitions(tcn_schema_tf2 PRIVATE ${TCN_SCHEMA_TF2_EXPORT})
set_target_properties(tcn_schema_tf2 PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

install(TARGETS tcn_schema_tf2 EXPORT tcn_schema_tf2Config
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(EXPORT tcn_schema_tf2Config DESTINATION share/tcn_schema_tf2/cmake)
export(TARGETS tcn_schema_tf2 FILE tcn_schema_tf2Config.cmake)
install(FILES ${TCN_SCHEMA_TF2_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tf2_msgs/msg)
install(FILES ${TCN_SCHEMA_IDL_TF2} DESTINATION share/tcn_schema/dds/tf2_msgs/msg)



#
#
#
# tcn_schema_unique_identifier
FASTDDS_GENERATE_CPP("unique_identifier_msgs" "msg" TCN_SCHEMA_UNIQUE_IDENTIFIER_SRC TCN_SCHEMA_UNIQUE_IDENTIFIER_HDRS TCN_SCHEMA_UNIQUE_IDENTIFIER_EXPORT "tcn_schema_tf2" ${TCN_SCHEMA_IDL_UNIQUE_IDENTIFIER})
add_library(tcn_schema_unique_identifier
        ${TCN_SCHEMA_UNIQUE_IDENTIFIER_SRC}
        ${TCN_SCHEMA_UNIQUE_IDENTIFIER_HDRS}
        ${TCN_SCHEMA_BUILTIN_INTERFACES_SRC}
        ${TCN_SCHEMA_BUILTIN_INTERFACES_HDRS}
        )
target_link_libraries(tcn_schema_unique_identifier ${DDS_LIBRARIES})
target_include_directories(tcn_schema_unique_identifier PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )
set_target_properties(tcn_schema_unique_identifier PROPERTIES CXX_STANDARD 17)
target_compile_definitions(tcn_schema_unique_identifier PRIVATE ${TCN_SCHEMA_UNIQUE_IDENTIFIER_EXPORT})
set_target_properties(tcn_schema_unique_identifier PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

install(TARGETS tcn_schema_unique_identifier EXPORT tcn_schema_unique_identifierConfig
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(EXPORT tcn_schema_unique_identifierConfig DESTINATION share/tcn_schema_unique_identifier/cmake)
export(TARGETS tcn_schema_unique_identifier FILE tcn_schema_unique_identifierConfig.cmake)
install(FILES ${TCN_SCHEMA_UNIQUE_IDENTIFIER_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/unique_identifier_msgs/msg)
install(FILES ${TCN_SCHEMA_IDL_UNIQUE_IDENTIFIER} DESTINATION share/tcn_schema/dds/unique_identifier_msgs/msg)


# generated cpp/h files need one import from fastrtps, which we provide to avoid pulling the dependency, if it should be build without
if (NOT WITH_DDS)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/fastrtps/include/fastrtps/utils/fixed_size_string.hpp DESTINATION include/fastrtps/utils)
endif()