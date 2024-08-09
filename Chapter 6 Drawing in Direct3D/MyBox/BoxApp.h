#pragma once
#include <WindowsX.h>

#include "Common/d3dUtil.h"
#include "ConstantBuffer.h"
#include "DefaultBuffer.h"
#include "MyApp/D3DApp.h"

struct Vertex {
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT4 color;
};

struct ConstantBufferObject {
    DirectX::XMFLOAT4X4 modelViewProj;
};

class BoxApp final : public D3DApp {
  public:
    BoxApp(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
        : D3DApp(hInstance, hPrevInstance, pCmdLine, nCmdShow) {}

    void Draw() override;

    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

  private:
    void OnInitialize() override;

    void OnResize() override;

    void OnUpdate() override;

    void OnMouseDown(int xPos, int yPos) override;

    void OnMouseUp(int xPos, int yPos) override;

    void BuildCbvHeap();
    void BuildConstantBuffers();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildBoxGeometry();
    void BuildPso();

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;

    std::unique_ptr<ConstantBuffer<ConstantBufferObject>> cbuffer_;
    std::unique_ptr<DescriptorHeap> cbvHeap_;

    std::unique_ptr<SubmeshGeometry> boxSubmesh_;
    std::unique_ptr<VertexBuffer> vbuffer_;
    std::unique_ptr<IndexBuffer> ibuffer_;

    Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderByteCode_;
    Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderByteCode_;

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout_;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;

    struct {
        float radius = 5.0f;
        float phi = DirectX::XM_PIDIV4;
        float theta = 1.5f * DirectX::XM_PI;
    } cameraPose_;

    DirectX::XMFLOAT4X4 matModel_ = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 matView_ = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 matProjection_ = MathHelper::Identity4x4();

    int lastMousePosX_ = 0;
    int lastMousePosY_ = 0;
};
