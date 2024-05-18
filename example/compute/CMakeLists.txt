cmake_minimum_required(VERSION 3.24)
project(compute)

add_executable(compute main.cpp)
target_include_directories(compute PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(compute PRIVATE ${LIBRARIES})

if(EMSCRIPTEN)
    target_link_options(compute
        PRIVATE
            "-sUSE_WEBGPU=1"
            "-sWASM=1"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sNO_EXIT_RUNTIME=0"
            "-sASSERTIONS=1"
            "-sDISABLE_EXCEPTION_CATCHING=1"
            "-sNO_FILESYSTEM=1"
    )

    set_target_properties(compute PROPERTIES OUTPUT_NAME "index")

    add_custom_command(TARGET compute POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/web/index.html" $<TARGET_FILE_DIR:compute>
    )
endif()
