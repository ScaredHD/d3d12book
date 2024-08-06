#pragma once
#include "Common/d3dUtil.h"

template <typename T>
class Cbuffer {
  public:
    Cbuffer(ID3D12Device* device, UINT elementCount);

    Cbuffer(const Cbuffer& other) = delete;
    Cbuffer(Cbuffer&& other) noexcept = delete;
    Cbuffer& operator=(const Cbuffer& other) = delete;
    Cbuffer& operator=(Cbuffer&& other) noexcept = delete;

    ~Cbuffer() {
        if (uploadBuffer_) {
            uploadBuffer_->Unmap(0, nullptr);
        }

        mappedData_ = nullptr;
    }

    ID3D12Resource* Resource() const { return uploadBuffer_.Get(); }

    void CopyData(int elementIndex, const T& data) {
        memcpy(&mappedData_[elementIndex * elementByteSize_], &data, sizeof(T));
    }

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer_;
    BYTE* mappedData_ = nullptr;

    UINT elementByteSize_ = 0;
};

template <typename T>
Cbuffer<T>::Cbuffer(ID3D12Device* device, UINT elementCount) {
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