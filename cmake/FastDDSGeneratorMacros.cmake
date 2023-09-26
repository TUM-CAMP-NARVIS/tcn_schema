# FASTDDS_GENERATE_CPP ===========================================================
#
# Configuration variables (optional):
#   FASTDDSGEN_OUTPUT_DIR
#       Directory to place compiled schema sources (default: CMAKE_CURRENT_BINARY_DIR).
#   FASTDDSGEN_IMPORT_DIRS
#       List of additional include directories for the schema compiler.
#       (FASTDDSGEN_SRC_PREFIX and FASTDDS_INCLUDE_DIRECTORY are always included.)
#   FASTDDSGEN_SRC_PREFIX
#       Schema file source prefix (default: CMAKE_CURRENT_SOURCE_DIR).
#   FASTDDSGEN_FLAGS
#       Additional flags to pass to the schema compiler.
#
# TODO: convert to cmake_parse_arguments

find_package(Java COMPONENTS Runtime REQUIRED)

function(FASTDDS_GENERATE_CPP MODPATH MODTYP SOURCES HEADERS DLLEXPORT GEN_DEPENDENCY)
    if(NOT ARGN)
        message(SEND_ERROR "FASTDDS_GENERATE_CPP() called without any source files.")
    endif()
    set(tool_depends ${EMPTY_STRING})
    if(NOT FASTDDSGEN_JAR)
        message(SEND_ERROR "Could not locate fastddsgen executable (FASTDDSGEN_JAR).")
    endif()
    if(NOT FASTDDS_INCLUDE_DIRECTORY)
        message(SEND_ERROR "Missing IDL include directory (FASTDDS_INCLUDE_DIRECTORY).")
    endif()

    if(DEFINED FASTDDSGEN_OUTPUT_DIR)
        # Prepend a ':' to get the format for the '-o' flag right
        set(output_dir "${FASTDDSGEN_OUTPUT_DIR}")
    else()
        set(output_dir ".")
    endif()

    if(NOT DEFINED FASTDDSGEN_SRC_PREFIX)
        set(FASTDDSGEN_SRC_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
    get_filename_component(FASTDDSGEN_SRC_PREFIX "${FASTDDSGEN_SRC_PREFIX}" ABSOLUTE)

    # Figure out where the output files will go
    if (NOT DEFINED FASTDDSGEN_OUTPUT_DIR)
        set(FASTDDSGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${MODPATH}/")
    endif()

    set(${SOURCES})
    set(${HEADERS})
    set(${DLLEXPORT})
    set(SCHEMAS)

    set(MODOUTDIR "${FASTDDSGEN_OUTPUT_DIR}/${MODPATH}/${MODTYP}")

    set(CMDARGS -jar ${FASTDDSGEN_JAR})
    list(APPEND CMDARGS -d ${MODOUTDIR})
    list(APPEND CMDARGS -I ${FASTDDS_INCLUDE_DIRECTORY}/ros2)
    list(APPEND CMDARGS -I ${FASTDDS_INCLUDE_DIRECTORY}/custom)
    list(APPEND CMDARGS -typeros2)
    list(APPEND CMDARGS -cs)
    list(APPEND CMDARGS -replace)
    list(APPEND CMDARGS ${FASTDDSGEN_FLAGS})


    foreach(schema_file ${ARGN})
        get_filename_component(file_path "${schema_file}" ABSOLUTE)
        get_filename_component(file_dir "${file_path}" PATH)
        if(NOT EXISTS "${file_path}")
            message(FATAL_ERROR "DDS idl file '${file_path}' does not exist!")
        endif()

        # Output files are placed in FASTDDSGEN_OUTPUT_DIR, at a location as if they were
        # relative to FASTDDSGEN_SRC_PREFIX.
        string(LENGTH "${FASTDDSGEN_SRC_PREFIX}" prefix_len)
        string(SUBSTRING "${file_path}" 0 ${prefix_len} output_prefix)
        if(NOT "${FASTDDSGEN_SRC_PREFIX}" STREQUAL "${output_prefix}")
            message(SEND_ERROR "Could not determine output path for '${schema_file}' ('${file_path}') with source prefix '${FASTDDSGEN_SRC_PREFIX}' into '${FASTDDSGEN_OUTPUT_DIR}'.")
        endif()

        string(SUBSTRING "${file_path}" ${prefix_len} -1 output_path1)
        get_filename_component(output_path ${output_path1} NAME_WE)

        set(output_base "${MODOUTDIR}/${output_path}")

        set(SCHEMAS ${SCHEMAS} "${schema_file}")
        list(APPEND CMDARGS "${CMAKE_CURRENT_SOURCE_DIR}/${schema_file}")
        list(APPEND ${SOURCES} "${output_base}.cxx")
        list(APPEND ${HEADERS} "${output_base}.h")
        list(APPEND ${SOURCES} "${output_base}CdrAux.ipp")
        list(APPEND ${HEADERS} "${output_base}CdrAux.hpp")
        list(APPEND ${DLLEXPORT} "${output_path}_SOURCE")
        if (WITH_DDS)
          list(APPEND ${SOURCES} "${output_base}PubSubTypes.cxx")
          list(APPEND ${HEADERS} "${output_base}PubSubTypes.h")
        endif()
    endforeach()

    if(NOT EXISTS ${MODOUTDIR})
        file(MAKE_DIRECTORY ${MODOUTDIR})
    endif()

    #message(STATUS "Schedule custom command for ${SCHEMAS} to generate headers: ${${HEADERS}} and sources: ${${SOURCES}}")

    add_custom_command(
            OUTPUT ${${SOURCES}} ${${HEADERS}}
            COMMAND "${Java_JAVA_EXECUTABLE}"
            ARGS ${CMDARGS}
            DEPENDS ${SCHEMAS} ${tool_depends} ${GEN_DEPENDENCY}
            COMMENT "Compiling IDL Files: ${SCHEMAS}"
            VERBATIM USES_TERMINAL
    )

    set_source_files_properties(${${SOURCES}} ${${HEADERS}} PROPERTIES GENERATED TRUE)
    set(${SOURCES} ${${SOURCES}} PARENT_SCOPE)
    set(${HEADERS} ${${HEADERS}} PARENT_SCOPE)
    set(${DLLEXPORT} ${${DLLEXPORT}} PARENT_SCOPE)
endfunction()
