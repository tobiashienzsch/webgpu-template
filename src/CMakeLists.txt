project(tobi)

add_library(tobi)
target_include_directories(tobi PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(tobi PUBLIC webgpu miniaudio imgui)
target_sources(tobi
    PRIVATE
        tobi/AudioDevice.cpp
        tobi/GPU.cpp
        tobi/Window.cpp
)
