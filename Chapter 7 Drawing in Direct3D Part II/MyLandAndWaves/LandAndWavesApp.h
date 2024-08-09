#pragma once

#include "MyApp/ConstantBuffer.h"
#include "MyApp/D3DApp.h"
#include "MyApp/DefaultBuffer.h"
#include "MyApp/DescriptorHeap.h"

struct ObjectConstant {};

struct PassConstant {};

class LandAndWavesApp final : public D3DApp {
  public:
    LandAndWavesApp(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
        : D3DApp(hInstance, hPrevInstance, pCmdLine, nCmdShow) {}

    void OnInitialize() override;

    void OnUpdate() override;

  private:
    void BuildGeometry();
    void BuildConstantBuffer();
    void BuildRootSignature();
    void BuildPso();

    Microsoft::WRL::ComPtr<VertexBuffer> vbuffer_;
    Microsoft::WRL::ComPtr<IndexBuffer> ibuffer_;

    Microsoft::WRL::ComPtr<ConstantBuffer<ObjectConstant>> objectCbuffer_;
    Microsoft::WRL::ComPtr<ConstantBuffer<PassConstant>> passCbuffer_;
    Microsoft::WRL::ComPtr<DescriptorHeap> cbvHeap_;
};
