cmake_minimum_required(VERSION 3.28)

project(kompose LANGUAGES C CXX VERSION 0.0.0)

set(PROJECT_NAME "Kompose")
set(PROJECT_TITLE "Kotlin build tool")

string(TIMESTAMP BUILD_DATE "%d/%m/%Y")
set(PROJECT_BUILD_DATE "${BUILD_DATE}")

add_subdirectory(deps/toolkit)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SRC "src/*.cxx")
add_executable(kompose ${SRC})

target_include_directories(kompose PRIVATE "include")

target_link_libraries(kompose PRIVATE
        toolkit::core
        toolkit::args
        toolkit::json
        toolkit::toml
)
