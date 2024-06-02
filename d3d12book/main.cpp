#ifndef UNICODE
#define UNICODE
#endif

#include <stdexcept>
#include <iostream>

#include "MyD3DApp.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    MyD3DApp d3dApp(hInstance, hPrevInstance, pCmdLine, nCmdShow);

    if (!d3dApp.InitializeWindow(L"My D3D App")) {
        throw std::runtime_error("Failed to initialize window");
    }

    d3dApp.ShowWindow();

    while (d3dApp.PollEvents()) {
        d3dApp.Update();
    }
}