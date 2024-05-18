#pragma once

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#endif

#include <webgpu/webgpu_cpp.h>

#include <cstdio>
#include <iostream>

namespace tobi::gpu {

#ifndef __EMSCRIPTEN__
[[nodiscard]] inline auto requesAdapter(wgpu::Instance& instance) -> wgpu::Adapter {
    auto callback = [](auto status, WGPUAdapter adapter, const char* message, void* pUserData) {
        if (status == WGPURequestAdapterStatus_Success) {
            *(wgpu::Adapter*)(pUserData) = wgpu::Adapter{adapter};
        } else {
            printf("Could not get WebGPU adapter: %s\n", message);
        }
    };
    wgpu::Adapter adapter;
    instance.RequestAdapter(nullptr, callback, (void*)&adapter);
    return adapter;
}

[[nodiscard]] inline auto requestDevice(wgpu::Adapter& adapter) -> wgpu::Device {
    auto callback = [](WGPURequestDeviceStatus status, WGPUDevice device, const char* message,
                       void* pUserData) {
        if (status == WGPURequestDeviceStatus_Success) {
            *(wgpu::Device*)(pUserData) = wgpu::Device{device};
        } else {
            printf("Could not get WebGPU device: %s\n", message);
        }
    };
    wgpu::Device device;
    adapter.RequestDevice(nullptr, callback, (void*)&device);
    return device;
}
#endif

[[nodiscard]] inline auto getDefaultDevice(wgpu::Instance instance) -> wgpu::Device {
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

[[nodiscard]] inline auto createShaderModule(const wgpu::Device& device,
                                             const char* source) -> wgpu::ShaderModule {
    auto wgsl = wgpu::ShaderModuleWGSLDescriptor{};
    wgsl.code = source;

    auto descriptor = wgpu::ShaderModuleDescriptor{};
    descriptor.nextInChain = &wgsl;

    return device.CreateShaderModule(&descriptor);
}

inline auto inspectAdapter(wgpu::Adapter adapter) -> void {
    std::vector<wgpu::FeatureName> features;
    size_t featureCount = adapter.EnumerateFeatures(nullptr);
    features.resize(featureCount);
    adapter.EnumerateFeatures(features.data());

    std::cout << "Adapter features:" << std::endl;
    for (auto f : features) {
        std::cout << " - " << static_cast<uint32_t>(f) << std::endl;
    }

    wgpu::SupportedLimits limits = {};
    limits.nextInChain = nullptr;
    bool success = adapter.GetLimits(&limits);
    if (success) {
        // clang-format off
		std::cout << "Adapter limits:" << std::endl;
		std::cout << " - maxTextureDimension1D: " << limits.limits.maxTextureDimension1D << std::endl;
		std::cout << " - maxTextureDimension2D: " << limits.limits.maxTextureDimension2D << std::endl;
		std::cout << " - maxTextureDimension3D: " << limits.limits.maxTextureDimension3D << std::endl;
		std::cout << " - maxTextureArrayLayers: " << limits.limits.maxTextureArrayLayers << std::endl;
		std::cout << " - maxBindGroups: " << limits.limits.maxBindGroups << std::endl;
		std::cout << " - maxDynamicUniformBuffersPerPipelineLayout: " << limits.limits.maxDynamicUniformBuffersPerPipelineLayout << std::endl;
		std::cout << " - maxDynamicStorageBuffersPerPipelineLayout: " << limits.limits.maxDynamicStorageBuffersPerPipelineLayout << std::endl;
		std::cout << " - maxSampledTexturesPerShaderStage: " << limits.limits.maxSampledTexturesPerShaderStage << std::endl;
		std::cout << " - maxSamplersPerShaderStage: " << limits.limits.maxSamplersPerShaderStage << std::endl;
		std::cout << " - maxStorageBuffersPerShaderStage: " << limits.limits.maxStorageBuffersPerShaderStage << std::endl;
		std::cout << " - maxStorageTexturesPerShaderStage: " << limits.limits.maxStorageTexturesPerShaderStage << std::endl;
		std::cout << " - maxUniformBuffersPerShaderStage: " << limits.limits.maxUniformBuffersPerShaderStage << std::endl;
		std::cout << " - maxUniformBufferBindingSize: " << limits.limits.maxUniformBufferBindingSize << std::endl;
		std::cout << " - maxStorageBufferBindingSize: " << limits.limits.maxStorageBufferBindingSize << std::endl;
		std::cout << " - minUniformBufferOffsetAlignment: " << limits.limits.minUniformBufferOffsetAlignment << std::endl;
		std::cout << " - minStorageBufferOffsetAlignment: " << limits.limits.minStorageBufferOffsetAlignment << std::endl;
		std::cout << " - maxVertexBuffers: " << limits.limits.maxVertexBuffers << std::endl;
		std::cout << " - maxVertexAttributes: " << limits.limits.maxVertexAttributes << std::endl;
		std::cout << " - maxVertexBufferArrayStride: " << limits.limits.maxVertexBufferArrayStride << std::endl;
		std::cout << " - maxInterStageShaderComponents: " << limits.limits.maxInterStageShaderComponents << std::endl;
		std::cout << " - maxComputeWorkgroupStorageSize: " << limits.limits.maxComputeWorkgroupStorageSize << std::endl;
		std::cout << " - maxComputeInvocationsPerWorkgroup: " << limits.limits.maxComputeInvocationsPerWorkgroup << std::endl;
		std::cout << " - maxComputeWorkgroupSizeX: " << limits.limits.maxComputeWorkgroupSizeX << std::endl;
		std::cout << " - maxComputeWorkgroupSizeY: " << limits.limits.maxComputeWorkgroupSizeY << std::endl;
		std::cout << " - maxComputeWorkgroupSizeZ: " << limits.limits.maxComputeWorkgroupSizeZ << std::endl;
		std::cout << " - maxComputeWorkgroupsPerDimension: " << limits.limits.maxComputeWorkgroupsPerDimension << std::endl;
        // clang-format on
    }

    wgpu::AdapterProperties properties = {};
    properties.nextInChain = nullptr;
    adapter.GetProperties(&properties);
    std::cout << "Adapter properties:" << std::endl;
    std::cout << " - vendorID: " << properties.vendorID << std::endl;
    std::cout << " - deviceID: " << properties.deviceID << std::endl;
    std::cout << " - name: " << properties.name << std::endl;
    if (properties.driverDescription) {
        std::cout << " - driverDescription: " << properties.driverDescription << std::endl;
    }
    std::cout << " - adapterType: " << static_cast<uint32_t>(properties.adapterType) << std::endl;
    std::cout << " - backendType: " << static_cast<uint32_t>(properties.backendType) << std::endl;
}

inline auto inspectDevice(wgpu::Device device) -> void {
    std::vector<wgpu::FeatureName> features;
    size_t featureCount = device.EnumerateFeatures(nullptr);
    features.resize(featureCount);
    device.EnumerateFeatures(features.data());

    std::cout << "Device features:" << std::endl;
    for (auto f : features) {
        std::cout << " - " << static_cast<uint32_t>(f) << std::endl;
    }

    wgpu::SupportedLimits limits = {};
    limits.nextInChain = nullptr;
    bool success = device.GetLimits(&limits);
    if (success) {
        // clang-format off
		std::cout << "Device limits:" << std::endl;
		std::cout << " - maxTextureDimension1D: " << limits.limits.maxTextureDimension1D << std::endl;
		std::cout << " - maxTextureDimension2D: " << limits.limits.maxTextureDimension2D << std::endl;
		std::cout << " - maxTextureDimension3D: " << limits.limits.maxTextureDimension3D << std::endl;
		std::cout << " - maxTextureArrayLayers: " << limits.limits.maxTextureArrayLayers << std::endl;
		std::cout << " - maxBindGroups: " << limits.limits.maxBindGroups << std::endl;
		std::cout << " - maxDynamicUniformBuffersPerPipelineLayout: " << limits.limits.maxDynamicUniformBuffersPerPipelineLayout << std::endl;
		std::cout << " - maxDynamicStorageBuffersPerPipelineLayout: " << limits.limits.maxDynamicStorageBuffersPerPipelineLayout << std::endl;
		std::cout << " - maxSampledTexturesPerShaderStage: " << limits.limits.maxSampledTexturesPerShaderStage << std::endl;
		std::cout << " - maxSamplersPerShaderStage: " << limits.limits.maxSamplersPerShaderStage << std::endl;
		std::cout << " - maxStorageBuffersPerShaderStage: " << limits.limits.maxStorageBuffersPerShaderStage << std::endl;
		std::cout << " - maxStorageTexturesPerShaderStage: " << limits.limits.maxStorageTexturesPerShaderStage << std::endl;
		std::cout << " - maxUniformBuffersPerShaderStage: " << limits.limits.maxUniformBuffersPerShaderStage << std::endl;
		std::cout << " - maxUniformBufferBindingSize: " << limits.limits.maxUniformBufferBindingSize << std::endl;
		std::cout << " - maxStorageBufferBindingSize: " << limits.limits.maxStorageBufferBindingSize << std::endl;
		std::cout << " - minUniformBufferOffsetAlignment: " << limits.limits.minUniformBufferOffsetAlignment << std::endl;
		std::cout << " - minStorageBufferOffsetAlignment: " << limits.limits.minStorageBufferOffsetAlignment << std::endl;
		std::cout << " - maxVertexBuffers: " << limits.limits.maxVertexBuffers << std::endl;
		std::cout << " - maxVertexAttributes: " << limits.limits.maxVertexAttributes << std::endl;
		std::cout << " - maxVertexBufferArrayStride: " << limits.limits.maxVertexBufferArrayStride << std::endl;
		std::cout << " - maxInterStageShaderComponents: " << limits.limits.maxInterStageShaderComponents << std::endl;
		std::cout << " - maxComputeWorkgroupStorageSize: " << limits.limits.maxComputeWorkgroupStorageSize << std::endl;
		std::cout << " - maxComputeInvocationsPerWorkgroup: " << limits.limits.maxComputeInvocationsPerWorkgroup << std::endl;
		std::cout << " - maxComputeWorkgroupSizeX: " << limits.limits.maxComputeWorkgroupSizeX << std::endl;
		std::cout << " - maxComputeWorkgroupSizeY: " << limits.limits.maxComputeWorkgroupSizeY << std::endl;
		std::cout << " - maxComputeWorkgroupSizeZ: " << limits.limits.maxComputeWorkgroupSizeZ << std::endl;
		std::cout << " - maxComputeWorkgroupsPerDimension: " << limits.limits.maxComputeWorkgroupsPerDimension << std::endl;
        // clang-format on
    }
}

inline auto errorCallback(WGPUErrorType errorType, const char* message, void*) -> void {
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
    std::printf("%s error: %s\n", type, message);
}

}  // namespace tobi::gpu
