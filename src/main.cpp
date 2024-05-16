#include "miniaudio.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_wgpu.h"

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

#include <stdio.h>

// Global WebGPU required states
static auto wgpu_instance = WGPUInstance{nullptr};
static auto wgpu_device = WGPUDevice{nullptr};
static auto wgpu_surface = WGPUSurface{nullptr};
static auto wgpu_preferred_fmt = WGPUTextureFormat{WGPUTextureFormat_RGBA8Unorm};
static auto swapChain = WGPUSwapChain{nullptr};
static int swapChainWidth = 1280;
static int swapChainHeight = 720;

// Forward declarations
static bool initWGPU(GLFWwindow* window);
static void createSwapChain(int width, int height);

static void glfw_error_callback(int error, const char* description) {
    printf("GLFW Error %d: %s\n", error, description);
}

static void wgpu_error_callback(WGPUErrorType error_type, const char* message, void*) {
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

#define DEVICE_FORMAT ma_format_f32
#define DEVICE_CHANNELS 2
#define DEVICE_SAMPLE_RATE 48000

static void data_callback(ma_device* pDevice,
                          void* pOutput,
                          const void* pInput,
                          ma_uint32 frameCount) {
    ma_waveform* pSineWave;

    assert(pDevice->playback.channels == DEVICE_CHANNELS);

    pSineWave = (ma_waveform*)pDevice->pUserData;
    assert(pSineWave != NULL);

    ma_waveform_read_pcm_frames(pSineWave, pOutput, frameCount, NULL);

    (void)pInput; /* Unused. */
}

// Main code
int main(int, char**) {
    ma_waveform sineWave;
    ma_device_config deviceConfig;
    ma_device device;
    ma_waveform_config sineWaveConfig;

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        return 1;
    }

    // Make sure GLFW does not initialize any graphics context.
    // This needs to be done explicitly later.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(swapChainWidth, swapChainHeight,
                                          "Dear ImGui GLFW+WebGPU example", nullptr, nullptr);
    if (window == nullptr) {
        return 1;
    }

    // Initialize the WebGPU environment
    if (!initWGPU(window)) {
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
        return 1;
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
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOther(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
#endif
    ImGui_ImplWGPU_InitInfo init_info;
    init_info.Device = wgpu_device;
    init_info.NumFramesInFlight = 3;
    init_info.RenderTargetFormat = wgpu_preferred_fmt;
    init_info.DepthStencilFormat = WGPUTextureFormat_Undefined;
    ImGui_ImplWGPU_Init(&init_info);

    // Our state
    bool showDemoWindow = true;
    bool showAnotherWindow = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

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
        glfwGetFramebufferSize((GLFWwindow*)window, &width, &height);
        if (width != swapChainWidth || height != swapChainHeight) {
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
            static float f = 0.0f;
            static bool audioIsEnabled = false;

            // Create a window called "Hello, world!" and append into it.
            ImGui::Begin("Hello, world!");

            ImGui::Text("This is some useful text.");
            ImGui::Checkbox("Demo Window", &showDemoWindow);
            ImGui::Checkbox("Another Window", &showAnotherWindow);
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::ColorEdit3("clear color", (float*)&clear_color);

            // Buttons return true when clicked (most widgets return true when edited/activated)
            if (ImGui::Button("Enable Audio")) {
                if (not audioIsEnabled) {
                    audioIsEnabled = true;

                    deviceConfig = ma_device_config_init(ma_device_type_playback);
                    deviceConfig.playback.format = DEVICE_FORMAT;
                    deviceConfig.playback.channels = DEVICE_CHANNELS;
                    deviceConfig.sampleRate = DEVICE_SAMPLE_RATE;
                    deviceConfig.dataCallback = data_callback;
                    deviceConfig.pUserData = &sineWave;

                    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
                        printf("Failed to open playback device.\n");
                    }

                    printf("Device Name: %s\n", device.playback.name);

                    sineWaveConfig =
                        ma_waveform_config_init(device.playback.format, device.playback.channels,
                                                device.sampleRate, ma_waveform_type_sine, 0.2, 220);
                    ma_waveform_init(&sineWaveConfig, &sineWave);

                    if (ma_device_start(&device) != MA_SUCCESS) {
                        printf("Failed to start playback device.\n");
                        ma_device_uninit(&device);
                    }
                }
            }

            ImGui::SameLine();
            ImGui::Text("Audio = %s", audioIsEnabled ? "true" : "false");

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
        wgpuDeviceTick(wgpu_device);
#endif

        WGPURenderPassColorAttachment color_attachments = {};
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

        WGPURenderPassDescriptor render_pass_desc = {};
        render_pass_desc.colorAttachmentCount = 1;
        render_pass_desc.colorAttachments = &color_attachments;
        render_pass_desc.depthStencilAttachment = nullptr;

        WGPUCommandEncoderDescriptor enc_desc = {};
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(wgpu_device, &enc_desc);

        WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_desc);
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);
        wgpuRenderPassEncoderEnd(pass);

        WGPUCommandBufferDescriptor cmd_buffer_desc = {};
        WGPUCommandBuffer cmd_buffer = wgpuCommandEncoderFinish(encoder, &cmd_buffer_desc);
        WGPUQueue queue = wgpuDeviceGetQueue(wgpu_device);
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

    /* Uninitialize the waveform after the device so we don't pull it from under the device while
     * it's being reference in the data callback. */
    ma_device_uninit(&device);
    ma_waveform_uninit(&sineWave);

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
    wgpu_device = emscripten_webgpu_get_device();
    if (!wgpu_device) {
        return false;
    }
#else
    WGPUAdapter adapter = RequestAdapter(instance);
    if (!adapter) {
        return false;
    }
    wgpu_device = RequestDevice(adapter);
#endif

#ifdef __EMSCRIPTEN__
    wgpu::SurfaceDescriptorFromCanvasHTMLSelector html_surface_desc = {};
    html_surface_desc.selector = "#canvas";
    wgpu::SurfaceDescriptor surface_desc = {};
    surface_desc.nextInChain = &html_surface_desc;
    wgpu::Surface surface = instance.CreateSurface(&surface_desc);

    wgpu::Adapter adapter = {};
    wgpu_preferred_fmt = (WGPUTextureFormat)surface.GetPreferredFormat(adapter);
#else
    wgpu::Surface surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);
    if (!surface) {
        return false;
    }
    wgpu_preferred_fmt = WGPUTextureFormat_BGRA8Unorm;
#endif

    wgpu_instance = instance.MoveToCHandle();
    wgpu_surface = surface.MoveToCHandle();

    wgpuDeviceSetUncapturedErrorCallback(wgpu_device, wgpu_error_callback, nullptr);

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
    descriptor.format = wgpu_preferred_fmt;
    descriptor.width = width;
    descriptor.height = height;
    descriptor.presentMode = WGPUPresentMode_Fifo;

    swapChain = wgpuDeviceCreateSwapChain(wgpu_device, wgpu_surface, &descriptor);
}
