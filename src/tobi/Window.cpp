#include "Window.hpp"

#include <tobi/GPU.hpp>

#include <fmt/format.h>
#include <fmt/os.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#else
#include <webgpu/webgpu_glfw.h>
#endif

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_wgpu.h"

#include <cassert>
#include <stdexcept>
#include <vector>

namespace tobi {

Window::Window() {
    glfwSetErrorCallback([](int error, const char* description) {
        fmt::println("GLFW Error {}: {}\n", error, description);
    });

    if (not glfwInit()) {
        throw std::runtime_error{"glfw init failed"};
    }

    // Make sure GLFW does not initialize any graphics context.
    // This needs to be done explicitly later.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto const* title = "Dear ImGui GLFW+WebGPU example";
    _window = glfwCreateWindow(_width, _height, title, nullptr, nullptr);
    if (_window == nullptr) {
        throw std::runtime_error{"window init failed"};
    }
}

Window::~Window() {
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (_window != nullptr) {
        glfwDestroyWindow(_window);
    }
    glfwTerminate();
}

auto Window::show() -> void {
    // Initialize the WebGPU environment
    if (not initWebGPU()) {
        // return EXIT_FAILURE;
    }
    createSwapChain(_width, _height);

    glfwShowWindow(_window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOther(_window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
#endif
    ImGui_ImplWGPU_InitInfo init_info;
    init_info.Device = _gpuDevice.Get();
    init_info.NumFramesInFlight = 3;
    init_info.RenderTargetFormat = static_cast<WGPUTextureFormat>(_gpuPreferredFormat);
    init_info.DepthStencilFormat = WGPUTextureFormat_Undefined;
    ImGui_ImplWGPU_Init(&init_info);

    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a
    // fopen() of the imgui.ini file. You may manually call LoadIniSettingsFromMemory() to load
    // settings from your own storage.
    io.IniFilename = nullptr;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg([](auto* p) { static_cast<Window*>(p)->loop(); }, this, 0, true);
#else
    while (not glfwWindowShouldClose(_window)) {
        loop();
    }
#endif
}

auto Window::loop() -> void {
    static bool showDemoWindow = true;
    static bool showAnotherWindow = true;
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    static float f = 0.0f;

    ImGuiIO& io = ImGui::GetIO();

    glfwPollEvents();

    // React to changes in screen size
    int width, height;
    glfwGetFramebufferSize(_window, &width, &height);
    if (width != _width or height != _height) {
        ImGui_ImplWGPU_InvalidateDeviceObjects();
        createSwapChain(width, height);
        ImGui_ImplWGPU_CreateDeviceObjects();
    }

    // Start the Dear ImGui frame
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (showDemoWindow) {
        ImGui::ShowDemoWindow(&showDemoWindow);
    }

    {
        // Create a window called "Hello, world!" and append into it.
        ImGui::Begin("Hello, world!");

        ImGui::Text("This is some useful text.");
        ImGui::Checkbox("Demo Window", &showDemoWindow);
        ImGui::Checkbox("Another Window", &showAnotherWindow);
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        ImGui::ColorEdit3("clear color", (float*)&clear_color);

        // AUDIO
        // if (ImGui::Button("Enable Audio")) {
        //     audioDevice.initialized();
        // }
        // ImGui::SameLine();
        // ImGui::Text("Audio = %s", audioDevice.isInitialized() ? "true" : "false");

        // FPS
        ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    if (showAnotherWindow) {
        ImGui::Begin("Another Window", &showAnotherWindow);
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me")) {
            showAnotherWindow = false;
        }
        ImGui::End();
    }

    // Rendering
    ImGui::Render();

#ifndef __EMSCRIPTEN__
    // Tick needs to be called in Dawn to display validation errors
    _gpuDevice.Tick();
#endif

    auto colorAttachment = wgpu::RenderPassColorAttachment{};
    colorAttachment.depthSlice = wgpu::kDepthSliceUndefined;
    colorAttachment.loadOp = wgpu::LoadOp::Clear;
    colorAttachment.storeOp = wgpu::StoreOp::Store;
    colorAttachment.clearValue = {
        clear_color.x * clear_color.w,
        clear_color.y * clear_color.w,
        clear_color.z * clear_color.w,
        clear_color.w,
    };
    colorAttachment.view = _gpuSwapChain.GetCurrentTextureView();

    auto renderPassDesc = wgpu::RenderPassDescriptor{};
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &colorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;

    auto enc_desc = wgpu::CommandEncoderDescriptor{};
    auto encoder = _gpuDevice.CreateCommandEncoder(&enc_desc);

    auto pass = encoder.BeginRenderPass(&renderPassDesc);
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass.Get());
    pass.End();

    auto cmd_buffer_desc = wgpu::CommandBufferDescriptor{};
    auto cmd_buffer = encoder.Finish(&cmd_buffer_desc);
    auto queue = _gpuDevice.GetQueue();
    queue.Submit(1, &cmd_buffer);

#ifndef __EMSCRIPTEN__
    _gpuSwapChain.Present();
#endif
}

bool Window::initWebGPU() {
    wgpu::Instance instance = wgpu::CreateInstance(nullptr);
    _gpuDevice = tobi::gpu::getDefaultDevice(instance);

#ifdef __EMSCRIPTEN__
    wgpu::SurfaceDescriptorFromCanvasHTMLSelector html_surface_desc = {};
    html_surface_desc.selector = "#canvas";
    wgpu::SurfaceDescriptor surface_desc = {};
    surface_desc.nextInChain = &html_surface_desc;
    wgpu::Surface surface = instance.CreateSurface(&surface_desc);

    wgpu::Adapter adapter = {};
    _gpuPreferredFormat = surface.GetPreferredFormat(adapter);
#else
    wgpu::Surface surface = wgpu::glfw::CreateSurfaceForWindow(instance, _window);
    if (!surface) {
        return false;
    }
    _gpuPreferredFormat = wgpu::TextureFormat::BGRA8Unorm;
#endif

    _gpuInstance = instance;
    _gpuSurface = surface;

    _gpuDevice.SetUncapturedErrorCallback(tobi::gpu::errorCallback, nullptr);

    // tobi::gpu::inspectAdapter(_gpuDevice.GetAdapter());
    tobi::gpu::inspectDevice(_gpuDevice);
    return true;
}

void Window::createSwapChain(int width, int height) {
    if (_gpuSwapChain) {
        _gpuSwapChain = nullptr;
    }

    _width = width;
    _height = height;

    auto descriptor = wgpu::SwapChainDescriptor{};
    descriptor.usage = wgpu::TextureUsage::RenderAttachment;
    descriptor.format = _gpuPreferredFormat;
    descriptor.width = width;
    descriptor.height = height;
    descriptor.presentMode = wgpu::PresentMode::Fifo;

    _gpuSwapChain = _gpuDevice.CreateSwapChain(_gpuSurface, &descriptor);
}

}  // namespace tobi
