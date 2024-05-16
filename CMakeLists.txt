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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if(EMSCRIPTEN)
    set(LIBRARIES glfw)
    add_compile_options(-sDISABLE_EXCEPTION_CATCHING=1 -DIMGUI_DISABLE_FILE_FUNCTIONS=1)
else()
    option(DAWN_FETCH_DEPENDENCIES "Use fetch_dawn_dependencies.py as an alternative to using depot_tools" ON)

    option(DAWN_BUILD_SAMPLES "Enables building Dawn's samples" OFF)
    option(DAWN_ENABLE_DESKTOP_GL "Enable OpenGL" OFF)
    option(DAWN_ENABLE_OPENGLES "Enable OpenGLES" OFF)
    option(TINT_BUILD_CMD_TOOLS "Build the Tint command line tools" OFF)
    option(TINT_BUILD_DOCS "Build documentation" OFF)
    option(TINT_BUILD_TESTS "Build tests" OFF)

    if (NOT APPLE)
        option(TINT_BUILD_MSL_WRITER "Build the MSL output writer" OFF)
    endif()

    if(WIN32)
        option(TINT_BUILD_SPV_READER "Build the SPIR-V input reader" OFF)
        option(TINT_BUILD_WGSL_READER "Build the WGSL input reader" ON)
        option(TINT_BUILD_GLSL_WRITER "Build the GLSL output writer" OFF)
        option(TINT_BUILD_GLSL_VALIDATOR "Build the GLSL output validator" OFF)
        option(TINT_BUILD_SPV_WRITER "Build the SPIR-V output writer" OFF)
        option(TINT_BUILD_WGSL_WRITER "Build the WGSL output writer" ON)
    endif()

    add_subdirectory(3rd_party/dawn "${CMAKE_CURRENT_BINARY_DIR}/dawn" EXCLUDE_FROM_ALL)

    set(LIBRARIES webgpu_dawn webgpu_cpp webgpu_glfw glfw)
endif()

add_subdirectory(3rd_party/clap EXCLUDE_FROM_ALL)
add_subdirectory(lib/miniaudio EXCLUDE_FROM_ALL)

add_executable(example
    src/main.cpp

    src/AudioDevice.cpp

    3rd_party/imgui/backends/imgui_impl_glfw.cpp
    3rd_party/imgui/backends/imgui_impl_wgpu.cpp

    3rd_party/imgui/imgui.cpp
    3rd_party/imgui/imgui_draw.cpp
    3rd_party/imgui/imgui_demo.cpp
    3rd_party/imgui/imgui_tables.cpp
    3rd_party/imgui/imgui_widgets.cpp
)

target_include_directories(example
    PRIVATE
        3rd_party/imgui
        3rd_party/imgui/backends
)

target_link_libraries(example PUBLIC ${LIBRARIES} clap miniaudio)

if(EMSCRIPTEN)
    target_link_options(example
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

    set_target_properties(example PROPERTIES OUTPUT_NAME "index")

    add_custom_command(TARGET example POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_LIST_DIR}/web/index.html" $<TARGET_FILE_DIR:example>
    )
endif()
