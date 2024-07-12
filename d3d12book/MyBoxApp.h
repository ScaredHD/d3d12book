#pragma once
#include <WindowsX.h>

#include "MyD3DApp.h"

#include "Common/d3dUtil.h"
#include "Common/UploadBuffer.h"

struct Vertex {
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT4 color;
};

struct ConstantBufferObject {
    DirectX::XMFLOAT4X4 modelViewProj;
};

// template <typename T>
// class ConstantBufferUploader {
//   public:
//     ConstantBufferUploader(ID3D12Device* device, UINT elementCount) {
//         elementByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(T));
//
//         ThrowIfFailed(device->CreateCommittedResource(
//             &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
//             D3D12_HEAP_FLAG_NONE,
//             &CD3DX12_RESOURCE_DESC::Buffer(elementByteSize * elementCount),
//             D3D12_RESOURCE_STATE_GENERIC_READ,
//             nullptr,
//             IID_PPV_ARGS(uploadBuffer.ReleaseAndGetAddressOf())));
//
//         ThrowIfFailed(uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));
//     }
//
//     ConstantBufferUploader(const ConstantBufferUploader& other) = delete;
//     ConstantBufferUploader(ConstantBufferUploader&& other) noexcept = delete;
//
//     ConstantBufferUploader& operator=(const ConstantBufferUploader& other) = delete;
//     ConstantBufferUploader& operator=(ConstantBufferUploader&& other) noexcept = delete;
//
//     ~ConstantBufferUploader() {
//         if (uploadBuffer) {
//             uploadBuffer->Unmap(0, nullptr);
//         }
//         mappedData = nullptr;
//     }
//
//     ID3D12Resource* Resource() const { return uploadBuffer.Get(); }
//
//     void CopyData(int elementIndex, const T& data) {
//         memcpy(&mappedData[elementIndex * elementByteSize], &data, sizeof(T));
//     }
//
//   private:
//     Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
//     BYTE* mappedData{};
//     UINT elementByteSize;
// };

class MyBoxApp final : public MyD3DApp {
  public:
    MyBoxApp(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
        : MyD3DApp(hInstance, hPrevInstance, pCmdLine, nCmdShow) {}

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

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignitature;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cbvHeap;

    std::unique_ptr<UploadBuffer<ConstantBufferObject>> uploader;

    std::unique_ptr<MeshGeometry> boxGeo;

    Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderByteCode;
    Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderByteCode;

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;

    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU{};
    Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader{};

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
