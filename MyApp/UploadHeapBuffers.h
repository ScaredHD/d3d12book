#pragma once

#include "Common/d3dUtil.h"
#include "DescriptorHeap.h"

template <typename T>
class UploadBuffer {
  public:
    UploadBuffer(ID3D12Device* device, UINT elementCount) {
        elementByteSize_ = sizeof(T);
        elementCount_ = elementCount;

        auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(elementCount * elementByteSize_);
        ThrowIfFailed(device->CreateCommittedResource(&heapProp,
                                                      D3D12_HEAP_FLAG_NONE,
                                                      &desc,
                                                      D3D12_RESOURCE_STATE_GENERIC_READ,
                                                      nullptr,
                                                      IID_PPV_ARGS(&uploadBuffer_)));

        ThrowIfFailed(uploadBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData_)));
    }

    UploadBuffer(const UploadBuffer& other) = delete;
    UploadBuffer(UploadBuffer&& other) noexcept = delete;
    UploadBuffer& operator=(const UploadBuffer& other) = delete;
    UploadBuffer& operator=(UploadBuffer&& other) noexcept = delete;

    ~UploadBuffer() {
        if (uploadBuffer_) {
            uploadBuffer_->Unmap(0, nullptr);
        }

        mappedData_ = nullptr;
    }

    ID3D12Resource* Resource() const { return uploadBuffer_.Get(); }

    D3D12_GPU_VIRTUAL_ADDRESS GetElementGpuVirtualAddress(int elementIndex) const {
        return uploadBuffer_->GetGPUVirtualAddress() + elementIndex * elementByteSize_;
    }

    UINT GetElementByteSize() const { return elementByteSize_; }

    UINT GetElementCount() const { return elementCount_; }

    UINT GetBufferByteSize() const { return elementCount_ * elementByteSize_; }

    void Load(int elementIndex, const T& data) {
        memcpy(&mappedData_[elementIndex * elementByteSize_], &data, sizeof(T));
    }

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer_;
    BYTE* mappedData_ = nullptr;

    UINT elementByteSize_ = 0;
    UINT elementCount_ = 0;
};

template <typename T>
D3D12_VERTEX_BUFFER_VIEW ViewAsVertexBuffer(const UploadBuffer<T>* uploadBuffer) {
    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = uploadBuffer->GetElementGpuVirtualAddress(0);
    vbv.StrideInBytes = uploadBuffer->GetElementByteSize();
    vbv.SizeInBytes = uploadBuffer->GetBufferByteSize();
    return vbv;
}

template <typename T>
class ConstantBuffer {
  public:
    ConstantBuffer(ID3D12Device* device, UINT elementCount) {
        elementByteSize_ = d3dUtil::CalcConstantBufferByteSize(sizeof(T));
        elementCount_ = elementCount;

        auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(elementByteSize_ * elementCount);
        ThrowIfFailed(device->CreateCommittedResource(&heapProp,
                                                      D3D12_HEAP_FLAG_NONE,
                                                      &desc,
                                                      D3D12_RESOURCE_STATE_GENERIC_READ,
                                                      nullptr,
                                                      IID_PPV_ARGS(&uploadBuffer_)));

        ThrowIfFailed(uploadBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData_)));
    }

    ConstantBuffer(const ConstantBuffer& other) = delete;
    ConstantBuffer(ConstantBuffer&& other) noexcept = delete;
    ConstantBuffer& operator=(const ConstantBuffer& other) = delete;
    ConstantBuffer& operator=(ConstantBuffer&& other) noexcept = delete;

    ~ConstantBuffer() {
        if (uploadBuffer_) {
            uploadBuffer_->Unmap(0, nullptr);
        }

        mappedData_ = nullptr;
    }

    ID3D12Resource* Resource() const { return uploadBuffer_.Get(); }

    D3D12_GPU_VIRTUAL_ADDRESS GetElementGpuVirtualAddress(int elementIndex) const {
        return uploadBuffer_->GetGPUVirtualAddress() + elementIndex * elementByteSize_;
    }

    UINT GetElementByteSize() const { return elementByteSize_; }

    UINT GetElementCount() const { return elementCount_; }

    UINT GetBufferByteSize() const { return elementCount_ * elementByteSize_; }

    void Load(int elementIndex, const T& data) {
        memcpy(&mappedData_[elementIndex * elementByteSize_], &data, sizeof(T));
    }

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer_;
    BYTE* mappedData_ = nullptr;

    UINT elementByteSize_ = 0;
    UINT elementCount_ = 0;
};

template <typename T>
void CreateConstantBufferViewOnHeap(ID3D12Device* device,
                                    ConstantBuffer<T>* cbuffer,
                                    UINT cbufferIndex,
                                    DescriptorHeap* heap,
                                    UINT heapIndex) {
    auto addressGpu = cbuffer->GetGpuVirtualAddress();
    addressGpu += cbufferIndex * cbuffer->GetElementByteSize();

    D3D12_CONSTANT_BUFFER_VIEW_DESC desc{};
    desc.BufferLocation = addressGpu;
    desc.SizeInBytes = cbuffer->GetElementByteSize();

    device->CreateConstantBufferView(&desc, heap->GetDescriptorHandleCpu(heapIndex));
}