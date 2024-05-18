cmake_minimum_required(VERSION 3.24)
project(render)

add_executable(render main.cpp)
target_link_libraries(render PRIVATE tobi clap)

if(EMSCRIPTEN)
    target_link_options(render
        PRIVATE
            "-sUSE_WEBGPU=1"
            "-sUSE_GLFW=3"
            "-sWASM=1"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sNO_EXIT_RUNTIME=0"
            "-sASSERTIONS=1"
            "-sDISABLE_EXCEPTION_CATCHING=1"
            "-sNO_FILESYSTEM=1"
    )

    set_target_properties(render PROPERTIES OUTPUT_NAME "index")

    add_custom_command(TARGET render POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/web/index.html" $<TARGET_FILE_DIR:render>
    )
endif()
