#pragma once
#include <Windows.h>
#include <wrl.h>

#define DIRECTINPUT_VERSION 0x0800

#include <dinput.h>


class Input {
public:
    void Update();
    void Initialize(HINSTANCE hInstance, HWND hwnd);
   
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    bool PushKey(BYTE keyNumber);
    bool TriggerKey(BYTE keyNumber);

    private:
       
        ComPtr<IDirectInputDevice8> keyboard;
        BYTE key[256] = {};
        BYTE keyPre[256] = {};

        // デバイスインプット

        // keyboardDevice

        // 各キーの入力状態

        // マウスの入力状態


};
