cmake_minimum_required(VERSION 3.19)  # string(JSON) for the package table
project(tcn_schema VERSION 0.10.7 LANGUAGES CXX)


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

# Every IDL package, in the build order of the package table (cmake/tcn_packages.json).
tcn_add_packages_from_table()

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/tcn_packages.json" DESTINATION share/tcn_schema RENAME packages.json)

# The SQE contract core. Header-only and dependency-free, so it adds nothing to
# a link line; it is added last because its INTERFACE target names
# tcn_schema_tcnart_msgs. Not behind an option -- a component with no libs
# costs a consumer nothing but a header copy, and an option here would only
# create a configuration in which one consumer has it and the other does not.
add_subdirectory(src/sqe)


# Machine-readable tables of every IDL declaration (decisions O7): share/tcn_schema/{types,enums}.json.
find_package(Python3 COMPONENTS Interpreter REQUIRED)
file(GLOB_RECURSE TCN_IDL_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/dds/*.idl")
file(GLOB_RECURSE TCN_IDL_TOOLS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/idl/*.py")
set(TCN_TABLES_DIR "${PROJECT_BINARY_DIR}/share")
add_custom_command(
        OUTPUT "${TCN_TABLES_DIR}/types.json" "${TCN_TABLES_DIR}/enums.json"
        COMMAND "${Python3_EXECUTABLE}" -I -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/idl_cli.py" tables
                --dds-root "${CMAKE_CURRENT_SOURCE_DIR}/src/dds" --version "${PROJECT_VERSION}" --out "${TCN_TABLES_DIR}"
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
endif()

# generated cpp/h files need one import from fastrtps, which we provide to avoid pulling the dependency, if it should be build without
if (NOT WITH_DDS)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/fastrtps/include/fastrtps/utils/fixed_size_string.hpp DESTINATION include/fastrtps/utils)
endif()
