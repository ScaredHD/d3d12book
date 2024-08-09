#pragma once
#include "Common/d3dUtil.h"
#include "DescriptorHeap.h"

template <typename T>
class ConstantBuffer {
  public:
    ConstantBuffer(ID3D12Device* device, UINT elementCount);

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

    D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const {
        return uploadBuffer_->GetGPUVirtualAddress();
    }

    UINT GetElementByteSize() const { return elementByteSize_; }

    void Load(int elementIndex, const T& data) {
        memcpy(&mappedData_[elementIndex * elementByteSize_], &data, sizeof(T));
    }

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer_;
    BYTE* mappedData_ = nullptr;

    UINT elementByteSize_ = 0;
};

template <typename T>
ConstantBuffer<T>::ConstantBuffer(ID3D12Device* device, UINT elementCount) {
    elementByteSize_ = d3dUtil::CalcConstantBufferByteSize(sizeof(T));

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