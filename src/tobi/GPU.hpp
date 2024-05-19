#pragma once

#include <webgpu/webgpu_cpp.h>

namespace tobi::gpu {

[[nodiscard]] auto getDefaultDevice(wgpu::Instance instance) -> wgpu::Device;

[[nodiscard]] auto createShaderModule(const wgpu::Device& device,
                                      const char* source) -> wgpu::ShaderModule;

auto inspectAdapter(wgpu::Adapter const& adapter) -> void;
auto inspectDevice(wgpu::Device const& device) -> void;

auto errorCallback(WGPUErrorType errorType, const char* message, void* userData) -> void;

}  // namespace tobi::gpu
