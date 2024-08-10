#pragma once

#include "D3DApp.h"

class DefaultBuffer {
  public:
    DefaultBuffer() = default;
    DefaultBuffer(ID3D12Device* device,
                  ID3D12GraphicsCommandList* commandList,
                  const void* initData,
                  unsigned int byteSize);

    void Load(ID3D12Device* device,
              ID3D12GraphicsCommandList* commandList,
              const void* data,
              unsigned int byteSize);

    [[nodiscard]]
    D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const;

    void ResetUploader();

  private:
    Microsoft::WRL::ComPtr<ID3DBlob> bufferCpu_;
    Microsoft::WRL::ComPtr<ID3D12Resource> bufferGpu_;
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer_;
};

class VertexBuffer {
  public:
    VertexBuffer(UINT strideInByte, UINT byteSize) : strideInByte_(strideInByte), byteSize_(byteSize) {}

    void Load(ID3D12Device* device,
              ID3D12GraphicsCommandList* commandList,
              const void* data,
              unsigned int byteSize);

    [[nodiscard]]
    D3D12_VERTEX_BUFFER_VIEW GetView() const;

  private:
    DefaultBuffer vbuffer_;

    UINT strideInByte_ = 0;
    UINT byteSize_ = 0;
};

class IndexBuffer {
  public:
    IndexBuffer(DXGI_FORMAT indexFormat, UINT byteSize)
        : indexFormat_(indexFormat),
          byteSize_(byteSize) {}

    void Load(ID3D12Device* device,
              ID3D12GraphicsCommandList* commandList,
              const void* data,
              unsigned int byteSize);

    [[nodiscard]]
    D3D12_INDEX_BUFFER_VIEW GetView() const;

  private:
    DefaultBuffer ibuffer_;

    DXGI_FORMAT indexFormat_ = DXGI_FORMAT_R16_UINT;
    UINT byteSize_ = 0;
};