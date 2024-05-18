add_library(imgui)
target_sources(imgui
    PRIVATE
        "${CMAKE_SOURCE_DIR}/3rd_party/imgui/backends/imgui_impl_glfw.cpp"
        "${CMAKE_SOURCE_DIR}/3rd_party/imgui/backends/imgui_impl_wgpu.cpp"

        "${CMAKE_SOURCE_DIR}/3rd_party/imgui/imgui.cpp"
        "${CMAKE_SOURCE_DIR}/3rd_party/imgui/imgui_draw.cpp"
        "${CMAKE_SOURCE_DIR}/3rd_party/imgui/imgui_demo.cpp"
        "${CMAKE_SOURCE_DIR}/3rd_party/imgui/imgui_tables.cpp"
        "${CMAKE_SOURCE_DIR}/3rd_party/imgui/imgui_widgets.cpp"
)

target_include_directories(imgui
    PUBLIC
        "${CMAKE_SOURCE_DIR}/3rd_party/imgui"
        "${CMAKE_SOURCE_DIR}/3rd_party/imgui/backends"
)

target_link_libraries(imgui PUBLIC ${LIBRARIES})
