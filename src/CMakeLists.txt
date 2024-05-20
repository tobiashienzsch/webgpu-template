project(tobi)

find_package(fmt CONFIG REQUIRED)
find_path(MINIAUDIO_INCLUDE_DIRS "miniaudio.h")

add_library(tobi)
target_include_directories(tobi PUBLIC ${PROJECT_SOURCE_DIR} ${MINIAUDIO_INCLUDE_DIRS})
target_link_libraries(tobi PUBLIC fmt::fmt webgpu imgui)
target_sources(tobi
    PRIVATE
        tobi/AudioDevice.cpp
        tobi/GPU.cpp
        tobi/Window.cpp
)
