cmake_minimum_required(VERSION 3.18)
project(tcn_schema VERSION 0.1.0 LANGUAGES CXX)


option(WITH_DDS "Compile with DDS Implementation" OFF)

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
        PATHS "${FASTDDS_GEN_JAR_PATH}" REQUIRED)

if (BUILD_SHARED_LIBS)
  add_definitions("-DEPROSIMA_USER_DLL_EXPORT" "-DFASTCDR_DYN_LINK")
else()
  add_definitions("-DFASTCDR_STATIC_LINK")
endif()


add_subdirectory(src/dds/ros2/builtin_interfaces)
add_subdirectory(src/dds/ros2/std_msgs)
add_subdirectory(src/dds/ros2/geometry_msgs)
add_subdirectory(src/dds/ros2/rosgraph_msgs)
add_subdirectory(src/dds/ros2/sensor_msgs)
add_subdirectory(src/dds/ros2/statistics_msgs)
add_subdirectory(src/dds/ros2/tf2_msgs)
add_subdirectory(src/dds/ros2/unique_identifier_msgs)

add_subdirectory(src/dds/custom/tcnart_msgs)
add_subdirectory(src/dds/custom/pcpd_msgs)
add_subdirectory(src/dds/custom/device_hl2_msgs)


# generated cpp/h files need one import from fastrtps, which we provide to avoid pulling the dependency, if it should be build without
if (NOT WITH_DDS)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/fastrtps/include/fastrtps/utils/fixed_size_string.hpp DESTINATION include/fastrtps/utils)
endif()