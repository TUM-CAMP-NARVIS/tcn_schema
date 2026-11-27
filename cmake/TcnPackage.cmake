# tcn_add_idl_package(): one IDL package -> one library `tcn_schema_<NAME>`.
#
#   tcn_add_idl_package(NAME <pkg>
#                       [MSG <idl>...] [RPC <idl>...] [ACTION <idl>...]
#                       [EXTRA_INSTALL <file>...])
#
# * Code is generated into ${PROJECT_BINARY_DIR}/generated/<pkg>/<msg|rpc|action>/ (never into the source tree).
# * Consumers of the build tree include ${PROJECT_BINARY_DIR}/generated; consumers of the installed package include
#   <prefix>/include. Header paths are the same in both: `<pkg>/<kind>/<File>.h`.
# * Headers install to include/<pkg>/<kind>/, IDL files to share/tcn_schema/dds/<pkg>/<kind>/.
# * EXTRA_INSTALL files (documentation shipped with the IDL) install next to the msg IDL.
# * Dependencies (the packages whose IDL this package includes) come from the package table cmake/tcn_packages.json;
#   tools/idl/tests/test_deps.py checks the table against the IDL #includes.
#
# The rest (export definitions, PIC, MSVC runtime, C++17, per-target CMake config export) is what each package's
# CMakeLists.txt did by hand up to 0.10.7.

set(TCN_GENERATED_DIR "${PROJECT_BINARY_DIR}/generated")
set(TCN_PACKAGE_TABLE "${PROJECT_SOURCE_DIR}/cmake/tcn_packages.json")
file(READ "${TCN_PACKAGE_TABLE}" TCN_PACKAGE_TABLE_JSON)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${TCN_PACKAGE_TABLE}")

# tcn_package_deps(<pkg> <out-var>): the direct dependencies of <pkg> from the package table.
function(tcn_package_deps pkg out)
  string(JSON n LENGTH "${TCN_PACKAGE_TABLE_JSON}" packages)
  math(EXPR last "${n} - 1")
  foreach(i RANGE ${last})
    string(JSON name GET "${TCN_PACKAGE_TABLE_JSON}" packages ${i} name)
    if(name STREQUAL pkg)
      set(deps)
      string(JSON nd LENGTH "${TCN_PACKAGE_TABLE_JSON}" packages ${i} deps)
      if(nd GREATER 0)
        math(EXPR lastd "${nd} - 1")
        foreach(j RANGE ${lastd})
          string(JSON d GET "${TCN_PACKAGE_TABLE_JSON}" packages ${i} deps ${j})
          list(APPEND deps ${d})
        endforeach()
      endif()
      set(${out} ${deps} PARENT_SCOPE)
      return()
    endif()
  endforeach()
  message(FATAL_ERROR "package '${pkg}' is not in ${TCN_PACKAGE_TABLE}")
endfunction()

# tcn_add_packages_from_table(): add_subdirectory() for every IDL package (family ros2 or tcn), in table order.
function(tcn_add_packages_from_table)
  string(JSON n LENGTH "${TCN_PACKAGE_TABLE_JSON}" packages)
  math(EXPR last "${n} - 1")
  foreach(i RANGE ${last})
    string(JSON family GET "${TCN_PACKAGE_TABLE_JSON}" packages ${i} family)
    if(family STREQUAL "ros2" OR family STREQUAL "tcn")
      string(JSON dir GET "${TCN_PACKAGE_TABLE_JSON}" packages ${i} dir)
      add_subdirectory("${PROJECT_SOURCE_DIR}/${dir}")
    endif()
  endforeach()
endfunction()

function(tcn_add_idl_package)
  cmake_parse_arguments(ARG "" "NAME" "MSG;RPC;ACTION;DEPS;EXTRA_INSTALL" ${ARGN})
  if(NOT ARG_NAME)
    message(FATAL_ERROR "tcn_add_idl_package: NAME is required")
  endif()

  set(lib "tcn_schema_${ARG_NAME}")
  if(ARG_DEPS)
    message(FATAL_ERROR "tcn_add_idl_package(${ARG_NAME}): DEPS come from cmake/tcn_packages.json; do not pass them")
  endif()
  tcn_package_deps(${ARG_NAME} ARG_DEPS)

  # Read by FASTDDS_GENERATE_CPP from the calling scope.
  set(FASTDDSGEN_OUTPUT_DIR "${TCN_GENERATED_DIR}")
  set(FASTDDSGEN_SRC_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}")
  set(FASTDDS_INCLUDE_DIRECTORY "${PROJECT_SOURCE_DIR}/src/dds")

  set(all_src)
  set(all_hdr)
  set(all_export)
  foreach(kind msg rpc action)
    string(TOUPPER "${kind}" KIND)
    if(ARG_${KIND})
      FASTDDS_GENERATE_CPP("${ARG_NAME}" "${kind}" src_${kind} hdr_${kind} export_${kind} "" ${ARG_${KIND}})
      list(APPEND all_src ${src_${kind}})
      list(APPEND all_hdr ${hdr_${kind}})
      list(APPEND all_export ${export_${kind}})
      install(FILES ${hdr_${kind}} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${ARG_NAME}/${kind})
      install(FILES ${ARG_${KIND}} DESTINATION share/tcn_schema/dds/${ARG_NAME}/${kind})
    endif()
  endforeach()

  add_library(${lib} ${all_src} ${all_hdr})

  set(dep_libs)
  foreach(dep ${ARG_DEPS})
    list(APPEND dep_libs tcn_schema_${dep})
  endforeach()
  target_link_libraries(${lib} ${dep_libs} ${DDS_LIBRARIES})

  # Generation of this package reads the IDL of its dependencies; make sure their code exists first, so a parallel
  # build never compiles against a header that is still being written.
  foreach(dep ${dep_libs})
    add_dependencies(${lib} ${dep})
  endforeach()

  target_include_directories(${lib} PUBLIC
          $<BUILD_INTERFACE:${TCN_GENERATED_DIR}>
          $<INSTALL_INTERFACE:include>)
  set_target_properties(${lib} PROPERTIES CXX_STANDARD 17)
  target_compile_definitions(${lib} PRIVATE ${all_export})
  set_target_properties(${lib} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
  set_property(TARGET ${lib} PROPERTY POSITION_INDEPENDENT_CODE ON)

  if(MSVC)
    target_compile_definitions(${lib} PRIVATE $<$<CONFIG:DEBUG>:_DEBUG>)
    set_target_properties(${lib} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
  endif()

  install(TARGETS ${lib} EXPORT ${lib}Config
          ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
          LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
          RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows
  install(EXPORT ${lib}Config DESTINATION share/${lib}/cmake)
  export(TARGETS ${lib} FILE ${lib}Config.cmake)

  if(ARG_EXTRA_INSTALL)
    install(FILES ${ARG_EXTRA_INSTALL} DESTINATION share/tcn_schema/dds/${ARG_NAME}/msg)
  endif()
endfunction()
