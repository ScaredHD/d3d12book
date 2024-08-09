#pragma once
#include "d3d12book/MyD3DApp.h"
#include "Frame.h"
#include "MyApp/D3DApp.h"

// Groups things needed to draw an object.
struct RenderItem {
    RenderItem() = default;

    DirectX::XMFLOAT4X4 modelToWorld = MathHelper::Identity4x4();

    int dirtyFrameResourceCount = 3;

    UINT CBufferElementIndex = -1;

    MeshGeometry* geo = nullptr;

    D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    UINT indexCount = 0;
    UINT indexStart = 0;
    int baseVertex = 0;
};

class ShapeApp : public D3DApp {
  public:
    ShapeApp(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
        : D3DApp(hInstance, hPrevInstance, pCmdLine, nCmdShow) {}

    void OnInitialize() override;

    void OnUpdate() override;

    void Draw() override;

  private:
    void BuildGeometry();

    void BuildFrameResources();

    void BuildRenderItems();

    void CreateCbvDescriptorHeap();

    void CreateCbufferViews();

    void UpdateObjectCBuffers();

    void UpdateMainPassCBuffer();

    void DrawRenderItems();

    int FrameResourceCount() const { return frameResources_.size(); }

    size_t OpaqueRenderItemCount() const { return opaqueRenderItems_.size(); }

    static constexpr int s_frameResourceCount = 3;
    std::vector<std::unique_ptr<FrameResource>> frameResources_;
    int currentFrameResourceIndex_ = 0;
    FrameResource* currentFrameResource_;

    std::vector<std::unique_ptr<RenderItem>> renderItems_;
    // categorized by visibility
    std::vector<RenderItem*> opaqueRenderItems_;
    std::vector<RenderItem*> transparentRenderItems_;

    DirectX::XMFLOAT3 eyePos_ = {0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT4X4 view_ = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 proj_ = MathHelper::Identity4x4();

    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> geometries_;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>>
        pipelineStateObjects_;

    PassConstant mainPassCBuffer_;
    UINT passCbvStartIndex_;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cbvHeap_;

    bool wireFrameMode_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
};
