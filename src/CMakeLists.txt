project(tobi)

add_library(tobi)
target_include_directories(tobi PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(tobi PUBLIC ${LIBRARIES} miniaudio imgui)
target_sources(tobi
    PRIVATE
        tobi/AudioDevice.cpp
        tobi/Window.cpp
)
