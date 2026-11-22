cmake_minimum_required(VERSION 3.28)

if (NOT DEFINED KOMPOSE_PROJECT_VERSION OR KOMPOSE_PROJECT_VERSION STREQUAL "")
    set(KOMPOSE_PROJECT_VERSION "0.0.0")
endif ()

string(REGEX MATCH "^[0-9]+(\\.[0-9]+)*" KOMPOSE_CMAKE_VERSION "${KOMPOSE_PROJECT_VERSION}")

if (KOMPOSE_CMAKE_VERSION STREQUAL "")
    message(FATAL_ERROR "invalid project version: ${KOMPOSE_PROJECT_VERSION}")
endif ()

project(kompose
        LANGUAGES C CXX
        VERSION "${KOMPOSE_CMAKE_VERSION}"
)

set(PROJECT_NAME "Kompose")
set(PROJECT_TITLE "Kotlin build tool")

string(TIMESTAMP BUILD_DATE "%d/%m/%Y")
set(PROJECT_BUILD_DATE "${BUILD_DATE}")

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/definitions/version.hxx.in"
        "${CMAKE_CURRENT_BINARY_DIR}/version.hxx"
        @ONLY
)

set(TOOLKIT_HTTP_NO_TLS OFF)

add_subdirectory(deps/toolkit)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)

file(GLOB_RECURSE SRC "src/*.cxx")
add_executable(kompose ${SRC})

target_include_directories(kompose PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "include"
)

target_link_libraries(kompose PRIVATE
        toolkit::core
        toolkit::args
        toolkit::http
        toolkit::json
        toolkit::toml
        toolkit::xml
        OpenSSL::SSL
)

install(TARGETS kompose)
