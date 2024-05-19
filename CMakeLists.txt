cmake_minimum_required(VERSION 3.24)
project(webgpu-template C CXX)

find_program(CCACHE ccache)
if(CCACHE)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_VISIBILITY_PRESET hidden)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

add_subdirectory(3rd_party/clap EXCLUDE_FROM_ALL)
add_subdirectory(3rd_party/fmt EXCLUDE_FROM_ALL)

add_subdirectory(lib/imgui EXCLUDE_FROM_ALL)
add_subdirectory(lib/miniaudio EXCLUDE_FROM_ALL)
add_subdirectory(lib/webgpu EXCLUDE_FROM_ALL)

add_subdirectory(src)

add_subdirectory(example/compute)
add_subdirectory(example/render)
