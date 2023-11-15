set(TCN_MODULE_NAME "sensor_msgs")

set(TCN_MODULE_IDL
        "msg/NavSatFix.idl"
        "msg/MultiEchoLaserScan.idl"
        "msg/Temperature.idl"
        "msg/LaserScan.idl"
        "msg/Image.idl"
        "msg/Illuminance.idl"
        "msg/JoyFeedback.idl"
        "msg/NavSatStatus.idl"
        "msg/PointCloud2.idl"
        "msg/BatteryState.idl"
        "msg/RegionOfInterest.idl"
        "msg/LaserEcho.idl"
        "msg/TimeReference.idl"
        "msg/RelativeHumidity.idl"
        "msg/MagneticField.idl"
        "msg/JointState.idl"
        "msg/JoyFeedbackArray.idl"
        "msg/Joy.idl"
        "msg/ChannelFloat32.idl"
        "msg/CompressedImage.idl"
        "msg/Range.idl"
        "msg/FluidPressure.idl"
        "msg/PointCloud.idl"
        "msg/PointField.idl"
        "msg/CameraInfo.idl"
        "msg/MultiDOFJointState.idl"
        "msg/Imu.idl"
        )

# Generate sources from idl and compile them into a shared/static library
set(TCN_SCHEMA_LIB_NAME "tcn_schema_${TCN_MODULE_NAME}")
set (FASTDDSGEN_OUTPUT_DIR "${PROJECT_SOURCE_DIR}/include")
set (FASTDDSGEN_SRC_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}")
set (FASTDDS_INCLUDE_DIRECTORY "${PROJECT_SOURCE_DIR}/src/dds")

FASTDDS_GENERATE_CPP("${TCN_MODULE_NAME}" "msg" TCN_MODULE_SRC TCN_MODULE_HDRS TCN_MODULE_EXPORT "" ${TCN_MODULE_IDL})
add_library(${TCN_SCHEMA_LIB_NAME}
        ${TCN_MODULE_SRC}
        ${TCN_MODULE_HDRS}
        )
target_link_libraries(${TCN_SCHEMA_LIB_NAME} tcn_schema_builtin_interfaces tcn_schema_std_msgs tcn_schema_geometry_msgs ${DDS_LIBRARIES})
target_include_directories(${TCN_SCHEMA_LIB_NAME} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        )
set_target_properties(${TCN_SCHEMA_LIB_NAME} PROPERTIES CXX_STANDARD 17)
target_compile_definitions(${TCN_SCHEMA_LIB_NAME} PRIVATE ${TCN_MODULE_EXPORT})
set_target_properties(${TCN_SCHEMA_LIB_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

if(MSVC)
  target_compile_definitions(${TCN_SCHEMA_LIB_NAME} PRIVATE $<$<CONFIG:DEBUG>:_DEBUG>)
  set_target_properties(${TCN_SCHEMA_LIB_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

install(TARGETS ${TCN_SCHEMA_LIB_NAME} EXPORT ${TCN_SCHEMA_LIB_NAME}Config
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

install(EXPORT ${TCN_SCHEMA_LIB_NAME}Config DESTINATION share/${TCN_SCHEMA_LIB_NAME}/cmake)
export(TARGETS ${TCN_SCHEMA_LIB_NAME} FILE ${TCN_SCHEMA_LIB_NAME}Config.cmake)
install(FILES ${TCN_MODULE_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${TCN_MODULE_NAME}/msg)
install(FILES ${TCN_MODULE_IDL} DESTINATION share/tcn_schema/dds/${TCN_MODULE_NAME}/msg)
