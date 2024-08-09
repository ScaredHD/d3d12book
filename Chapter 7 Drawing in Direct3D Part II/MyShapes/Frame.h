#pragma once

#include "Common/d3dUtil.h"
#include "Common/My/ConstantBuffer.h"

struct PassConstant {
    DirectX::XMFLOAT4X4 view = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 invView = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 proj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 invProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 viewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 invViewProj = MathHelper::Identity4x4();

    DirectX::XMFLOAT3 eyePos = {0.0f, 0.0f, 0.0f};
    float cbPerObjectPad1 = 0.0f;
    DirectX::XMFLOAT2 renderTargetSize = {0.0f, 0.0f};
    DirectX::XMFLOAT2 invRenderTargetSize = {0.0f, 0.0f};

    float nearZ = 0.0f;
    float farZ = 0.0f;
    float totalTime = 0.0f;
    float deltaTime = 0.0f;
};

struct ObjectConstant {
    DirectX::XMFLOAT4X4 modelToWorld;
};

struct Vertex {
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT4 color;
};

struct FrameResource {
    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount);

    FrameResource(const FrameResource& other) = delete;
    FrameResource(FrameResource&& other) noexcept = delete;
    FrameResource& operator=(const FrameResource& other) = delete;
    FrameResource& operator=(FrameResource&& other) noexcept = delete;

    ~FrameResource() = default;

    // frame resources that will be modified every frame
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;

    std::unique_ptr<ConstantBuffer<PassConstant>> passCBuffer;
    std::unique_ptr<ConstantBuffer<ObjectConstant>> objectCBuffer;


    UINT64 fence = 0;
};
