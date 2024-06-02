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

    bool InitializeWindow(std::wstring windowName);

    void ShowWindow();

    bool PollEvents();

    virtual LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

  protected:
    virtual void OnMouseDown(int xPos, int yPos) {}

    virtual void OnMouseUp(int xPos, int yPos) {}

    virtual void OnKeyDown() {}

    virtual void OnKeyUp() {}

    virtual void OnResize(int width, int height) {}

    [[nodiscard]] bool IsMouseDown() const { return mouseDown; }

    [[nodiscard]] bool IsKeyDown(char key) const { return GetKeyState(key) & 0x8000; }

    auto GetWindowHandle() const { return hwnd; }

  private:
    // ReSharper disable once IdentifierTypo
    HWND hwnd;

    HINSTANCE hInstance;
    HINSTANCE hPrevInstance;
    PWSTR pCmdLine;
    int nCmdShow;

    bool isRunning;

    bool mouseDown;
};
