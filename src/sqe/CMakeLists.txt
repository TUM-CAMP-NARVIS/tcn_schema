# The SQE contract core: header-only, dependency-free, C++17.
#
# See docs/2026-08-31-sqe-contract-library-design.md §5.
#
# Rules this file keeps, each because violating it breaks a real target:
#   * `target_compile_features(... cxx_std_17)`, never `set(CMAKE_CXX_STANDARD)`
#     -- a consumer that has already chosen 17 or 20 must not be overridden;
#   * no `if(WIN32)`, `if(ANDROID)`, `if(UNIX)` -- there is nothing
#     platform-specific here, and the absence of a branch is the guarantee;
#   * no warning flags on the INTERFACE target -- each consumer sets its own
#     policy, and an INTERFACE target must not push flags into it;
#   * no `find_package`, no `FetchContent`, no network at configure time.

# Usable standalone, so the core and its tests can be configured and built on a
# machine with no fastddsgen toolchain and no Java. That is not a convenience:
# it is what makes the contract core testable at all today, given that
# `include/` is generated output and is gitignored.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.18)
  project(tcn_schema_sqe VERSION 0.3.0 LANGUAGES CXX)
endif()

include(GNUInstallDirs)

set(TCN_SQE_CONTRACT_TARGET "tcn_schema_sqe_contract")

add_library(${TCN_SQE_CONTRACT_TARGET} INTERFACE)

target_include_directories(${TCN_SQE_CONTRACT_TARGET} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_compile_features(${TCN_SQE_CONTRACT_TARGET} INTERFACE cxx_std_17)

# The core includes no generated header -- it forward-declares the wire types
# it registers -- but a consumer that passes a generated message to it needs
# the message library, so the component depends on it. Guarded so the core
# still configures on its own.
if(TARGET tcn_schema_tcnart_msgs)
  target_link_libraries(${TCN_SQE_CONTRACT_TARGET} INTERFACE tcn_schema_tcnart_msgs)
endif()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/tcn
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS ${TCN_SQE_CONTRACT_TARGET} EXPORT ${TCN_SQE_CONTRACT_TARGET}Config)
install(EXPORT ${TCN_SQE_CONTRACT_TARGET}Config DESTINATION share/${TCN_SQE_CONTRACT_TARGET}/cmake)
export(TARGETS ${TCN_SQE_CONTRACT_TARGET} FILE ${TCN_SQE_CONTRACT_TARGET}Config.cmake)

# Off by default, so no consumer's build can be broken by a test target it did
# not ask for. `tcn_schema` compiles no C++ of its own in CI today; this is the
# first target that does.
# ---------------------------------------------------------------------------
# sqe_session: the protocol, over an abstract transport
# ---------------------------------------------------------------------------
#
# A second INTERFACE target rather than more headers in the first one, because
# the two answer different questions for a consumer. `sqe_contract` is what you
# need to speak the wire correctly; `sqe_session` is what you need to run the
# protocol, and it obliges you to implement `tcn::sqe::Transport`. A consumer
# that only decodes messages should not inherit that obligation.
#
# It links no Zenoh and names no zenoh type, so it is not behind an option
# either: there is no configuration in which having it costs anything.

set(TCN_SQE_SESSION_TARGET "tcn_schema_sqe_session")

add_library(${TCN_SQE_SESSION_TARGET} INTERFACE)

target_link_libraries(${TCN_SQE_SESSION_TARGET} INTERFACE ${TCN_SQE_CONTRACT_TARGET})
target_compile_features(${TCN_SQE_SESSION_TARGET} INTERFACE cxx_std_17)

install(TARGETS ${TCN_SQE_SESSION_TARGET} EXPORT ${TCN_SQE_SESSION_TARGET}Config)
install(EXPORT ${TCN_SQE_SESSION_TARGET}Config DESTINATION share/${TCN_SQE_SESSION_TARGET}/cmake)
export(TARGETS ${TCN_SQE_SESSION_TARGET} FILE ${TCN_SQE_SESSION_TARGET}Config.cmake)

# Both components' headers live under the one include/ tree already installed
# above, so there is no second install(DIRECTORY) here: `sqe_session` adds a
# target and an obligation, not a second copy of the headers.

# No reference Zenoh adapter ships from this repository, and that is a decision
# rather than an omission. See docs/2026-08-31-sqe-contract-library-design.md
# and the note at the top of tcn/sqe/transport.h: the two consumers move to new
# zenoh-cpp versions on their own schedules, and shipping an adapter here would
# put zenoh-version-dependent code in `tcn_schema` -- the one thing this split
# exists to avoid. The adapter is five functions; it belongs in the repository
# that owns the zenoh pin. src/sqe/test/fake_transport.h is the worked example
# of its shape.

option(TCN_SCHEMA_BUILD_SQE_TESTS "Build the sqe_contract and sqe_session unit tests" OFF)

if(TCN_SCHEMA_BUILD_SQE_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
