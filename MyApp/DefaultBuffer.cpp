#include "DefaultBuffer.h"

DefaultBuffer::DefaultBuffer(ID3D12Device* device,
                             ID3D12GraphicsCommandList* commandList,
                             const void* initData,
                             unsigned int byteSize) {
    Load(device, commandList, initData, byteSize);
}

void DefaultBuffer::Load(ID3D12Device* device,
                         ID3D12GraphicsCommandList* commandList,
                         const void* data,
                         unsigned int byteSize) {
    ThrowIfFailed(D3DCreateBlob(byteSize, bufferCpu_.ReleaseAndGetAddressOf()));
    CopyMemory(bufferCpu_->GetBufferPointer(), data, byteSize);
    bufferGpu_ = d3dUtil::CreateDefaultBuffer(device, commandList, data, byteSize, uploadBuffer_);
}

D3D12_GPU_VIRTUAL_ADDRESS DefaultBuffer::GetGpuVirtualAddress() const {
    return bufferGpu_->GetGPUVirtualAddress();
}

void DefaultBuffer::ResetUploader() {
    uploadBuffer_ = nullptr;
}

void VertexBuffer::Load(ID3D12Device* device,
                        ID3D12GraphicsCommandList* commandList,
                        const void* data,
                        unsigned int byteSize) {
    vbuffer_.Load(device, commandList, data, byteSize);
}

D3D12_VERTEX_BUFFER_VIEW VertexBuffer::GetView() const {
    D3D12_VERTEX_BUFFER_VIEW vbv{};
    vbv.BufferLocation = vbuffer_.GetGpuVirtualAddress();
    vbv.StrideInBytes = byteStride_;
    vbv.SizeInBytes = bufferByteSize_;
    return vbv;
}

void IndexBuffer::Load(ID3D12Device* device,
                       ID3D12GraphicsCommandList* commandList,
                       const void* data,
                       unsigned int byteSize) {
    ibuffer_.Load(device, commandList, data, byteSize);
}

D3D12_INDEX_BUFFER_VIEW IndexBuffer::GetView() const {
    D3D12_INDEX_BUFFER_VIEW ibv{};
    ibv.BufferLocation = ibuffer_.GetGpuVirtualAddress();
    ibv.Format = indexFormat_;
    ibv.SizeInBytes = bufferByteSize_;
    return ibv;
}