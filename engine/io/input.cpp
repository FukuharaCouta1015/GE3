
#include "input.h"

#include <cassert>

#pragma once
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")


bool Input::PushKey(BYTE keyNumber)
{
    if (key[keyNumber]) {
        return true;
    }
    return false;
}

bool Input::TriggerKey(BYTE keyNumber)
{
    if (key[keyNumber] && !keyPre[keyNumber]) {
        return true;
    }

    return false;
}

void Input::Update()
{

    memcpy(keyPre, key, sizeof(key));
   
    // キーボード情報の取得を開始
    keyboard->Acquire();
    // 全キーの入力状態を取得
    keyboard->GetDeviceState(sizeof(key), key);

 

}

void Input::Initialize(WinApp* winApp)
{
    HRESULT hr;
    this->winApp_ = winApp;
    

    IDirectInput8* directInput = nullptr;
    hr = DirectInput8Create(
        winApp->GetHinstance(),
        DIRECTINPUT_VERSION,
        IID_IDirectInput8,
        (void**)&directInput, nullptr);
    assert(SUCCEEDED(hr));

  //  ComPtr<IDirectInputDevice8> keyboard;
    hr = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
    assert(SUCCEEDED(hr));

    hr = keyboard->SetDataFormat(&c_dfDIKeyboard);
    assert(SUCCEEDED(hr));

    hr = keyboard->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    assert(SUCCEEDED(hr));

}

