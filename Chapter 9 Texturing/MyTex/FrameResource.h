#pragma once
#include <memory>

#include "MyApp/UploadHeapBuffers.h"

struct Vertex {
  DirectX::XMFLOAT3 pos;
  DirectX::XMFLOAT3 normal;
  DirectX::XMFLOAT2 texCoord;
};

struct ObjectConstant {
  DirectX::XMFLOAT4X4 model;
  DirectX::XMFLOAT4X4 texTransform = MathHelper::Identity4x4();
};

struct PassConstant {
  DirectX::XMFLOAT4X4 view;
  DirectX::XMFLOAT4X4 invView;
  DirectX::XMFLOAT4X4 proj;
  DirectX::XMFLOAT4X4 invProj;

  DirectX::XMFLOAT3 eyePos;
  float pad1 = 0.0f;
  DirectX::XMFLOAT2 renderTargetSize = {0.0f, 0.0f};
  DirectX::XMFLOAT2 invRenderTargetSize = {0.0f, 0.0f};

  float zNear = 0.0f;
  float zFar = 0.0f;
  float totalTime = 0.0f;
  float deltaTime = 0.0f;
  DirectX::XMFLOAT4 ambientLight = {0.0f, 0.0f, 0.0f, 1.0f};
  Light lights[16];
};

struct FrameResource {
  FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT waveVertexCount, UINT materialCount);

  FrameResource(const FrameResource& other) = delete;
  FrameResource(FrameResource&& other) noexcept = delete;
  FrameResource& operator=(const FrameResource& other) = delete;
  FrameResource& operator=(FrameResource&& other) noexcept = delete;
  ~FrameResource() = default;

  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> alloc;

  std::unique_ptr<ConstantBuffer<PassConstant>> passCbuffer;
  std::unique_ptr<ConstantBuffer<ObjectConstant>> objectCbuffer;
  std::unique_ptr<ConstantBuffer<MaterialConstants>> materialCbuffer;

  std::unique_ptr<UploadBuffer<Vertex>> waveVbuffer;

  UINT64 fence = 0;
};
