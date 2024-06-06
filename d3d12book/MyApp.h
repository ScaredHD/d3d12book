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

  protected:
    virtual void OnMouseDown(int xPos, int yPos) {}

    virtual void OnMouseUp(int xPos, int yPos) {}

    virtual void OnMouseMove(int xPos, int yPos) {}

    virtual void OnKeyDown() {}

    virtual void OnKeyUp() {}

    virtual void OnResize() {}

    [[nodiscard]] bool IsMouseDown() const { return mouseDown; }

    [[nodiscard]] bool IsKeyDown(char key) const { return GetKeyState(key) & 0x8000; }

    auto GetWindowHandle() const { return hwnd; }

    auto GetWindowName() const { return windowName; }

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

    bool mouseDown;
};
