#pragma once
#include <WindowsX.h>

#include "MyApp/D3DApp.h"

#include "Common/d3dUtil.h"
#include "Common/UploadBuffer.h"

struct Vertex {
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT4 color;
};

struct ConstantBufferObject {
    DirectX::XMFLOAT4X4 modelViewProj;
};

class MyBoxApp final : public D3DApp {
  public:
    MyBoxApp(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
        : D3DApp(hInstance, hPrevInstance, pCmdLine, nCmdShow) {}

    void Draw() override;

    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

  private:
    void OnInitialize() override;

    void OnResize() override;

    void OnUpdate() override;

    void OnMouseDown(int xPos, int yPos) override;

    void OnMouseUp(int xPos, int yPos) override;

    void BuildDescriptorHeaps();
    void BuildConstantBuffers();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildBoxGeometry();
    void BuildPSO();

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cbvHeap;

    std::unique_ptr<UploadBuffer<ConstantBufferObject>> uploader;

    std::unique_ptr<MeshGeometry> boxGeo;

    Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderByteCode;
    Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderByteCode;

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;

    struct {
        float radius = 5.0f;
        float phi = DirectX::XM_PIDIV4;
        float theta = 1.5f * DirectX::XM_PI;
    } cameraPose;

    DirectX::XMFLOAT4X4 matModel = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 matView = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 matProjection = MathHelper::Identity4x4();

    int lastMousePosX;
    int lastMousePosY;
};
