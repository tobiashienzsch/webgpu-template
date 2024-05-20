cmake_minimum_required(VERSION 3.24)
project(render)

find_package(clap CONFIG REQUIRED)

add_executable(render main.cpp)
target_link_libraries(render PRIVATE tobi clap)

if(EMSCRIPTEN)
    target_link_options(render
        PRIVATE
            "SHELL:-s USE_WEBGPU=1"
            "SHELL:-s USE_GLFW=3"
            "SHELL:-s WASM=1"
            "SHELL:-s ALLOW_MEMORY_GROWTH=1"
            "SHELL:-s NO_EXIT_RUNTIME=0"
            "SHELL:-s ASSERTIONS=1"
            "SHELL:-s DISABLE_EXCEPTION_CATCHING=1"
            "SHELL:-s NO_FILESYSTEM=1"
            "SHELL:-s ASYNCIFY=1"
    )

    set_target_properties(render PROPERTIES OUTPUT_NAME "index")

    add_custom_command(TARGET render POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/web/index.html" $<TARGET_FILE_DIR:render>
    )
endif()
