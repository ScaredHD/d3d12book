#pragma once

#include <windows.h>

#include <xstring>

class MyApp {
  public:
    virtual ~MyApp() = default;

    explicit MyApp(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
        : hInstance{hInstance},
          hPrevInstance{hPrevInstance},
          pCmdLine{pCmdLine},
          nCmdShow{nCmdShow} {}

    bool InitializeWindow(const std::wstring &windowName);

    void ShowWindow();

    bool PollEvents();

    virtual LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    unsigned int GetClientWidth() const { return clientWidth; }

    unsigned int GetClientHeight() const { return clientHeight; }

    float GetAspectRatio() const {
        return static_cast<float>(clientWidth) / static_cast<float>(clientHeight);
    }

  protected:
    virtual void OnLMouseDown() {}

    virtual void OnLMouseUp() {}

    virtual void OnRMouseDown() {}

    virtual void OnRMouseUp() {}

    virtual void OnMouseMove() {}

    virtual void OnKeyDown() {}

    virtual void OnKeyUp() {}

    virtual void OnResize() {}

    [[nodiscard]] static bool IsKeyDown(char key) { return GetKeyState(key) & 0x8000; }

    auto GetWindow() const { return hwnd; }

    auto GetWindowName() const { return windowName; }

    POINT GetMousePos() const { return mousePos_; }

    bool IsLMouseDown() const { return lMouseDown_; }

    bool IsRMouseDown() const { return rMouseDown_; }

  private:
    // ReSharper disable once IdentifierTypo
    HWND hwnd;
    std::wstring windowName;

    UINT clientWidth = 800;
    UINT clientHeight = 600;

    HINSTANCE hInstance;
    HINSTANCE hPrevInstance;
    PWSTR pCmdLine;
    int nCmdShow;

    bool isRunning;

    bool lMouseDown_;
    bool rMouseDown_;
    POINT mousePos_;
};
