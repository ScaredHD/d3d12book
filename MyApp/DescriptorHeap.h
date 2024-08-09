#pragma once

#include "Common/d3dUtil.h"

class DescriptorHeap {
  public:
    explicit DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_DESC desc);

    [[nodiscard]]
    ID3D12DescriptorHeap* Get() const {
        return heap_.Get();
    }

    [[nodiscard]]
    UINT GetDescriptorSize() const {
        return descriptorSize_;
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandleCpu(int index) {
        return {heap_->GetCPUDescriptorHandleForHeapStart(), index, descriptorSize_};
    }

    CD3DX12_GPU_DESCRIPTOR_HANDLE GetDescriptorHandleGpu(int index) {
        return {heap_->GetGPUDescriptorHandleForHeapStart(), index, descriptorSize_};
    }

  private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;

    UINT descriptorSize_;
};