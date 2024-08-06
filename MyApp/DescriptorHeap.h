#pragma once

#include "Common/d3dUtil.h"

class DescriptorHeap {
  public:
    explicit DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_DESC desc);

    UINT GetDescriptorSize() const { return descriptorSize_; }

    CD3DX12_CPU_DESCRIPTOR_HANDLE GetDescriptor(int index) {
        return {heap_->GetCPUDescriptorHandleForHeapStart(), index, descriptorSize_};
    }

  private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;

    UINT descriptorSize_;
};