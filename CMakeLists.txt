cmake_minimum_required(VERSION 3.18)
project(hl2comm_idl VERSION 1.0.0 LANGUAGES CXX)


option(WITH_DDS "Compile with DDS Implementation" OFF)
option(FASTDDS_GEN_JAR_PATH "FastDDS Generator jar path")

# Find requirements
find_package(fastcdr REQUIRED)

set (DDS_LIBRARIES)
if (WITH_DDS)
  find_package(fastdds REQUIRED)
  set (DDS_LIBRARIES ${DDS_LIBRARIES})
else()
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
set(HL2COMM_IDL_IDL_BUILTIN_INTERFACES
        "idl/builtin_interfaces/msg/Time_.idl"
        "idl/builtin_interfaces/msg/Duration_.idl"
        # "idl/unique_identifier_msgs/msg/UUID.idl"
        )
set(HL2COMM_IDL_IDL_ROSGRAPH
        "idl/rosgraph_msgs/msg/Clock.idl"
        )
set(HL2COMM_IDL_IDL_STATISTICS
        "idl/statistics_msgs/msg/StatisticDataType.idl"
        "idl/statistics_msgs/msg/StatisticDataPoint.idl"
        "idl/statistics_msgs/msg/MetricsMessage.idl"
        )
set(HL2COMM_IDL_IDL_UNIQUE_IDENTIFIER
        "idl/unique_identifier_msgs/msg/UUID.idl"
        )
set(HL2COMM_IDL_IDL_SENSOR
        "idl/sensor_msgs/msg/NavSatFix.idl"
        "idl/sensor_msgs/msg/MultiEchoLaserScan.idl"
        "idl/sensor_msgs/msg/Temperature.idl"
        "idl/sensor_msgs/msg/LaserScan.idl"
        "idl/sensor_msgs/msg/Image.idl"
        "idl/sensor_msgs/msg/Illuminance.idl"
        "idl/sensor_msgs/msg/JoyFeedback.idl"
        "idl/sensor_msgs/msg/NavSatStatus.idl"
        "idl/sensor_msgs/msg/PointCloud2.idl"
        "idl/sensor_msgs/msg/BatteryState.idl"
        "idl/sensor_msgs/msg/RegionOfInterest.idl"
        "idl/sensor_msgs/msg/LaserEcho.idl"
        "idl/sensor_msgs/msg/TimeReference.idl"
        "idl/sensor_msgs/msg/RelativeHumidity.idl"
        "idl/sensor_msgs/msg/MagneticField.idl"
        "idl/sensor_msgs/msg/JointState.idl"
        "idl/sensor_msgs/msg/JoyFeedbackArray.idl"
        "idl/sensor_msgs/msg/Joy.idl"
        "idl/sensor_msgs/msg/ChannelFloat32.idl"
        "idl/sensor_msgs/msg/CompressedImage.idl"
        "idl/sensor_msgs/msg/Range.idl"
        "idl/sensor_msgs/msg/FluidPressure.idl"
        "idl/sensor_msgs/msg/PointCloud.idl"
        "idl/sensor_msgs/msg/PointField.idl"
        "idl/sensor_msgs/msg/CameraInfo.idl"
        "idl/sensor_msgs/msg/MultiDOFJointState.idl"
        "idl/sensor_msgs/msg/Imu.idl"
        )
set(HL2COMM_IDL_IDL_STD
        "idl/std_msgs/msg/UInt8MultiArray.idl"
        "idl/std_msgs/msg/UInt16.idl"
        "idl/std_msgs/msg/Int8MultiArray.idl"
        "idl/std_msgs/msg/Int16MultiArray.idl"
        "idl/std_msgs/msg/Float64MultiArray.idl"
        "idl/std_msgs/msg/ByteMultiArray.idl"
        "idl/std_msgs/msg/Int8.idl"
        "idl/std_msgs/msg/Int16.idl"
        "idl/std_msgs/msg/Int32.idl"
        "idl/std_msgs/msg/String_.idl"
        "idl/std_msgs/msg/UInt16MultiArray.idl"
        "idl/std_msgs/msg/ColorRGBA.idl"
        "idl/std_msgs/msg/Float64.idl"
        "idl/std_msgs/msg/Char.idl"
        "idl/std_msgs/msg/UInt64.idl"
        "idl/std_msgs/msg/UInt64MultiArray.idl"
        "idl/std_msgs/msg/MultiArrayLayout.idl"
        "idl/std_msgs/msg/Int64MultiArray.idl"
        "idl/std_msgs/msg/MultiArrayDimension.idl"
        "idl/std_msgs/msg/Header.idl"
        "idl/std_msgs/msg/Float32MultiArray.idl"
        "idl/std_msgs/msg/UInt32.idl"
        "idl/std_msgs/msg/Int64.idl"
        "idl/std_msgs/msg/Byte.idl"
        "idl/std_msgs/msg/UInt8.idl"
        "idl/std_msgs/msg/Float32.idl"
        "idl/std_msgs/msg/Empty.idl"
        "idl/std_msgs/msg/Int32MultiArray.idl"
        "idl/std_msgs/msg/UInt32MultiArray.idl"
        "idl/std_msgs/msg/Bool.idl"
        )
set(HL2COMM_IDL_IDL_TF2
        "idl/tf2_msgs/msg/TF2Error.idl"
        "idl/tf2_msgs/msg/TFMessage.idl"
        "idl/tf2_msgs/action/LookupTransform.idl"
        )
set(HL2COMM_IDL_IDL_GEOMETRY
        "idl/geometry_msgs/msg/Point32.idl"
        "idl/geometry_msgs/msg/Accel.idl"
        "idl/geometry_msgs/msg/Vector3.idl"
        "idl/geometry_msgs/msg/TwistStamped.idl"
        "idl/geometry_msgs/msg/PoseWithCovarianceStamped.idl"
        "idl/geometry_msgs/msg/Quaternion.idl"
        "idl/geometry_msgs/msg/Point.idl"
        "idl/geometry_msgs/msg/Pose2D.idl"
        "idl/geometry_msgs/msg/Transform.idl"
        "idl/geometry_msgs/msg/AccelStamped.idl"
        "idl/geometry_msgs/msg/AccelWithCovariance.idl"
        "idl/geometry_msgs/msg/PolygonStamped.idl"
        "idl/geometry_msgs/msg/Pose.idl"
        "idl/geometry_msgs/msg/AccelWithCovarianceStamped.idl"
        "idl/geometry_msgs/msg/Inertia.idl"
        "idl/geometry_msgs/msg/PoseStamped.idl"
        "idl/geometry_msgs/msg/WrenchStamped.idl"
        "idl/geometry_msgs/msg/PoseArray.idl"
        "idl/geometry_msgs/msg/TwistWithCovariance.idl"
        "idl/geometry_msgs/msg/TransformStamped.idl"
        "idl/geometry_msgs/msg/TwistWithCovarianceStamped.idl"
        "idl/geometry_msgs/msg/Vector3Stamped.idl"
        "idl/geometry_msgs/msg/Polygon.idl"
        "idl/geometry_msgs/msg/PoseWithCovariance.idl"
        "idl/geometry_msgs/msg/PointStamped.idl"
        "idl/geometry_msgs/msg/QuaternionStamped.idl"
        "idl/geometry_msgs/msg/Twist.idl"
        "idl/geometry_msgs/msg/Wrench.idl"
        "idl/geometry_msgs/msg/InertiaStamped.idl"
        )
set(HL2COMM_IDL_IDL_SHAPE
        "idl/shape_msgs/msg/MeshTriangle.idl"
        "idl/shape_msgs/msg/Mesh.idl"
        "idl/shape_msgs/msg/Plane.idl"
        "idl/shape_msgs/msg/SolidPrimitive.idl"
        )

set(HL2COMM_IDL_IDL_PCPD
        "idl/pcpd_msgs/msg/Types.idl"
        "idl/pcpd_msgs/msg/Instance.idl"
        "idl/pcpd_msgs/msg/DataflowBufferInfo.idl"
        "idl/pcpd_msgs/msg/CameraSensor.idl"
        "idl/pcpd_msgs/msg/Hololens2EyeTracking.idl"
        "idl/pcpd_msgs/msg/Hololens2HandTracking.idl"
        "idl/pcpd_msgs/msg/Hololens2VideoStream.idl"
        "idl/pcpd_msgs/msg/Hololens2AudioStream.idl"
        "idl/pcpd_msgs/msg/Hololens2Sensors.idl"
        )

set(HL2COMM_IDL_IDL_PCPD_RPC
        "idl/pcpd_msgs/rpc/Types.idl"
        "idl/pcpd_msgs/rpc/Hololens2RemoteControl.idl"
        "idl/pcpd_msgs/rpc/Hololens2SpatialMapping.idl"
        "idl/pcpd_msgs/rpc/Hololens2SceneUnderstanding.idl"
        "idl/pcpd_msgs/rpc/Hololens2VoiceInput.idl"
        "idl/pcpd_msgs/rpc/Hololens2Manager.idl"
        )

if (BUILD_SHARED_LIBS)
  add_definitions("-DEPROSIMA_USER_DLL_EXPORT" "-DFASTCDR_DYN_LINK")
else()
  add_definitions("-DFASTCDR_STATIC_LINK")
endif()

FASTDDS_GENERATE_CPP("builtin_interfaces" "msg" HL2COMM_IDL_BUILTIN_INTERFACES_SRC HL2COMM_IDL_BUILTIN_INTERFACES_HDRS HL2COMM_IDL_BUILTIN_INTERFACES_EXPORT "" ${HL2COMM_IDL_IDL_BUILTIN_INTERFACES})
install(FILES ${HL2COMM_IDL_IDL_BUILTIN_INTERFACES} DESTINATION share/pcpd_dds/builtin_interfaces/msg)
install(FILES ${HL2COMM_IDL_BUILTIN_INTERFACES_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/builtin_interfaces/msg)

#
#
#
# hl2comm_idl_std
FASTDDS_GENERATE_CPP("std_msgs" "msg" HL2COMM_IDL_STD_SRC HL2COMM_IDL_STD_HDRS HL2COMM_IDL_STD_EXPORT "" ${HL2COMM_IDL_IDL_STD})
add_library(hl2comm_idl_std
        ${HL2COMM_IDL_STD_SRC}
        ${HL2COMM_IDL_STD_HDRS}
        ${HL2COMM_IDL_BUILTIN_INTERFACES_SRC}
        ${HL2COMM_IDL_BUILTIN_INTERFACES_HDRS}
        )
target_link_libraries(hl2comm_idl_std ${DDS_LIBRARIES})
target_include_directories(hl2comm_idl_std PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )
set_target_properties(hl2comm_idl_std PROPERTIES CXX_STANDARD 17)
target_compile_definitions(hl2comm_idl_std PRIVATE ${HL2COMM_IDL_STD_EXPORT})
set_target_properties(hl2comm_idl_std PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

install(TARGETS hl2comm_idl_std EXPORT hl2comm_idl_stdConfig
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(EXPORT hl2comm_idl_stdConfig DESTINATION share/hl2comm_idl_std/cmake)
export(TARGETS hl2comm_idl_std FILE hl2comm_idl_stdConfig.cmake)
install(FILES ${HL2COMM_IDL_STD_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/std_msgs/msg)
install(FILES ${HL2COMM_IDL_IDL_STD} DESTINATION share/pcpd_dds/std_msgs/msg)



#
#
#
# hl2comm_idl_geometry
FASTDDS_GENERATE_CPP("geometry_msgs" "msg" HL2COMM_IDL_GEOMETRY_SRC HL2COMM_IDL_GEOMETRY_HDRS HL2COMM_IDL_GEOMETRY_EXPORT "hl2comm_idl_std" ${HL2COMM_IDL_IDL_GEOMETRY})
add_library(hl2comm_idl_geometry
        ${HL2COMM_IDL_GEOMETRY_SRC}
        ${HL2COMM_IDL_GEOMETRY_HDRS}
        ${HL2COMM_IDL_BUILTIN_INTERFACES_SRC}
        ${HL2COMM_IDL_BUILTIN_INTERFACES_HDRS}
        )
target_link_libraries(hl2comm_idl_geometry ${DDS_LIBRARIES})
add_dependencies(hl2comm_idl_geometry hl2comm_idl_std)
target_link_libraries(hl2comm_idl_geometry hl2comm_idl_std)
target_include_directories(hl2comm_idl_geometry PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )
set_target_properties(hl2comm_idl_geometry PROPERTIES CXX_STANDARD 17)
target_compile_definitions(hl2comm_idl_geometry PRIVATE ${HL2COMM_IDL_GEOMETRY_EXPORT})
set_target_properties(hl2comm_idl_geometry PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

install(TARGETS hl2comm_idl_geometry EXPORT hl2comm_idl_geometryConfig
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(EXPORT hl2comm_idl_geometryConfig DESTINATION share/hl2comm_idl_geometry/cmake)
export(TARGETS hl2comm_idl_geometry FILE hl2comm_idl_geometryConfig.cmake)
install(FILES ${HL2COMM_IDL_GEOMETRY_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/geometry_msgs/msg)
install(FILES ${HL2COMM_IDL_IDL_GEOMETRY} DESTINATION share/pcpd_dds/geometry_msgs/msg)




#
#
#
# hl2comm_idl_pcpd
FASTDDS_GENERATE_CPP("pcpd_msgs" "msg" HL2COMM_IDL_PCPD_SRC HL2COMM_IDL_PCPD_HDRS HL2COMM_IDL_PCPD_EXPORT "hl2comm_idl_geometry" ${HL2COMM_IDL_IDL_PCPD})
FASTDDS_GENERATE_CPP("pcpd_msgs" "rpc" HL2COMM_IDL_PCPD_RPC_SRC HL2COMM_IDL_PCPD_RPC_HDRS HL2COMM_IDL_PCPD_RPC_EXPORT "hl2comm_idl_geometry" ${HL2COMM_IDL_IDL_PCPD_RPC})
add_library(hl2comm_idl_pcpd
        ${HL2COMM_IDL_PCPD_SRC}
        ${HL2COMM_IDL_PCPD_HDRS}
        ${HL2COMM_IDL_PCPD_RPC_SRC}
        ${HL2COMM_IDL_PCPD_RPC_HDRS}
        ${HL2COMM_IDL_BUILTIN_INTERFACES_SRC}
        ${HL2COMM_IDL_BUILTIN_INTERFACES_HDRS}
        )
target_link_libraries(hl2comm_idl_pcpd ${DDS_LIBRARIES})
add_dependencies(hl2comm_idl_pcpd hl2comm_idl_geometry hl2comm_idl_std)
target_link_libraries(hl2comm_idl_pcpd hl2comm_idl_geometry hl2comm_idl_std)
# probably std at some point
target_include_directories(hl2comm_idl_pcpd PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )
set_target_properties(hl2comm_idl_pcpd PROPERTIES CXX_STANDARD 17)
target_compile_definitions(hl2comm_idl_pcpd PRIVATE ${HL2COMM_IDL_PCPD_EXPORT})
set_target_properties(hl2comm_idl_pcpd PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

install(TARGETS hl2comm_idl_pcpd EXPORT hl2comm_idl_pcpdConfig
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(EXPORT hl2comm_idl_pcpdConfig DESTINATION share/hl2comm_idl_pcpd/cmake)
export(TARGETS hl2comm_idl_pcpd FILE hl2comm_idl_pcpdConfig.cmake)
install(FILES ${HL2COMM_IDL_PCPD_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pcpd_msgs/msg)
install(FILES ${HL2COMM_IDL_PCPD_RPC_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pcpd_msgs/rpc)
install(FILES ${HL2COMM_IDL_IDL_PCPD} DESTINATION share/pcpd_dds/pcpd_msgs/msg)
install(FILES ${HL2COMM_IDL_IDL_PCPD_RPC} DESTINATION share/pcpd_dds/pcpd_msgs/rpc)




#
#
#
# hl2comm_idl_rosgraph
FASTDDS_GENERATE_CPP("rosgraph_msgs" "msg" HL2COMM_IDL_ROSGRAPH_SRC HL2COMM_IDL_ROSGRAPH_HDRS HL2COMM_IDL_ROSGRAPH_EXPORT "hl2comm_idl_pcpd" ${HL2COMM_IDL_IDL_ROSGRAPH})
add_library(hl2comm_idl_rosgraph
        ${HL2COMM_IDL_ROSGRAPH_SRC}
        ${HL2COMM_IDL_ROSGRAPH_HDRS}
        ${HL2COMM_IDL_BUILTIN_INTERFACES_SRC}
        ${HL2COMM_IDL_BUILTIN_INTERFACES_HDRS}
        )
target_link_libraries(hl2comm_idl_rosgraph ${DDS_LIBRARIES})
target_include_directories(hl2comm_idl_rosgraph PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )
set_target_properties(hl2comm_idl_rosgraph PROPERTIES CXX_STANDARD 17)
target_compile_definitions(hl2comm_idl_rosgraph PRIVATE ${HL2COMM_IDL_ROSGRAPH_EXPORT})
set_target_properties(hl2comm_idl_rosgraph PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

install(TARGETS hl2comm_idl_rosgraph EXPORT hl2comm_idl_rosgraphConfig
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(EXPORT hl2comm_idl_rosgraphConfig DESTINATION share/hl2comm_idl_rosgraph/cmake)
export(TARGETS hl2comm_idl_rosgraph FILE hl2comm_idl_rosgraphConfig.cmake)
install(FILES ${HL2COMM_IDL_ROSGRAPH_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rosgraph_msgs/msg)
install(FILES ${HL2COMM_IDL_IDL_ROSGRAPH} DESTINATION share/pcpd_dds/rosgraph_msgs/msg)




#
#
#
# hl2comm_idl_sensor
FASTDDS_GENERATE_CPP("sensor_msgs" "msg" HL2COMM_IDL_SENSOR_SRC HL2COMM_IDL_SENSOR_HDRS HL2COMM_IDL_SENSOR_EXPORT "hl2comm_idl_rosgraph" ${HL2COMM_IDL_IDL_SENSOR})
add_library(hl2comm_idl_sensor
        ${HL2COMM_IDL_SENSOR_SRC}
        ${HL2COMM_IDL_SENSOR_HDRS}
        ${HL2COMM_IDL_BUILTIN_INTERFACES_SRC}
        ${HL2COMM_IDL_BUILTIN_INTERFACES_HDRS}
        )
target_link_libraries(hl2comm_idl_sensor ${DDS_LIBRARIES})
add_dependencies(hl2comm_idl_sensor hl2comm_idl_std)
add_dependencies(hl2comm_idl_sensor hl2comm_idl_geometry)
target_link_libraries(hl2comm_idl_sensor hl2comm_idl_std)
target_link_libraries(hl2comm_idl_sensor hl2comm_idl_geometry)
target_include_directories(hl2comm_idl_sensor PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )
set_target_properties(hl2comm_idl_sensor PROPERTIES CXX_STANDARD 17)
target_compile_definitions(hl2comm_idl_sensor PRIVATE ${HL2COMM_IDL_SENSOR_EXPORT})
set_target_properties(hl2comm_idl_sensor PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

install(TARGETS hl2comm_idl_sensor EXPORT hl2comm_idl_sensorConfig
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(EXPORT hl2comm_idl_sensorConfig DESTINATION share/hl2comm_idl_sensor/cmake)
export(TARGETS hl2comm_idl_sensor FILE hl2comm_idl_sensorConfig.cmake)
install(FILES ${HL2COMM_IDL_SENSOR_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sensor_msgs/msg)
install(FILES ${HL2COMM_IDL_IDL_SENSOR} DESTINATION share/pcpd_dds/sensor_msgs/msg)




#
#
#
# hl2comm_idl_shape
FASTDDS_GENERATE_CPP("shape_msgs" "msg" HL2COMM_IDL_SHAPE_SRC HL2COMM_IDL_SHAPE_HDRS HL2COMM_IDL_SHAPE_EXPORT "hl2comm_idl_sensor" ${HL2COMM_IDL_IDL_SHAPE})
add_library(hl2comm_idl_shape
        ${HL2COMM_IDL_SHAPE_SRC}
        ${HL2COMM_IDL_SHAPE_HDRS}
        ${HL2COMM_IDL_BUILTIN_INTERFACES_SRC}
        ${HL2COMM_IDL_BUILTIN_INTERFACES_HDRS}
        )
target_link_libraries(hl2comm_idl_shape ${DDS_LIBRARIES})
add_dependencies(hl2comm_idl_shape hl2comm_idl_std)
add_dependencies(hl2comm_idl_shape hl2comm_idl_geometry)
target_link_libraries(hl2comm_idl_shape hl2comm_idl_std)
target_link_libraries(hl2comm_idl_shape hl2comm_idl_geometry)
target_include_directories(hl2comm_idl_shape PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )
set_target_properties(hl2comm_idl_shape PROPERTIES CXX_STANDARD 17)
target_compile_definitions(hl2comm_idl_shape PRIVATE ${HL2COMM_IDL_SHAPE_EXPORT})
set_target_properties(hl2comm_idl_shape PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

install(TARGETS hl2comm_idl_shape EXPORT hl2comm_idl_shapeConfig
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(EXPORT hl2comm_idl_shapeConfig DESTINATION share/hl2comm_idl_shape/cmake)
export(TARGETS hl2comm_idl_shape FILE hl2comm_idl_shapeConfig.cmake)
install(FILES ${HL2COMM_IDL_SHAPE_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/shape_msgs/msg)
install(FILES ${HL2COMM_IDL_IDL_SHAPE} DESTINATION share/pcpd_dds/shape_msgs/msg)






#
#
#
# hl2comm_idl_statistics
FASTDDS_GENERATE_CPP("statistics_msgs" "msg" HL2COMM_IDL_STATISTICS_SRC HL2COMM_IDL_STATISTICS_HDRS HL2COMM_IDL_STATISTICS_EXPORT "hl2comm_idl_shape" ${HL2COMM_IDL_IDL_STATISTICS})
add_library(hl2comm_idl_statistics
        ${HL2COMM_IDL_STATISTICS_SRC}
        ${HL2COMM_IDL_STATISTICS_HDRS}
        ${HL2COMM_IDL_BUILTIN_INTERFACES_SRC}
        ${HL2COMM_IDL_BUILTIN_INTERFACES_HDRS}
        )
target_link_libraries(hl2comm_idl_statistics ${DDS_LIBRARIES})
target_include_directories(hl2comm_idl_statistics PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )
set_target_properties(hl2comm_idl_statistics PROPERTIES CXX_STANDARD 17)
target_compile_definitions(hl2comm_idl_statistics PRIVATE ${HL2COMM_IDL_STATISTICS_EXPORT})
set_target_properties(hl2comm_idl_statistics PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

install(TARGETS hl2comm_idl_statistics EXPORT hl2comm_idl_statisticsConfig
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(EXPORT hl2comm_idl_statisticsConfig DESTINATION share/hl2comm_idl_statistics/cmake)
export(TARGETS hl2comm_idl_statistics FILE hl2comm_idl_statisticsConfig.cmake)
install(FILES ${HL2COMM_IDL_STATISTICS_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/statistics_msgs/msg)
install(FILES ${HL2COMM_IDL_IDL_STATISTICS} DESTINATION share/pcpd_dds/statistics_msgs/msg)





#
#
#
# hl2comm_idl_tf2
FASTDDS_GENERATE_CPP("tf2_msgs" "msg" HL2COMM_IDL_TF2_SRC HL2COMM_IDL_TF2_HDRS HL2COMM_IDL_TF2_EXPORT "hl2comm_idl_statistics" ${HL2COMM_IDL_IDL_TF2})
add_library(hl2comm_idl_tf2
        ${HL2COMM_IDL_TF2_SRC}
        ${HL2COMM_IDL_TF2_HDRS}
        ${HL2COMM_IDL_BUILTIN_INTERFACES_SRC}
        ${HL2COMM_IDL_BUILTIN_INTERFACES_HDRS}
        )
target_link_libraries(hl2comm_idl_tf2 ${DDS_LIBRARIES})
add_dependencies(hl2comm_idl_tf2 hl2comm_idl_std)
add_dependencies(hl2comm_idl_tf2 hl2comm_idl_geometry)
target_link_libraries(hl2comm_idl_tf2 hl2comm_idl_std)
target_link_libraries(hl2comm_idl_tf2 hl2comm_idl_geometry)
target_include_directories(hl2comm_idl_tf2 PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )
set_target_properties(hl2comm_idl_tf2 PROPERTIES CXX_STANDARD 17)
target_compile_definitions(hl2comm_idl_tf2 PRIVATE ${HL2COMM_IDL_TF2_EXPORT})
set_target_properties(hl2comm_idl_tf2 PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

install(TARGETS hl2comm_idl_tf2 EXPORT hl2comm_idl_tf2Config
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(EXPORT hl2comm_idl_tf2Config DESTINATION share/hl2comm_idl_tf2/cmake)
export(TARGETS hl2comm_idl_tf2 FILE hl2comm_idl_tf2Config.cmake)
install(FILES ${HL2COMM_IDL_TF2_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tf2_msgs/msg)
install(FILES ${HL2COMM_IDL_IDL_TF2} DESTINATION share/pcpd_dds/tf2_msgs/msg)



#
#
#
# hl2comm_idl_unique_identifier
FASTDDS_GENERATE_CPP("unique_identifier_msgs" "msg" HL2COMM_IDL_UNIQUE_IDENTIFIER_SRC HL2COMM_IDL_UNIQUE_IDENTIFIER_HDRS HL2COMM_IDL_UNIQUE_IDENTIFIER_EXPORT "hl2comm_idl_tf2" ${HL2COMM_IDL_IDL_UNIQUE_IDENTIFIER})
add_library(hl2comm_idl_unique_identifier
        ${HL2COMM_IDL_UNIQUE_IDENTIFIER_SRC}
        ${HL2COMM_IDL_UNIQUE_IDENTIFIER_HDRS}
        ${HL2COMM_IDL_BUILTIN_INTERFACES_SRC}
        ${HL2COMM_IDL_BUILTIN_INTERFACES_HDRS}
        )
target_link_libraries(hl2comm_idl_unique_identifier ${DDS_LIBRARIES})
target_include_directories(hl2comm_idl_unique_identifier PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )
set_target_properties(hl2comm_idl_unique_identifier PROPERTIES CXX_STANDARD 17)
target_compile_definitions(hl2comm_idl_unique_identifier PRIVATE ${HL2COMM_IDL_UNIQUE_IDENTIFIER_EXPORT})
set_target_properties(hl2comm_idl_unique_identifier PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

install(TARGETS hl2comm_idl_unique_identifier EXPORT hl2comm_idl_unique_identifierConfig
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(EXPORT hl2comm_idl_unique_identifierConfig DESTINATION share/hl2comm_idl_unique_identifier/cmake)
export(TARGETS hl2comm_idl_unique_identifier FILE hl2comm_idl_unique_identifierConfig.cmake)
install(FILES ${HL2COMM_IDL_UNIQUE_IDENTIFIER_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/unique_identifier_msgs/msg)
install(FILES ${HL2COMM_IDL_IDL_UNIQUE_IDENTIFIER} DESTINATION share/pcpd_dds/unique_identifier_msgs/msg)


# generated cpp/h files need one import from fastrtps, which we provide to avoid pulling the dependency, if it should be build without
if (NOT WITH_DDS)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/fastrtps/include/fastrtps/utils/fixed_size_string.hpp DESTINATION include/fastrtps/utils)
endif()