#pragma once

#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>

namespace tobi {

struct Window {
    Window();
    ~Window();

    Window(Window const& other) = delete;
    Window(Window&& other) = delete;

    auto operator=(Window const& other) -> Window& = delete;
    auto operator=(Window&& other) -> Window& = delete;

    auto show() -> void;

  private:
    auto loop() -> void;

    auto initWebGPU() -> bool;
    auto createSwapChain(int width, int height) -> void;

    GLFWwindow* _window{nullptr};
    int _width = 1280;
    int _height = 720;

    wgpu::Instance _gpuInstance{};
    wgpu::Device _gpuDevice{};
    wgpu::Surface _gpuSurface{};
    wgpu::SwapChain _gpuSwapChain{};
    wgpu::TextureFormat _gpuPreferredFormat{wgpu::TextureFormat::RGBA8Unorm};
};

}  // namespace tobi
