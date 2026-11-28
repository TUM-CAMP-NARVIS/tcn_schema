cmake_minimum_required(VERSION 3.19)  # string(JSON) for the package table
project(tcn_schema VERSION 0.11.0 LANGUAGES CXX)


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

include(GNUInstallDirs)
# The IDL tools (tools/) and the generator post-processing need Python 3 (standard library only).
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FastDDSGeneratorMacros.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TcnPackage.cmake)

find_file(FASTDDSGEN_JAR
        "fastddsgen.jar"
        PATHS "${FASTDDS_GEN_JAR_PATH}" REQUIRED)

if (BUILD_SHARED_LIBS)
  add_definitions("-DEPROSIMA_USER_DLL_EXPORT")
endif()


# The test options live in the subdirectories; enabling testing here makes `ctest` work from the build root.
if(TCN_SCHEMA_BUILD_MSG_TESTS OR TCN_SCHEMA_BUILD_SQE_TESTS)
  enable_testing()
endif()

# ROS 2 tiers to build (decisions §5 R-D2); the others are generated and committed, and built on request.
set(TCN_SCHEMA_ROS2_TIERS "base;core" CACHE STRING
    "ROS 2 interface tiers to build: base;core (default), manipulation, navigation, perception")
tcn_validate_ros2_tiers()
message(STATUS "tcn_schema: ROS 2 tiers: ${TCN_SCHEMA_ROS2_TIERS}")

# Every IDL package, in the build order of the package table (cmake/tcn_packages.json).
tcn_add_packages_from_table()

install(FILES "${PROJECT_BINARY_DIR}/share/packages.json" DESTINATION share/tcn_schema)
# ROS names, DDS names and RIHS01 hashes of every ROS 2 type (decisions §5 R3), for every tier.
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/dds/ros2/ros2_types.json" DESTINATION share/tcn_schema)
# The 0.11 type moves (old -> new fully qualified names and headers), for consumers updating their code and mirrors.
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/dds/tcn/type_map_0_11.json" DESTINATION share/tcn_schema)
# The plan 5 (#35) moves, deletions and reshaped types. Apply type_map_0_11.json first, then this one: it supersedes
# the plan 3 entries for SRNodeScope and SISTargetKind (moved again) and for types deleted since.
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/dds/tcn/type_map_0_11_sqe.json" DESTINATION share/tcn_schema)
# The plan 6 (#35/#36/#37/#38) stream, descriptor, camera, parameter and conversation moves. Apply the two maps above
# first, then this one.
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/dds/tcn/type_map_0_11_streams.json" DESTINATION share/tcn_schema)
# The joint-set table's generated arrays (decisions §7 HumanPose; source tcnart_msgs/msg/joint_sets.yaml): the
# header-only C++ arrays, and the C# arrays and JSON for the other languages.
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/joint_sets/include/tcn/joint_sets.h" DESTINATION include/tcn)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/joint_sets/joint_sets.json"
              "${CMAKE_CURRENT_SOURCE_DIR}/src/joint_sets/cs/JointSets.cs" DESTINATION share/tcn_schema)

# The SQE contract core. Header-only and dependency-free, so it adds nothing to
# a link line; it is added last because its INTERFACE target names
# tcn_schema_tcnart_msgs. Not behind an option -- a component with no libs
# costs a consumer nothing but a header copy, and an option here would only
# create a configuration in which one consumer has it and the other does not.
add_subdirectory(src/sqe)


# Machine-readable tables of every IDL declaration (decisions O7): share/tcn_schema/{types,enums}.json.
file(GLOB_RECURSE TCN_IDL_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/dds/*.idl")
file(GLOB_RECURSE TCN_IDL_TOOLS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/idl/*.py")
# CMake's VERSION cannot carry a pre-release suffix, so the label (conanfile.py's version, written into the tables'
# schema_version) is kept here. At 0.11.0 itself it becomes "0.11.0"; conanfile.py changes with it.
set(TCN_SCHEMA_VERSION_LABEL "0.11.0-rc1")
set(TCN_TABLES_DIR "${PROJECT_BINARY_DIR}/share")
add_custom_command(
        OUTPUT "${TCN_TABLES_DIR}/types.json" "${TCN_TABLES_DIR}/enums.json"
        COMMAND "${Python3_EXECUTABLE}" -I -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/idl_cli.py" tables
                --dds-root "${CMAKE_CURRENT_SOURCE_DIR}/src/dds" --version "${TCN_SCHEMA_VERSION_LABEL}" --out "${TCN_TABLES_DIR}"
        DEPENDS ${TCN_IDL_FILES} ${TCN_IDL_TOOLS} "${CMAKE_CURRENT_SOURCE_DIR}/tools/idl_cli.py"
        COMMENT "Generating types.json and enums.json"
        VERBATIM)
add_custom_target(tcn_schema_tables ALL DEPENDS "${TCN_TABLES_DIR}/types.json" "${TCN_TABLES_DIR}/enums.json")
install(FILES "${TCN_TABLES_DIR}/types.json" "${TCN_TABLES_DIR}/enums.json" DESTINATION share/tcn_schema)

option(TCN_SCHEMA_BUILD_MSG_TESTS "Build the message tests: round trips of every type, golden bytes, IDL lint" OFF)
if(TCN_SCHEMA_BUILD_MSG_TESTS)
  add_subdirectory(test)
  # The IDL style rules (docs/idl-style.md) against tools/idl/lint_baseline.txt, and the Python tools' own tests.
  add_test(NAME idl_lint COMMAND "${Python3_EXECUTABLE}" -I -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/idl_cli.py" lint
           --dds-root "${CMAKE_CURRENT_SOURCE_DIR}/src/dds" --baseline "${CMAKE_CURRENT_SOURCE_DIR}/tools/idl/lint_baseline.txt")
  add_test(NAME idl_tools COMMAND "${Python3_EXECUTABLE}" -B -m unittest discover -s tools -p "test_*.py" -t .
           WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  # `cmake --install` (what conan's package() runs) of this build tree into a scratch prefix must succeed: every
  # header the install lists has to exist (enum-only IDL files once listed CdrAux headers fastddsgen never writes).
  add_test(NAME install_smoke COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --prefix "${CMAKE_BINARY_DIR}/install_smoke")
  # ... and put the files consumers look for in place (sha256sum fails on a missing file).
  set(_smoke "${CMAKE_BINARY_DIR}/install_smoke")
  add_test(NAME install_smoke_files COMMAND "${CMAKE_COMMAND}" -E sha256sum
           "${_smoke}/include/tcn/joint_sets.h" "${_smoke}/share/tcn_schema/joint_sets.json"
           "${_smoke}/share/tcn_schema/JointSets.cs" "${_smoke}/share/tcn_schema/dds/tcnart_msgs/msg/JointSets.md"
           "${_smoke}/share/tcn_schema/dds/tcnart_msgs/msg/JointSet.idl" "${_smoke}/include/tcnart_msgs/msg/JointSet.h")
  set_tests_properties(install_smoke PROPERTIES FIXTURES_SETUP install_smoke)
  set_tests_properties(install_smoke_files PROPERTIES FIXTURES_REQUIRED install_smoke)
endif()

# generated cpp/h files need one import from fastrtps, which we provide to avoid pulling the dependency, if it should be build without
if (NOT WITH_DDS)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/fastrtps/include/fastrtps/utils/fixed_size_string.hpp DESTINATION include/fastrtps/utils)
endif()
