cmake_minimum_required(VERSION 3.24)
project(compute)

add_executable(compute main.cpp)
target_include_directories(compute PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(compute PRIVATE tobi)

if(EMSCRIPTEN)
    target_link_options(compute
        PRIVATE
            "SHELL:-s USE_WEBGPU=1"
            "SHELL:-s WASM=1"
            "SHELL:-s ALLOW_MEMORY_GROWTH=1"
            "SHELL:-s NO_EXIT_RUNTIME=0"
            "SHELL:-s ASSERTIONS=1"
            "SHELL:-s DISABLE_EXCEPTION_CATCHING=1"
            "SHELL:-s NO_FILESYSTEM=1"
            "SHELL:-s ASYNCIFY=1"
    )

    set_target_properties(compute PROPERTIES OUTPUT_NAME "index")

    add_custom_command(TARGET compute POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/web/index.html" $<TARGET_FILE_DIR:compute>
    )
endif()
