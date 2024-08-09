#include "Frame.h"

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount) {
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                 IID_PPV_ARGS(commandAllocator.GetAddressOf())));

    passCBuffer = std::make_unique<ConstantBuffer<PassConstant>>(device, passCount);
    objectCBuffer = std::make_unique<ConstantBuffer<ObjectConstant>>(device, objectCount);
}

