#pragma once
#include "FrameResource.h"
#include "MyApp/D3DApp.h"
#include "MyApp/DefaultHeapBuffers.h"
#include "RenderItem.h"
#include "WavesGeometry.h"

class TexCrate final : public D3DApp {
public:
  static constexpr int s_frameResourceCount = 3;

  TexCrate(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
      : D3DApp(hInstance, hPrevInstance, pCmdLine, nCmdShow) {};

  void OnKeyDown() override;

  void OnMouseMove() override;

  void OnInitialize() override;
  void WaterTextureAnimation();

  void OnUpdate() override;

  void Draw() override;

  void BuildLandGeometry();

  void BuildWavesGeometry();

  void BuildCrateGeometry();

  void LoadTexture();

  void DrawAllRenderItems();

  void BuildMaterials();

private:
  std::unique_ptr<WavesGeometry> waves_;
  std::unique_ptr<IndexBuffer> wavesIbuffer_;

  std::unique_ptr<VertexBuffer> landVbuffer_;
  std::unique_ptr<IndexBuffer> landIbuffer_;

  std::array<std::unique_ptr<FrameResource>, s_frameResourceCount> frameResources_;
  int currentFrameResourceIndex_ = 0;
  FrameResource* currentFrameResource_{};

  std::vector<RenderItem> renderItems_;
  RenderItem waveRenderItem_;

  std::unique_ptr<VertexBuffer> crateVbuffer_;
  std::unique_ptr<IndexBuffer> crateIbuffer_;
  RenderItem crateRenderItem_;

  std::unique_ptr<Texture> crateTex_ = nullptr;
  std::unique_ptr<Texture> grassTex_ = nullptr;
  std::unique_ptr<Texture> stoneTex_ = nullptr;
  std::unique_ptr<Texture> waterTex_ = nullptr;
  std::unique_ptr<DescriptorHeap> srvHeap_ = nullptr;

  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;

  Microsoft::WRL::ComPtr<ID3DBlob> vertexShader_;
  Microsoft::WRL::ComPtr<ID3DBlob> pixelShader_;

  std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout_;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> psoWireframe_;

  std::unordered_map<std::string, std::unique_ptr<Material>> materials_;

  // std::unique_ptr<DescriptorHeap> samplerHeap_;

  static std::array<CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

  // Camera location in spherical coordinates
  float radius_ = 50.0f;
  float theta_ = 0.0f;
  float phi_ = MathHelper::Pi / 4.0f;

  POINT prevMousePos_;
  bool wireframe_ = false;

  float sunTheta_ = 0.0f;
  float sunPhi_ = MathHelper::Pi / 4.0f;
};
