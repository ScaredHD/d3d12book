#ifndef UNICODE
#define UNICODE
#endif

#define DEBUG

#include "BoxApp.h"
#include "MyApp/D3DApp.h"

#pragma comment(lib, "MyApp.lib")

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    auto app = std::make_unique<BoxApp>(hInstance, hPrevInstance, pCmdLine, nCmdShow);

    try {
        app->Initialize();
        app->ShowWindow();

        while (app->PollEvents()) {
            app->Update();
            app->Draw();
        }

    } catch (const DxException& e) {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}