#pragma once

#include "MyApp.h"

#include "Common/d3dUtil.h"

// Link necessary d3d12 libraries.
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class MyD3DApp : public MyApp {
  public:
    MyD3DApp(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
        : MyApp(hInstance, hPrevInstance, pCmdLine, nCmdShow) {}

    bool InitializeD3D();

    void Update() {}

    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

  protected:
    void OnMouseDown(int xPos, int yPos) override;

    void OnMouseUp(int xPos, int yPos) override;

    void OnKeyDown() override;

    void OnKeyUp() override;

    void OnResize(int width, int height) override;

  private:
    Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
    Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    UINT rtvDescriptorSize;
    UINT dsvDescriptorSize;
    UINT cbvSrvDescriptorSize;

    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    UINT msaa4xQuality;
};
