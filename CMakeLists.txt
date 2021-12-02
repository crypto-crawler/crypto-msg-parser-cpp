cmake_minimum_required(VERSION 3.16)
include(FetchContent)
include(GoogleTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

if(DEFINED ENV{VCPKG_ROOT})
  set(CMAKE_TOOLCHAIN_FILE
      "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
else()
  message("vcpkg not detected, now installing it")
  FetchContent_Declare(vcpkg GIT_REPOSITORY https://github.com/microsoft/vcpkg)
  FetchContent_GetProperties(vcpkg)
  if(NOT vcpkg_POPULATED)
    FetchContent_Populate(vcpkg)
    set(ENV{ VCPKG_ROOT} "${vcpkg_SOURCE_DIR}")
    execute_process(COMMAND "${vcpkg_SOURCE_DIR}/bootstrap-vcpkg.sh")
    set(CMAKE_TOOLCHAIN_FILE
        "${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
  endif()
endif()

project(
  "crypto_msg_parser"
  LANGUAGES CXX
  VERSION 1.7.8
  DESCRIPTION
    "A parser library to parser websocket messages from cryptocurrency exchanges."
  HOMEPAGE_URL "https://github.com/soulmachine/crypto-msg-parser-cpp")

execute_process(COMMAND "$ENV{VCPKG_ROOT}/vcpkg" install nlohmann-json)

find_package(nlohmann_json CONFIG REQUIRED)

find_program(CARGO "cargo")
if(!CARGO)
  message(
    FATAL_ERROR
      "Could not find cargo. Please install cargo(see https://rustup.rs/) as it is needed to build rust libraries."
  )
endif()

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  crypto_msg_parser_ffi
  GIT_REPOSITORY https://github.com/soulmachine/crypto-msg-parser-ffi
  GIT_TAG main)
FetchContent_GetProperties(crypto_msg_parser_ffi)
if(NOT crypto_msg_parser_ffi_POPULATED)
  FetchContent_Populate(crypto_msg_parser_ffi)
endif()
include_directories(${crypto_msg_parser_ffi_SOURCE_DIR}/include)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CARGO_CMD cargo build --release
                --target-dir=${crypto_msg_parser_ffi_BINARY_DIR})
  set(OUTPUT_DIR "${crypto_msg_parser_ffi_BINARY_DIR}/release")
else()
  set(CARGO_CMD cargo build --target-dir=${crypto_msg_parser_ffi_BINARY_DIR})
  set(OUTPUT_DIR "${crypto_msg_parser_ffi_BINARY_DIR}/debug")
endif()

add_custom_target(
  crypto_msg_parser_ffi_cargo ALL
  COMMENT "Compiling crypto_msg_parser_ffi cargo project"
  COMMAND ${CARGO_CMD}
  WORKING_DIRECTORY ${crypto_msg_parser_ffi_SOURCE_DIR})

add_library(crypto_msg_parser_ffi STATIC IMPORTED GLOBAL)
add_dependencies(crypto_msg_parser_ffi crypto_msg_parser_ffi_cargo)
set_target_properties(
  crypto_msg_parser_ffi PROPERTIES IMPORTED_LOCATION
                                   "${OUTPUT_DIR}/libcrypto_msg_parser_ffi.a")
set_target_properties(crypto_msg_parser_ffi PROPERTIES INTERFACE_LINK_LIBRARIES
                                                       "pthread;ssl;crypto;dl")
add_library(crypto_msg_parser_ffi::crypto_msg_parser_ffi ALIAS
            crypto_msg_parser_ffi)

add_library(crypto_msg_parser STATIC crypto_msg_parser.cpp)
target_link_libraries(crypto_msg_parser PRIVATE nlohmann_json::nlohmann_json
                                                crypto_msg_parser_ffi)
set_target_properties(
  PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/crypto_msg_parser.h")

enable_testing()
add_executable(crypto_msg_parser_test crypto_msg_parser_test.cpp)
target_link_libraries(crypto_msg_parser_test PRIVATE crypto_msg_parser gtest_main)
gtest_discover_tests(crypto_msg_parser_test)
