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

    add_subdirectory(${CMAKE_SOURCE_DIR}/3rd_party/dawn "${CMAKE_CURRENT_BINARY_DIR}/dawn" EXCLUDE_FROM_ALL)

    set(LIBRARIES webgpu_dawn webgpu_cpp webgpu_glfw glfw)
endif()

add_library(webgpu INTERFACE)
target_link_libraries(webgpu INTERFACE ${LIBRARIES})
