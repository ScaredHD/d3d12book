#include "MyApp.h"

#include <WindowsX.h>

#include <string>

// ReSharper disable once CppParameterMayBeConst
LRESULT MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MyApp* hwndOwner = nullptr;

    if (msg == WM_NCCREATE) {
        void* p = reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(p));
    } else {
        LONG_PTR p = GetWindowLongPtr(hwnd, GWLP_USERDATA);
        hwndOwner = reinterpret_cast<MyApp*>(p);
    }

    if (hwndOwner) {
        return hwndOwner->HandleMessage(hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool MyApp::InitializeWindow(std::wstring windowName) {
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = L"MainWnd";

    RegisterClass(&wc);

    hwnd = CreateWindowEx(0,                    // Optional window styles.
                          L"MainWnd",           // Window class
                          windowName.c_str(),   // Window text
                          WS_OVERLAPPEDWINDOW,  // Window style

                          // Size and position
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,

                          0,          // Parent window
                          0,          // Menu
                          hInstance,  // Instance handle
                          this        // Additional application data
    );

    isRunning = (hwnd != nullptr);

    return isRunning;
}

void MyApp::ShowWindow() {
    ::ShowWindow(hwnd, nCmdShow);
}

bool MyApp::PollEvents() {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return isRunning;
}

LRESULT MyApp::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            isRunning = false;
            return 0;

        case WM_SIZE:
            OnResize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_LBUTTONDOWN:
            mouseDown = true;
            OnMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_LBUTTONUP:
            mouseDown = false;
            OnMouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_KEYDOWN:
            OnKeyDown();
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
