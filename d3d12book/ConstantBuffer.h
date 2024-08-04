#pragma once
#include "Common/d3dUtil.h"

template <typename T>
class CBuffer {
  public:
    CBuffer(ID3D12Device* device, UINT elementCount);

    CBuffer(const CBuffer& other) = delete;
    CBuffer(CBuffer&& other) noexcept = delete;
    CBuffer& operator=(const CBuffer& other) = delete;
    CBuffer& operator=(CBuffer&& other) noexcept = delete;

    ~CBuffer() {
        if (uploadBuffer) {
            uploadBuffer->Unmap(0, nullptr);
        }

        mappedData = nullptr;
    }

    ID3D12Resource* Resource() const { return uploadBuffer.Get(); }

    void CopyData(int elementIndex, const T& data) {
        memcpy(&mappedData[elementIndex * elementByteSize], &data, sizeof(T));
    }

  private:
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
    BYTE* mappedData = nullptr;

    UINT elementByteSize = 0;
};

template <typename T>
CBuffer<T>::CBuffer(ID3D12Device* device, UINT elementCount) {
    elementByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(T));

    auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto desc = CD3DX12_RESOURCE_DESC::Buffer(elementByteSize * elementCount);
    ThrowIfFailed(device->CreateCommittedResource(&heapProp,
                                                  D3D12_HEAP_FLAG_NONE,
                                                  &desc,
                                                  D3D12_RESOURCE_STATE_GENERIC_READ,
                                                  nullptr,
                                                  IID_PPV_ARGS(&uploadBuffer)));

    ThrowIfFailed(uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));
}