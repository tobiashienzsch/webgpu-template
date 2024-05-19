#include "GPU.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#endif

#include <cstdio>
#include <iostream>

namespace tobi::gpu {

#ifndef __EMSCRIPTEN__
auto requesAdapter(wgpu::Instance& instance) -> wgpu::Adapter {
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

auto requestDevice(wgpu::Adapter& adapter) -> wgpu::Device {
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

    std::printf("Adapter features:\n");
    for (auto f : features) {
        std::printf(" - %u\n", static_cast<uint32_t>(f));
    }

    wgpu::SupportedLimits limits = {};
    limits.nextInChain = nullptr;
    bool success = adapter.GetLimits(&limits);
    if (success) {
        // clang-format off
		std::printf("Adapter limits:\n");
		std::printf("maxTextureDimension1D: %u\n", limits.limits.maxTextureDimension1D);
		std::printf("maxTextureDimension2D: %u\n", limits.limits.maxTextureDimension2D);
		std::printf("maxTextureDimension3D: %u\n", limits.limits.maxTextureDimension3D);
		std::printf("maxTextureArrayLayers: %u\n", limits.limits.maxTextureArrayLayers);
		std::printf("maxBindGroups: %u\n", limits.limits.maxBindGroups);
		std::printf("maxDynamicUniformBuffersPerPipelineLayout: %u\n", limits.limits.maxDynamicUniformBuffersPerPipelineLayout);
		std::printf("maxDynamicStorageBuffersPerPipelineLayout: %u\n", limits.limits.maxDynamicStorageBuffersPerPipelineLayout);
		std::printf("maxSampledTexturesPerShaderStage: %u\n", limits.limits.maxSampledTexturesPerShaderStage);
		std::printf("maxSamplersPerShaderStage: %u\n", limits.limits.maxSamplersPerShaderStage);
		std::printf("maxStorageBuffersPerShaderStage: %u\n", limits.limits.maxStorageBuffersPerShaderStage);
		std::printf("maxStorageTexturesPerShaderStage: %u\n", limits.limits.maxStorageTexturesPerShaderStage);
		std::printf("maxUniformBuffersPerShaderStage: %u\n", limits.limits.maxUniformBuffersPerShaderStage);
		std::printf("maxUniformBufferBindingSize: %llu\n", limits.limits.maxUniformBufferBindingSize);
		std::printf("maxStorageBufferBindingSize: %llu\n", limits.limits.maxStorageBufferBindingSize);
		std::printf("minUniformBufferOffsetAlignment: %u\n", limits.limits.minUniformBufferOffsetAlignment);
		std::printf("minStorageBufferOffsetAlignment: %u\n", limits.limits.minStorageBufferOffsetAlignment);
		std::printf("maxVertexBuffers: %u\n", limits.limits.maxVertexBuffers);
		std::printf("maxVertexAttributes: %u\n", limits.limits.maxVertexAttributes);
		std::printf("maxVertexBufferArrayStride: %u\n", limits.limits.maxVertexBufferArrayStride);
		std::printf("maxInterStageShaderComponents: %u\n", limits.limits.maxInterStageShaderComponents);
		std::printf("maxComputeWorkgroupStorageSize: %u\n", limits.limits.maxComputeWorkgroupStorageSize);
		std::printf("maxComputeInvocationsPerWorkgroup: %u\n", limits.limits.maxComputeInvocationsPerWorkgroup);
		std::printf("maxComputeWorkgroupSizeX: %u\n", limits.limits.maxComputeWorkgroupSizeX);
		std::printf("maxComputeWorkgroupSizeY: %u\n", limits.limits.maxComputeWorkgroupSizeY);
		std::printf("maxComputeWorkgroupSizeZ: %u\n", limits.limits.maxComputeWorkgroupSizeZ);
		std::printf("maxComputeWorkgroupsPerDimension: %u\n", limits.limits.maxComputeWorkgroupsPerDimension);
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

auto inspectDevice(wgpu::Device const& device) -> void {
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
        std::printf("Device limits:\n");
		std::printf("maxTextureDimension1D: %u\n", limits.limits.maxTextureDimension1D);
		std::printf("maxTextureDimension2D: %u\n", limits.limits.maxTextureDimension2D);
		std::printf("maxTextureDimension3D: %u\n", limits.limits.maxTextureDimension3D);
		std::printf("maxTextureArrayLayers: %u\n", limits.limits.maxTextureArrayLayers);
		std::printf("maxBindGroups: %u\n", limits.limits.maxBindGroups);
		std::printf("maxDynamicUniformBuffersPerPipelineLayout: %u\n", limits.limits.maxDynamicUniformBuffersPerPipelineLayout);
		std::printf("maxDynamicStorageBuffersPerPipelineLayout: %u\n", limits.limits.maxDynamicStorageBuffersPerPipelineLayout);
		std::printf("maxSampledTexturesPerShaderStage: %u\n", limits.limits.maxSampledTexturesPerShaderStage);
		std::printf("maxSamplersPerShaderStage: %u\n", limits.limits.maxSamplersPerShaderStage);
		std::printf("maxStorageBuffersPerShaderStage: %u\n", limits.limits.maxStorageBuffersPerShaderStage);
		std::printf("maxStorageTexturesPerShaderStage: %u\n", limits.limits.maxStorageTexturesPerShaderStage);
		std::printf("maxUniformBuffersPerShaderStage: %u\n", limits.limits.maxUniformBuffersPerShaderStage);
		std::printf("maxUniformBufferBindingSize: %llu\n", limits.limits.maxUniformBufferBindingSize);
		std::printf("maxStorageBufferBindingSize: %llu\n", limits.limits.maxStorageBufferBindingSize);
		std::printf("minUniformBufferOffsetAlignment: %u\n", limits.limits.minUniformBufferOffsetAlignment);
		std::printf("minStorageBufferOffsetAlignment: %u\n", limits.limits.minStorageBufferOffsetAlignment);
		std::printf("maxVertexBuffers: %u\n", limits.limits.maxVertexBuffers);
		std::printf("maxVertexAttributes: %u\n", limits.limits.maxVertexAttributes);
		std::printf("maxVertexBufferArrayStride: %u\n", limits.limits.maxVertexBufferArrayStride);
		std::printf("maxInterStageShaderComponents: %u\n", limits.limits.maxInterStageShaderComponents);
		std::printf("maxComputeWorkgroupStorageSize: %u\n", limits.limits.maxComputeWorkgroupStorageSize);
		std::printf("maxComputeInvocationsPerWorkgroup: %u\n", limits.limits.maxComputeInvocationsPerWorkgroup);
		std::printf("maxComputeWorkgroupSizeX: %u\n", limits.limits.maxComputeWorkgroupSizeX);
		std::printf("maxComputeWorkgroupSizeY: %u\n", limits.limits.maxComputeWorkgroupSizeY);
		std::printf("maxComputeWorkgroupSizeZ: %u\n", limits.limits.maxComputeWorkgroupSizeZ);
		std::printf("maxComputeWorkgroupsPerDimension: %u\n", limits.limits.maxComputeWorkgroupsPerDimension);
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
    std::printf("%s error: %s\n", type, message);
}

}  // namespace tobi::gpu
