#include "LitWaves.h"

#include "Common/GeometryGenerator.h"

using namespace DirectX;
using namespace Microsoft::WRL;

const int gNumFrameResources = 3;

void LandAndWaves::OnKeyDown() {
  if (IsKeyDown('W')) {
    wireframe_ = !wireframe_;
  }
}

void LandAndWaves::OnMouseMove() {
  auto curPos = GetMousePos();
  if (IsLMouseDown()) {
    float dx = XMConvertToRadians(0.25f * static_cast<float>(curPos.x - prevMousePos_.x));
    float dy = XMConvertToRadians(0.25f * static_cast<float>(curPos.y - prevMousePos_.y));

    theta_ -= dx;
    phi_ = MathHelper::Clamp(phi_ - dy, 0.1f, MathHelper::Pi - 0.1f);

  } else if (IsRMouseDown()) {
    float dx = 0.2f * static_cast<float>(curPos.x - prevMousePos_.x);
    float dy = 0.2f * static_cast<float>(curPos.y - prevMousePos_.y);
    radius_ += dx - dy;
    radius_ = MathHelper::Clamp(radius_, 5.0f, 150.f);
  }

  prevMousePos_ = curPos;
  D3DApp::OnMouseMove();
}

void LandAndWaves::OnInitialize() {
  ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), nullptr));

  BuildMaterials();
  BuildLandGeometry();
  BuildWavesGeometry();

  // Build frame resources
  for (auto& frameRes : frameResources_) {
    frameRes = std::make_unique<FrameResource>(device_.Get(),
                                               1,
                                               renderItems_.size(),
                                               waves_->VertexCount(),
                                               materials_.size());
  }

  // Build root signature
  CD3DX12_ROOT_PARAMETER rootParams[3];
  rootParams[0].InitAsConstantBufferView(0);
  rootParams[1].InitAsConstantBufferView(1);
  rootParams[2].InitAsConstantBufferView(2);

  CD3DX12_ROOT_SIGNATURE_DESC desc = {3,
                                      rootParams,
                                      0,
                                      nullptr,
                                      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};

  ComPtr<ID3DBlob> serializedRootSignature;
  ComPtr<ID3DBlob> error;
  auto hr = D3D12SerializeRootSignature(&desc,
                                        D3D_ROOT_SIGNATURE_VERSION_1,
                                        serializedRootSignature.GetAddressOf(),
                                        error.GetAddressOf());
  if (error) {
    ::OutputDebugStringA(static_cast<LPCSTR>(error->GetBufferPointer()));
  }
  ThrowIfFailed(hr);

  ThrowIfFailed(device_->CreateRootSignature(0,
                                             serializedRootSignature->GetBufferPointer(),
                                             serializedRootSignature->GetBufferSize(),
                                             IID_PPV_ARGS(rootSignature_.GetAddressOf())));

  // Build shaders
  vertexShader_ = d3dUtil::CompileShader(L"shaders.hlsl", nullptr, "VS", "vs_5_0");
  pixelShader_ = d3dUtil::CompileShader(L"shaders.hlsl", nullptr, "PS", "ps_5_0");

  // Input layout
  inputLayout_ = {{"POSITION",
                   0,
                   DXGI_FORMAT_R32G32B32_FLOAT,
                   0,
                   0,
                   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                   0},
                  {"NORMAL",
                   0,
                   DXGI_FORMAT_R32G32B32_FLOAT,
                   0,
                   12,
                   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                   0}};

  // Build pipeline state object
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
  psoDesc.InputLayout = {inputLayout_.data(), static_cast<UINT>(inputLayout_.size())};
  psoDesc.pRootSignature = rootSignature_.Get();
  psoDesc.VS = {static_cast<BYTE*>(vertexShader_->GetBufferPointer()),
                vertexShader_->GetBufferSize()};
  psoDesc.PS = {static_cast<BYTE*>(pixelShader_->GetBufferPointer()),
                pixelShader_->GetBufferSize()};
  psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  psoDesc.SampleMask = UINT_MAX;
  psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.NumRenderTargets = 1;
  psoDesc.RTVFormats[0] = backBufferFormat_;
  psoDesc.SampleDesc.Count = msaa4XEnabled_ ? 4 : 1;
  psoDesc.SampleDesc.Quality = msaa4XEnabled_ ? (msaa4XQuality_ - 1) : 0;
  psoDesc.DSVFormat = depthStencilFormat_;

  ThrowIfFailed(device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(pso_.GetAddressOf())));

  auto psoWireframeDesc = psoDesc;
  psoWireframeDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
  ThrowIfFailed(device_->CreateGraphicsPipelineState(&psoWireframeDesc,
                                                     IID_PPV_ARGS(psoWireframe_.GetAddressOf())));

  ThrowIfFailed(commandList_->Close());
  ExecuteCommandList();
  FlushCommandQueue();
}

void LandAndWaves::OnUpdate() {
  // Find next available slot in frame resource array.
  // If all slots have been filled by CPU and not yet used by GPU, let CPU wait.
  currentFrameResourceIndex_ = (currentFrameResourceIndex_ + 1) % s_frameResourceCount;
  currentFrameResource_ = frameResources_[currentFrameResourceIndex_].get();

  if (UINT64 frameFence = currentFrameResource_->fence;
      frameFence != 0 && fence_->GetCompletedValue() < frameFence) {
    HANDLE event = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
    ThrowIfFailed(fence_->SetEventOnCompletion(frameFence, event));
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);
  }

  // Update pass constant buffer
  auto x = radius_ * sinf(phi_) * cosf(theta_);
  auto y = radius_ * cosf(phi_);
  auto z = radius_ * sinf(phi_) * sinf(theta_);

  XMVECTOR eyePos = XMVectorSet(x, y, z, 1.0f);
  XMVECTOR target = XMVectorZero();
  XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

  XMMATRIX view = XMMatrixLookAtLH(eyePos, target, up);
  auto detView = XMMatrixDeterminant(view);
  auto invView = XMMatrixInverse(&detView, view);

  XMMATRIX proj = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
  auto detProj = XMMatrixDeterminant(proj);
  auto invProj = XMMatrixInverse(&detProj, proj);

  PassConstant passConst;
  XMStoreFloat4x4(&passConst.view, view);
  XMStoreFloat4x4(&passConst.invView, invView);
  XMStoreFloat4x4(&passConst.proj, proj);
  XMStoreFloat4x4(&passConst.invProj, invProj);
  passConst.eyePos = {x, y, z};
  passConst.renderTargetSize = XMFLOAT2{static_cast<float>(GetClientWidth()),
                                        static_cast<float>(GetClientHeight())};
  passConst.invRenderTargetSize = XMFLOAT2{1.0f / GetClientWidth(), 1.0f / GetClientHeight()};
  passConst.zNear = 1.0f;
  passConst.zFar = 1000.0f;
  passConst.totalTime = timer_.TotalTimeFromStart();
  passConst.deltaTime = timer_.DeltaTimeSecond();
  passConst.ambientLight = {0.25f, 0.25f, 0.35f, 1.0f};

  XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, sunTheta_, sunPhi_);
  XMStoreFloat3(&passConst.lights[0].Direction, lightDir);
  passConst.lights[0].Strength = {0.8f, 0.8f, 0.7f};

  currentFrameResource_->passCbuffer->Load(0, passConst);

  // Update object constant buffer
  for (auto& item : renderItems_) {
    if (item.dirtyFrameCount > 0) {
      auto model = XMLoadFloat4x4(&item.modelToWorld);
      ObjectConstant objConst;
      XMStoreFloat4x4(&objConst.model, model);
      currentFrameResource_->objectCbuffer->Load(item.objectCbufferIndex, objConst);
      --item.dirtyFrameCount;
    }
  }

  // Update material constant buffer
  for (auto& mat : materials_) {
    Material* m = mat.second.get();
    if (m->NumFramesDirty > 0) {
      MaterialConstants c;
      c.DiffuseAlbedo = m->DiffuseAlbedo;
      c.FresnelR0 = m->FresnelR0;
      c.Roughness = m->Roughness;
      // MatTransform not used in this chapter

      currentFrameResource_->materialCbuffer->Load(m->MatCBIndex, c);
      --m->NumFramesDirty;
    }
  }

  // Update wave vertex buffer
  waves_->Update(timer_.TotalTimeFromStart(), timer_.DeltaTimeSecond());
  Vertex* vertices = waves_->VertexData();
  for (int i = 0; i < waves_->VertexCount(); ++i) {
    currentFrameResource_->waveVbuffer->Load(i, vertices[i]);
  }
}

void LandAndWaves::Draw() {
  auto curAlloc = currentFrameResource_->alloc;
  ThrowIfFailed(curAlloc->Reset());

  auto* pso = wireframe_ ? psoWireframe_.Get() : pso_.Get();
  ThrowIfFailed(commandList_->Reset(curAlloc.Get(), pso));

  SetViewportAndScissorRects();

  Transition(swapChain_->GetCurrentBackBuffer(),
             D3D12_RESOURCE_STATE_PRESENT,
             D3D12_RESOURCE_STATE_RENDER_TARGET);

  auto rtv = swapChain_->GetCurrentBackBufferView();
  commandList_->ClearRenderTargetView(rtv, Colors::LightSteelBlue, 0, nullptr);

  auto dsv = dsvHeap_->GetDescriptorHandleCpu(0);
  commandList_->ClearDepthStencilView(dsv,
                                      D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                      1.0f,
                                      0,
                                      0,
                                      nullptr);

  commandList_->OMSetRenderTargets(1, &rtv, true, &dsv);

  commandList_->SetGraphicsRootSignature(rootSignature_.Get());

  // Bind pass constant buffer
  commandList_->SetGraphicsRootConstantBufferView(
      2,
      currentFrameResource_->passCbuffer->GetElementGpuVirtualAddress(0));

  DrawAllRenderItems();

  Transition(swapChain_->GetCurrentBackBuffer(),
             D3D12_RESOURCE_STATE_RENDER_TARGET,
             D3D12_RESOURCE_STATE_PRESENT);

  ThrowIfFailed(commandList_->Close());
  ExecuteCommandList();

  ThrowIfFailed(swapChain_->Present());
  swapChain_->SwapBuffers();

  currentFrameResource_->fence = ++nextFenceValue_;
  commandQueue_->Signal(fence_.Get(), nextFenceValue_);
}

void LandAndWaves::BuildLandGeometry() {
  auto gridMesh = GeometryGenerator{}.CreateGrid(160.0f, 160.0f, 50, 50);

  auto height = [](float x, float z) { return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z)); };
  
  auto normal = [](float x, float z) {
    // n = (-df/dx, 1, -df/dz)
    XMFLOAT3 n(-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
               1.0f,
               -0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

    XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
    XMStoreFloat3(&n, unitNormal);

    return n;
  };

  // offset vertices on y-axis and assign colors
  std::vector<Vertex> vertices;
  for (const auto& v : gridMesh.Vertices) {
    auto p = v.Position;
    p.y = height(p.x, p.z);
    vertices.push_back(Vertex{p, normal(p.x, p.z)});
  }

  UINT vbByteSize = vertices.size() * sizeof(Vertex);
  landVbuffer_ = std::make_unique<VertexBuffer>(sizeof(Vertex), vbByteSize);
  landVbuffer_->Load(device_.Get(), commandList_.Get(), vertices.data(), vbByteSize);

  std::vector<std::uint16_t> indices = gridMesh.GetIndices16();

  UINT ibByteSize = indices.size() * sizeof(std::uint16_t);
  landIbuffer_ = std::make_unique<IndexBuffer>(DXGI_FORMAT_R16_UINT, ibByteSize);
  landIbuffer_->Load(device_.Get(), commandList_.Get(), indices.data(), ibByteSize);

  RenderItem land;
  land.indexCount = indices.size();
  land.indexStart = 0;
  land.baseVertex = 0;
  land.objectCbufferIndex = 0;
  land.modelToWorld = MathHelper::Identity4x4();
  land.mat = materials_["grass"].get();
  renderItems_.push_back(std::move(land));
}

void LandAndWaves::BuildWavesGeometry() {
  waves_ = std::make_unique<WavesGeometry>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

  UINT ibByteSize = waves_->IndexCount() * sizeof(std::uint16_t);
  wavesIbuffer_ = std::make_unique<IndexBuffer>(DXGI_FORMAT_R16_UINT, ibByteSize);
  wavesIbuffer_->Load(device_.Get(), commandList_.Get(), waves_->IndexData(), ibByteSize);

  waveRenderItem_.indexCount = waves_->IndexCount();
  waveRenderItem_.indexStart = 0;
  waveRenderItem_.baseVertex = 0;
  waveRenderItem_.objectCbufferIndex = 0;
  waveRenderItem_.modelToWorld = MathHelper::Identity4x4();
  waveRenderItem_.mat = materials_["water"].get();
}

void LandAndWaves::DrawAllRenderItems() {
  auto* objectCbuffer = currentFrameResource_->objectCbuffer.get();
  auto* matCbuffer = currentFrameResource_->materialCbuffer.get();

  // Draw land
  for (const auto& item : renderItems_) {
    // bind VBV and IBV
    auto vbv = landVbuffer_->GetView();
    commandList_->IASetVertexBuffers(0, 1, &vbv);

    auto ibv = landIbuffer_->GetView();
    commandList_->IASetIndexBuffer(&ibv);

    // bind CBV to object constant buffer
    auto objCbufferAddress = objectCbuffer->GetElementGpuVirtualAddress(item.objectCbufferIndex);
    commandList_->SetGraphicsRootConstantBufferView(0, objCbufferAddress);
    commandList_->IASetPrimitiveTopology(item.primitiveType);

    auto matCbufferAddress = matCbuffer->GetElementGpuVirtualAddress(item.mat->MatCBIndex);
    commandList_->SetGraphicsRootConstantBufferView(1, matCbufferAddress);

    commandList_->DrawIndexedInstanced(item.indexCount, 1, item.indexStart, item.baseVertex, 0);
  }

  // Draw waves
  auto* waveVbuffer = currentFrameResource_->waveVbuffer.get();
  D3D12_VERTEX_BUFFER_VIEW vbv = ViewAsVertexBuffer(waveVbuffer);
  commandList_->IASetVertexBuffers(0, 1, &vbv);

  auto ibv = wavesIbuffer_->GetView();
  commandList_->IASetIndexBuffer(&ibv);

  auto address = objectCbuffer->GetElementGpuVirtualAddress(waveRenderItem_.objectCbufferIndex);
  auto matAddress = matCbuffer->GetElementGpuVirtualAddress(waveRenderItem_.mat->MatCBIndex);
  commandList_->SetGraphicsRootConstantBufferView(0, address);
  commandList_->SetGraphicsRootConstantBufferView(1, matAddress);

  commandList_->IASetPrimitiveTopology(waveRenderItem_.primitiveType);

  commandList_->DrawIndexedInstanced(waveRenderItem_.indexCount,
                                     1,
                                     waveRenderItem_.indexStart,
                                     waveRenderItem_.baseVertex,
                                     0);
}

void LandAndWaves::BuildMaterials() {
  auto grass = std::make_unique<Material>();
  grass->Name = "grass";
  grass->MatCBIndex = 0;
  grass->DiffuseAlbedo = XMFLOAT4(0.2f, 0.6f, 0.6f, 1.0f);
  grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
  grass->Roughness = 0.125f;

  auto water = std::make_unique<Material>();
  water->Name = "water";
  water->MatCBIndex = 1;
  water->DiffuseAlbedo = XMFLOAT4(0.0f, 0.2f, 0.6f, 1.0f);
  water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
  water->Roughness = 0.0f;

  materials_["grass"] = std::move(grass);
  materials_["water"] = std::move(water);
}
