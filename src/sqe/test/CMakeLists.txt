# Unit tests for the header-only contract core.
#
# No gtest, no Conan test dependency, no network: the harness is check.h.
# `tcn_schema` has no C++ test infrastructure at all today, and the first test
# target should not also be the repository's first third-party test dependency.

set(TCN_SQE_TEST_TARGET "tcn_schema_sqe_contract_tests")

add_executable(${TCN_SQE_TEST_TARGET}
        test_main.cpp
        test_result.cpp
        test_validate.cpp
        test_client_id.cpp
        test_names.cpp
        test_status.cpp
        test_topics.cpp
        test_wire_type.cpp
        test_handle.cpp)

target_link_libraries(${TCN_SQE_TEST_TARGET} PRIVATE tcn_schema_sqe_contract)
target_include_directories(${TCN_SQE_TEST_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
  target_compile_options(${TCN_SQE_TEST_TARGET} PRIVATE /W4 /permissive-)
else()
  target_compile_options(${TCN_SQE_TEST_TARGET} PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME sqe_contract COMMAND ${TCN_SQE_TEST_TARGET})

# ---------------------------------------------------------------------------
# Negative compile tests
# ---------------------------------------------------------------------------
#
# Several of the core's guarantees are compile-time and cannot be asserted at
# run time by definition -- "a Handle cannot be constructed from a bare string"
# is a statement about what does not build. Each source below must fail to
# compile; the test drives a build of it and expects that build to fail.
#
# Excluded from ALL, so an ordinary build never tries to compile them.

set(TCN_SQE_MUST_NOT_COMPILE
        handle_from_string
        handle_from_literal
        handle_from_reply_string
        handle_default_construct
        wire_type_unregistered
        client_id_unvalidated)

foreach(case ${TCN_SQE_MUST_NOT_COMPILE})
  add_executable(sqe_must_not_compile_${case} EXCLUDE_FROM_ALL negative/${case}.cpp)
  target_link_libraries(sqe_must_not_compile_${case} PRIVATE tcn_schema_sqe_contract)
  set_target_properties(sqe_must_not_compile_${case} PROPERTIES
          EXCLUDE_FROM_DEFAULT_BUILD TRUE)

  add_test(NAME sqe_must_not_compile.${case}
          COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
                  --target sqe_must_not_compile_${case}
                  --config $<CONFIG>)
  set_tests_properties(sqe_must_not_compile.${case} PROPERTIES WILL_FAIL TRUE)
endforeach()
