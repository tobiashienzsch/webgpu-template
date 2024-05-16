#include "AudioDevice.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#else
#include <webgpu/webgpu_glfw.h>
#endif

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>

#include <clap/clap.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_wgpu.h"

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#include <functional>
static std::function<void()> MainLoopForEmscriptenP;
static void MainLoopForEmscripten() {
    MainLoopForEmscriptenP();
}
#define EMSCRIPTEN_MAINLOOP_BEGIN MainLoopForEmscriptenP = [&]()
#define EMSCRIPTEN_MAINLOOP_END \
    ;                           \
    emscripten_set_main_loop(MainLoopForEmscripten, 0, true)
#else
#define EMSCRIPTEN_MAINLOOP_BEGIN
#define EMSCRIPTEN_MAINLOOP_END
#endif

#include <cstdio>

static bool initWGPU(GLFWwindow* window);
static void createSwapChain(int width, int height);

struct Window {
    Window() {
        glfwSetErrorCallback([](int error, const char* description) {
            printf("GLFW Error %d: %s\n", error, description);
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

    ~Window() {
        if (_window != nullptr) {
            glfwDestroyWindow(_window);
        }
        glfwTerminate();
    }

    Window(Window const& other) = delete;
    Window(Window&& other) = delete;

    auto operator=(Window const& other) -> Window& = delete;
    auto operator=(Window&& other) -> Window& = delete;

  private:
    int _width = 1280;
    int _height = 720;
    GLFWwindow* _window{nullptr};
    wgpu::SwapChain _swapChain;
};

static void glfwErrorCallback(int error, const char* description) {
    printf("GLFW Error %d: %s\n", error, description);
}

static void wgpuErrorCallback(WGPUErrorType error_type, const char* message, void*) {
    const char* error_type_lbl = "";
    switch (error_type) {
        case WGPUErrorType_Validation:
            error_type_lbl = "Validation";
            break;
        case WGPUErrorType_OutOfMemory:
            error_type_lbl = "Out of memory";
            break;
        case WGPUErrorType_Unknown:
            error_type_lbl = "Unknown";
            break;
        case WGPUErrorType_DeviceLost:
            error_type_lbl = "Device lost";
            break;
        default:
            error_type_lbl = "Unknown";
    }
    printf("%s error: %s\n", error_type_lbl, message);
}

// Global WebGPU required states
static auto gpuInstance = WGPUInstance{nullptr};
static auto gpuDevice = WGPUDevice{nullptr};
static auto gpuSurface = WGPUSurface{nullptr};
static auto gpuPreferredFormat = WGPUTextureFormat{WGPUTextureFormat_RGBA8Unorm};
static auto swapChain = WGPUSwapChain{nullptr};
static auto swapChainWidth = 1280;
static auto swapChainHeight = 720;

// Main code
int main(int, char**) {
    auto audioDevice = tobi::AudioDevice{};

    glfwSetErrorCallback(glfwErrorCallback);
    if (not glfwInit()) {
        return EXIT_FAILURE;
    }

    // Make sure GLFW does not initialize any graphics context.
    // This needs to be done explicitly later.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto const* windowTitle = "Dear ImGui GLFW+WebGPU example";
    auto* window = glfwCreateWindow(swapChainWidth, swapChainHeight, windowTitle, nullptr, nullptr);
    if (window == nullptr) {
        return EXIT_FAILURE;
    }

    // Initialize the WebGPU environment
    if (not initWGPU(window)) {
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
        return EXIT_FAILURE;
    }
    createSwapChain(swapChainWidth, swapChainHeight);
    glfwShowWindow(window);

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
    ImGui_ImplGlfw_InitForOther(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
#endif
    ImGui_ImplWGPU_InitInfo init_info;
    init_info.Device = gpuDevice;
    init_info.NumFramesInFlight = 3;
    init_info.RenderTargetFormat = gpuPreferredFormat;
    init_info.DepthStencilFormat = WGPUTextureFormat_Undefined;
    ImGui_ImplWGPU_Init(&init_info);

    // Our state
    bool showDemoWindow = true;
    bool showAnotherWindow = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    float f = 0.0f;

    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a
    // fopen() of the imgui.ini file. You may manually call LoadIniSettingsFromMemory() to load
    // settings from your own storage.
    io.IniFilename = nullptr;

    // Main loop
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        glfwPollEvents();

        // React to changes in screen size
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        if (width != swapChainWidth or height != swapChainHeight) {
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
            if (ImGui::Button("Enable Audio")) {
                audioDevice.initialized();
            }
            ImGui::SameLine();
            ImGui::Text("Audio = %s", audioDevice.isInitialized() ? "true" : "false");

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
        wgpuDeviceTick(gpuDevice);
#endif

        auto color_attachments = WGPURenderPassColorAttachment{};
        color_attachments.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        color_attachments.loadOp = WGPULoadOp_Clear;
        color_attachments.storeOp = WGPUStoreOp_Store;
        color_attachments.clearValue = {
            clear_color.x * clear_color.w,
            clear_color.y * clear_color.w,
            clear_color.z * clear_color.w,
            clear_color.w,
        };
        color_attachments.view = wgpuSwapChainGetCurrentTextureView(swapChain);

        auto renderPassDesc = WGPURenderPassDescriptor{};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &color_attachments;
        renderPassDesc.depthStencilAttachment = nullptr;

        auto enc_desc = WGPUCommandEncoderDescriptor{};
        auto encoder = wgpuDeviceCreateCommandEncoder(gpuDevice, &enc_desc);

        auto pass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);
        wgpuRenderPassEncoderEnd(pass);

        auto cmd_buffer_desc = WGPUCommandBufferDescriptor{};
        auto cmd_buffer = wgpuCommandEncoderFinish(encoder, &cmd_buffer_desc);
        auto queue = wgpuDeviceGetQueue(gpuDevice);
        wgpuQueueSubmit(queue, 1, &cmd_buffer);

#ifndef __EMSCRIPTEN__
        wgpuSwapChainPresent(swapChain);
#endif

        wgpuTextureViewRelease(color_attachments.view);
        wgpuRenderPassEncoderRelease(pass);
        wgpuCommandEncoderRelease(encoder);
        wgpuCommandBufferRelease(cmd_buffer);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

#ifndef __EMSCRIPTEN__
static WGPUAdapter RequestAdapter(wgpu::Instance& instance) {
    auto onAdapterRequestEnded = [](auto status, WGPUAdapter adapter, const char* message,
                                    void* pUserData) {
        if (status == WGPURequestAdapterStatus_Success) {
            *(WGPUAdapter*)(pUserData) = adapter;
        } else {
            printf("Could not get WebGPU adapter: %s\n", message);
        }
    };
    WGPUAdapter adapter;
    instance.RequestAdapter(nullptr, onAdapterRequestEnded, (void*)&adapter);
    return adapter;
}

static WGPUDevice RequestDevice(WGPUAdapter& adapter) {
    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device,
                                   const char* message, void* pUserData) {
        if (status == WGPURequestDeviceStatus_Success) {
            *(WGPUDevice*)(pUserData) = device;
        } else {
            printf("Could not get WebGPU device: %s\n", message);
        }
    };
    WGPUDevice device;
    wgpuAdapterRequestDevice(adapter, nullptr, onDeviceRequestEnded, (void*)&device);
    return device;
}
#endif

static bool initWGPU(GLFWwindow* window) {
    wgpu::Instance instance = wgpu::CreateInstance(nullptr);

#ifdef __EMSCRIPTEN__
    gpuDevice = emscripten_webgpu_get_device();
    if (!gpuDevice) {
        return false;
    }
#else
    WGPUAdapter adapter = RequestAdapter(instance);
    if (!adapter) {
        return false;
    }
    gpuDevice = RequestDevice(adapter);
#endif

#ifdef __EMSCRIPTEN__
    wgpu::SurfaceDescriptorFromCanvasHTMLSelector html_surface_desc = {};
    html_surface_desc.selector = "#canvas";
    wgpu::SurfaceDescriptor surface_desc = {};
    surface_desc.nextInChain = &html_surface_desc;
    wgpu::Surface surface = instance.CreateSurface(&surface_desc);

    wgpu::Adapter adapter = {};
    gpuPreferredFormat = (WGPUTextureFormat)surface.GetPreferredFormat(adapter);
#else
    wgpu::Surface surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);
    if (!surface) {
        return false;
    }
    gpuPreferredFormat = WGPUTextureFormat_BGRA8Unorm;
#endif

    gpuInstance = instance.MoveToCHandle();
    gpuSurface = surface.MoveToCHandle();

    wgpuDeviceSetUncapturedErrorCallback(gpuDevice, wgpuErrorCallback, nullptr);

    return true;
}

static void createSwapChain(int width, int height) {
    if (swapChain) {
        wgpuSwapChainRelease(swapChain);
    }
    swapChainWidth = width;
    swapChainHeight = height;

    auto descriptor = WGPUSwapChainDescriptor{};
    descriptor.usage = WGPUTextureUsage_RenderAttachment;
    descriptor.format = gpuPreferredFormat;
    descriptor.width = width;
    descriptor.height = height;
    descriptor.presentMode = WGPUPresentMode_Fifo;

    swapChain = wgpuDeviceCreateSwapChain(gpuDevice, gpuSurface, &descriptor);
}
