#ifndef UNICODE
#define UNICODE
#endif

#include <iostream>
#include <stdexcept>

#include "MyD3DApp.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    MyD3DApp d3dApp(hInstance, hPrevInstance, pCmdLine, nCmdShow);

    try {
        if (!d3dApp.InitializeWindow(L"My D3D App")) {
            throw std::runtime_error("Failed to initialize window");
        }

        if (!d3dApp.InitializeD3D()) {
            throw std::runtime_error("Failed to initialize D3D");
        }

        d3dApp.ShowWindow();

        while (d3dApp.PollEvents()) {
            d3dApp.Update();
        }

    } catch (const DxException& e) {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}