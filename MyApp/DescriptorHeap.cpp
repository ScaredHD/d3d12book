#include "DescriptorHeap.h"

DescriptorHeap::DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_DESC desc) {
    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(heap_.GetAddressOf())));
    descriptorSize_ = device->GetDescriptorHandleIncrementSize(desc.Type);
}