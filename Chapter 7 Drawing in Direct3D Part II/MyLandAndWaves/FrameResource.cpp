#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT waveVertexCount) {
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                 IID_PPV_ARGS(alloc.ReleaseAndGetAddressOf())));

    passCbuffer = std::make_unique<ConstantBuffer<PassConstant>>(device, passCount);
    objectCbuffer = std::make_unique<ConstantBuffer<ObjectConstant>>(device, objectCount);
    waveVbuffer = std::make_unique<UploadBuffer<Vertex>>(device, waveVertexCount);
}