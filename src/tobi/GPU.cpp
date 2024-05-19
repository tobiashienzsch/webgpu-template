#include "GPU.hpp"

#include <fmt/format.h>
#include <fmt/os.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#endif

namespace tobi::gpu {

#ifndef __EMSCRIPTEN__
auto requesAdapter(wgpu::Instance& instance) -> wgpu::Adapter {
    auto callback = [](auto status, WGPUAdapter adapter, const char* message, void* pUserData) {
        if (status == WGPURequestAdapterStatus_Success) {
            *(wgpu::Adapter*)(pUserData) = wgpu::Adapter{adapter};
        } else {
            fmt::println("Could not get WebGPU adapter: {}", message);
        }
    };
    wgpu::Adapter adapter;
    instance.RequestAdapter(nullptr, callback, static_cast<void*>(&adapter));
    return adapter;
}

auto requestDevice(wgpu::Adapter& adapter) -> wgpu::Device {
    auto callback = [](WGPURequestDeviceStatus status, WGPUDevice device, const char* message,
                       void* pUserData) {
        if (status == WGPURequestDeviceStatus_Success) {
            *(wgpu::Device*)(pUserData) = wgpu::Device{device};
        } else {
            fmt::println("Could not get WebGPU device: {}", message);
        }
    };
    wgpu::Device device;
    adapter.RequestDevice(nullptr, callback, static_cast<void*>(&device));
    return device;
}
#endif

auto getDefaultDevice(wgpu::Instance instance) -> wgpu::Device {
#ifdef __EMSCRIPTEN__
    auto device = wgpu::Device{emscripten_webgpu_get_device()};
    if (not device) {
        return {};
    }
#else
    auto adapter = requesAdapter(instance);
    if (not adapter) {
        return {};
    }
    auto device = requestDevice(adapter);
#endif

    return device;
}

auto createShaderModule(const wgpu::Device& device, const char* source) -> wgpu::ShaderModule {
    auto wgsl = wgpu::ShaderModuleWGSLDescriptor{};
    wgsl.code = source;

    auto descriptor = wgpu::ShaderModuleDescriptor{};
    descriptor.nextInChain = &wgsl;

    return device.CreateShaderModule(&descriptor);
}

auto inspectAdapter(wgpu::Adapter const& adapter) -> void {
    std::vector<wgpu::FeatureName> features;
    size_t featureCount = adapter.EnumerateFeatures(nullptr);
    features.resize(featureCount);
    adapter.EnumerateFeatures(features.data());

    fmt::println("Adapter features:");
    for (auto f : features) {
        fmt::println(" - {}", static_cast<uint32_t>(f));
    }

    wgpu::SupportedLimits limits = {};
    limits.nextInChain = nullptr;
    bool success = adapter.GetLimits(&limits);
    if (success) {
        // clang-format off
		fmt::println("Adapter limits:");
		fmt::println("maxTextureDimension1D: {}", limits.limits.maxTextureDimension1D);
		fmt::println("maxTextureDimension2D: {}", limits.limits.maxTextureDimension2D);
		fmt::println("maxTextureDimension3D: {}", limits.limits.maxTextureDimension3D);
		fmt::println("maxTextureArrayLayers: {}", limits.limits.maxTextureArrayLayers);
		fmt::println("maxBindGroups: {}", limits.limits.maxBindGroups);
		fmt::println("maxDynamicUniformBuffersPerPipelineLayout: {}", limits.limits.maxDynamicUniformBuffersPerPipelineLayout);
		fmt::println("maxDynamicStorageBuffersPerPipelineLayout: {}", limits.limits.maxDynamicStorageBuffersPerPipelineLayout);
		fmt::println("maxSampledTexturesPerShaderStage: {}", limits.limits.maxSampledTexturesPerShaderStage);
		fmt::println("maxSamplersPerShaderStage: {}", limits.limits.maxSamplersPerShaderStage);
		fmt::println("maxStorageBuffersPerShaderStage: {}", limits.limits.maxStorageBuffersPerShaderStage);
		fmt::println("maxStorageTexturesPerShaderStage: {}", limits.limits.maxStorageTexturesPerShaderStage);
		fmt::println("maxUniformBuffersPerShaderStage: {}", limits.limits.maxUniformBuffersPerShaderStage);
		fmt::println("maxUniformBufferBindingSize: {}", limits.limits.maxUniformBufferBindingSize);
		fmt::println("maxStorageBufferBindingSize: {}", limits.limits.maxStorageBufferBindingSize);
		fmt::println("minUniformBufferOffsetAlignment: {}", limits.limits.minUniformBufferOffsetAlignment);
		fmt::println("minStorageBufferOffsetAlignment: {}", limits.limits.minStorageBufferOffsetAlignment);
		fmt::println("maxVertexBuffers: {}", limits.limits.maxVertexBuffers);
		fmt::println("maxVertexAttributes: {}", limits.limits.maxVertexAttributes);
		fmt::println("maxVertexBufferArrayStride: {}", limits.limits.maxVertexBufferArrayStride);
		fmt::println("maxInterStageShaderComponents: {}", limits.limits.maxInterStageShaderComponents);
		fmt::println("maxComputeWorkgroupStorageSize: {}", limits.limits.maxComputeWorkgroupStorageSize);
		fmt::println("maxComputeInvocationsPerWorkgroup: {}", limits.limits.maxComputeInvocationsPerWorkgroup);
		fmt::println("maxComputeWorkgroupSizeX: {}", limits.limits.maxComputeWorkgroupSizeX);
		fmt::println("maxComputeWorkgroupSizeY: {}", limits.limits.maxComputeWorkgroupSizeY);
		fmt::println("maxComputeWorkgroupSizeZ: {}", limits.limits.maxComputeWorkgroupSizeZ);
		fmt::println("maxComputeWorkgroupsPerDimension: {}", limits.limits.maxComputeWorkgroupsPerDimension);
        // clang-format on
    }

    wgpu::AdapterProperties properties = {};
    properties.nextInChain = nullptr;
    adapter.GetProperties(&properties);
    fmt::println("Adapter properties:");
    fmt::println(" - vendorID: {}", properties.vendorID);
    fmt::println(" - deviceID: {}", properties.deviceID);
    fmt::println(" - name: {}", properties.name);
    if (properties.driverDescription) {
        fmt::println(" - driverDescription: {}", properties.driverDescription);
    }
    fmt::println(" - adapterType: {}", static_cast<uint32_t>(properties.adapterType));
    fmt::println(" - backendType: {}", static_cast<uint32_t>(properties.backendType));
}

auto inspectDevice(wgpu::Device const& device) -> void {
    std::vector<wgpu::FeatureName> features;
    size_t featureCount = device.EnumerateFeatures(nullptr);
    features.resize(featureCount);
    device.EnumerateFeatures(features.data());

    fmt::println("Device features:");
    for (auto f : features) {
        fmt::println(" - {}", static_cast<uint32_t>(f));
    }

    wgpu::SupportedLimits limits = {};
    limits.nextInChain = nullptr;
    bool success = device.GetLimits(&limits);
    if (success) {
        // clang-format off
        fmt::println("Device limits:");
		fmt::println("maxTextureDimension1D: {}", limits.limits.maxTextureDimension1D);
		fmt::println("maxTextureDimension2D: {}", limits.limits.maxTextureDimension2D);
		fmt::println("maxTextureDimension3D: {}", limits.limits.maxTextureDimension3D);
		fmt::println("maxTextureArrayLayers: {}", limits.limits.maxTextureArrayLayers);
		fmt::println("maxBindGroups: {}", limits.limits.maxBindGroups);
		fmt::println("maxDynamicUniformBuffersPerPipelineLayout: {}", limits.limits.maxDynamicUniformBuffersPerPipelineLayout);
		fmt::println("maxDynamicStorageBuffersPerPipelineLayout: {}", limits.limits.maxDynamicStorageBuffersPerPipelineLayout);
		fmt::println("maxSampledTexturesPerShaderStage: {}", limits.limits.maxSampledTexturesPerShaderStage);
		fmt::println("maxSamplersPerShaderStage: {}", limits.limits.maxSamplersPerShaderStage);
		fmt::println("maxStorageBuffersPerShaderStage: {}", limits.limits.maxStorageBuffersPerShaderStage);
		fmt::println("maxStorageTexturesPerShaderStage: {}", limits.limits.maxStorageTexturesPerShaderStage);
		fmt::println("maxUniformBuffersPerShaderStage: {}", limits.limits.maxUniformBuffersPerShaderStage);
		fmt::println("maxUniformBufferBindingSize: {}", limits.limits.maxUniformBufferBindingSize);
		fmt::println("maxStorageBufferBindingSize: {}", limits.limits.maxStorageBufferBindingSize);
		fmt::println("minUniformBufferOffsetAlignment: {}", limits.limits.minUniformBufferOffsetAlignment);
		fmt::println("minStorageBufferOffsetAlignment: {}", limits.limits.minStorageBufferOffsetAlignment);
		fmt::println("maxVertexBuffers: {}", limits.limits.maxVertexBuffers);
		fmt::println("maxVertexAttributes: {}", limits.limits.maxVertexAttributes);
		fmt::println("maxVertexBufferArrayStride: {}", limits.limits.maxVertexBufferArrayStride);
		fmt::println("maxInterStageShaderComponents: {}", limits.limits.maxInterStageShaderComponents);
		fmt::println("maxComputeWorkgroupStorageSize: {}", limits.limits.maxComputeWorkgroupStorageSize);
		fmt::println("maxComputeInvocationsPerWorkgroup: {}", limits.limits.maxComputeInvocationsPerWorkgroup);
		fmt::println("maxComputeWorkgroupSizeX: {}", limits.limits.maxComputeWorkgroupSizeX);
		fmt::println("maxComputeWorkgroupSizeY: {}", limits.limits.maxComputeWorkgroupSizeY);
		fmt::println("maxComputeWorkgroupSizeZ: {}", limits.limits.maxComputeWorkgroupSizeZ);
		fmt::println("maxComputeWorkgroupsPerDimension: {}", limits.limits.maxComputeWorkgroupsPerDimension);
        // clang-format on
    }
}

auto errorCallback(WGPUErrorType errorType, const char* message, void*) -> void {
    const char* type = "";
    auto const error = static_cast<wgpu::ErrorType>(errorType);
    switch (error) {
        case wgpu::ErrorType::Validation:
            type = "Validation";
            break;
        case wgpu::ErrorType::OutOfMemory:
            type = "Out of memory";
            break;
        case wgpu::ErrorType::Unknown:
            type = "Unknown";
            break;
        case wgpu::ErrorType::DeviceLost:
            type = "Device lost";
            break;
        default:
            type = "Unknown";
    }
    fmt::println("{} error: {}", type, message);
}

}  // namespace tobi::gpu
