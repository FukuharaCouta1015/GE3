#pragma once
#include <Windows.h>
#include <cstdint>

class WinApp {
public:
    static LRESULT CALLBACK WindowProc(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam);

    // クライアント領域のサイズ
    static const int32_t kClinetWidth = 1280; // 幅
    static const int32_t kClineHeigth = 720; // 高さ

    HWND GetHwnd()const { return hwnd; }
    HINSTANCE GetHinstance() const { return wc.hInstance;}

    bool ProcessMessage();

public:
    void Initialize();

    void Update();

    void Finalize();

private:
    HWND hwnd = nullptr;

    WNDCLASS wc {};
};