#include <tobi/GPU.hpp>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>

static constexpr auto const* ShaderCode = R"(
    @group(0) @binding(0) var<storage, read> lhs: array<f32>;
    @group(0) @binding(1) var<storage, read> rhs: array<f32>;
    @group(0) @binding(2) var<storage, read_write> out: array<f32>;

    @compute @workgroup_size(1)
    fn main(@builtin(global_invocation_id) id: vec3<u32>) {
        let index : u32 = id.x;
        out[index] = lhs[index] + rhs[index];
    }
)";

auto main(int, char**) -> int {
    auto instance = wgpu::CreateInstance(nullptr);
    auto device = tobi::gpu::getDefaultDevice(instance);
    if (not device) {
        return EXIT_FAILURE;
    }

    device.SetUncapturedErrorCallback(tobi::gpu::errorCallback, nullptr);
    // tobi::gpu::inspectDevice(device);

    wgpu::Queue queue = device.GetQueue();

    // Create buffers
    const size_t dataSize = 1024 * sizeof(float);
    auto dataLHS = std::vector<float>(1024, 1.0f);
    auto dataRHS = std::vector<float>(1024, 2.0f);
    auto dataOut = std::vector<float>(1024, 0.0f);

    auto buffer = wgpu::BufferDescriptor{};
    buffer.size = dataSize;
    buffer.usage =
        wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc;

    auto lhs = device.CreateBuffer(&buffer);
    auto rhs = device.CreateBuffer(&buffer);
    auto out = device.CreateBuffer(&buffer);

    queue.WriteBuffer(lhs, 0, dataLHS.data(), dataSize);
    queue.WriteBuffer(rhs, 0, dataRHS.data(), dataSize);

    auto shaderModule = tobi::gpu::createShaderModule(device, ShaderCode);

    // Create the pipeline
    wgpu::ComputePipelineDescriptor pipelineDesc;
    pipelineDesc.compute.module = shaderModule;
    pipelineDesc.compute.entryPoint = "main";
    wgpu::ComputePipeline pipeline = device.CreateComputePipeline(&pipelineDesc);

    // wgpu::BindGroupLayoutEntry entries[3];
    // entries[0].binding = 0;
    // entries[0].visibility = wgpu::ShaderStage::Compute;
    // entries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
    // entries[1].binding = 1;
    // entries[1].visibility = wgpu::ShaderStage::Compute;
    // entries[1].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
    // entries[2].binding = 2;
    // entries[2].visibility = wgpu::ShaderStage::Compute;
    // entries[2].buffer.type = wgpu::BufferBindingType::Storage;

    // wgpu::BindGroupLayoutDescriptor bdlDescriptor;
    // bdlDescriptor.entryCount = std::size(entries);
    // bdlDescriptor.entries = std::data(entries);
    // wgpu::BindGroupLayout bindGroupLayout = device.CreateBindGroupLayout(&bdlDescriptor);

    // wgpu::PipelineLayoutDescriptor pipelineDescriptor;
    // pipelineDescriptor.bindGroupLayoutCount = 1;
    // pipelineDescriptor.bindGroupLayouts = std::addressof(bindGroupLayout);
    // wgpu::PipelineLayout pl = device.CreatePipelineLayout(&pipelineDescriptor);

    // wgpu::ComputePipelineDescriptor csDesc;
    // // csDesc.layout = pl;
    // csDesc.compute.module = shaderModule;
    // csDesc.compute.entryPoint = "main";
    // auto computePipeline = device.CreateComputePipeline(&csDesc);

    wgpu::BindGroupLayout bindGroupLayout = pipeline.GetBindGroupLayout(0);

    // Create the bind group
    wgpu::BindGroupEntry bgEntries[3];
    bgEntries[0].binding = 0;
    bgEntries[0].buffer = lhs;
    bgEntries[0].offset = 0;
    bgEntries[0].size = dataSize;

    bgEntries[1].binding = 1;
    bgEntries[1].buffer = rhs;
    bgEntries[1].offset = 0;
    bgEntries[1].size = dataSize;

    bgEntries[2].binding = 2;
    bgEntries[2].buffer = out;
    bgEntries[2].offset = 0;
    bgEntries[2].size = dataSize;

    auto bindGroupDesc = wgpu::BindGroupDescriptor{};
    bindGroupDesc.layout = bindGroupLayout;
    bindGroupDesc.entryCount = 3;
    bindGroupDesc.entries = bgEntries;
    wgpu::BindGroup bindGroup = device.CreateBindGroup(&bindGroupDesc);

    // Create command buffer
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder pass = encoder.BeginComputePass();
    pass.SetPipeline(pipeline);
    pass.SetBindGroup(0, bindGroup);
    pass.DispatchWorkgroups(1024, 1, 1);
    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    queue.Submit(1, &commands);

    // Read the dataOut
    wgpu::BufferDescriptor readbackDesc;
    readbackDesc.size = dataSize;
    readbackDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
    wgpu::Buffer readbackBuffer = device.CreateBuffer(&readbackDesc);

    encoder = device.CreateCommandEncoder();
    encoder.CopyBufferToBuffer(out, 0, readbackBuffer, 0, dataSize);
    commands = encoder.Finish();
    queue.Submit(1, &commands);

    readbackBuffer.MapAsync(
        wgpu::MapMode::Read, 0, dataSize,
        [](WGPUBufferMapAsyncStatus status, void* userdata) {
            assert(status == WGPUBufferMapAsyncStatus_Success);
        },
        nullptr);

#if not defined(__EMSCRIPTEN__)
    // Tick needs to be called in Dawn to display validation errors
    device.Tick();
#endif

    // const void* mappedData = readbackBuffer.GetMappedRange();
    // std::memcpy(dataOut.data(), mappedData, dataSize);
    readbackBuffer.Unmap();

    // Print dataOut
    for (size_t i = 0; i < 10; ++i) {
        std::cout << dataOut[i] << " ";
    }
    std::cout << std::endl;

    return EXIT_SUCCESS;
}
