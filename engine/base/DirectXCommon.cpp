#include "DirectXCommon.h"
#include "Logger.h"
#include "StringUtility.h"
#include "WinApp.h"
#include "externals/imagui/imgui.h"
#include "externals/imagui/imgui_impl_dx12.h"
#include "externals/imagui/imgui_impl_win32.h"
#include <cassert>
#include <format>
#include <string>
#include "externals/DirectXTex/DirectXTex.h"
#pragma comment(lib, "d3d12.lib")

using namespace Microsoft::WRL;
using namespace Logger;
using namespace StringUtility;

HRESULT hr = S_OK;

void DirectXCommon::Initialize(WinApp* winApp)
{

    assert(winApp);
    this->winApp = winApp;

    // デバックレイヤーの有効化
    CreateDevice();

    // コマンド周りの生成
    CreateCommand();

    //====================================
    // スワップチェインの生成
    // スワップチェイン用のリソースの生成
    //====================================

    CreateSwapChain();

    CreateDepthBuffer();

    // ディスクリプタヒープの生成
    CreateHeap();

    /*
      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = CreateDescriptorHeap(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV, // レンダーターゲットビュー用
        2, // ダブルバッファ用に2つ
        false); // シェーダーからは使わない

      Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = CreateDescriptorHeap(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // シェーダーリソースビュー用
        128,
        true); // シェーダーから使う
        */

    // SwapChainからResourceを取得する
    hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
    assert(SUCCEEDED(hr));
    hr = swapChain_->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
    assert(SUCCEEDED(hr));

    //=============================
    // 震度バッファの生成
    // 震度バッファのテクスチャ生成
    // DSVの生成
    //=============================

    CreateDepth();

    // RTVの生成
    CreateRTV();

    //// RTVの設定
    // D3D12_RENDER_TARGET_VIEW_DESC rtvDesc {};
    // rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 出力結果をSRGBに変換して書き込む
    // rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
    //// ディスクリプタの先頭を取得する
    // D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    //// 2つ用意
    // D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
    //// 1つめを作る
    // rtvHandles[0] = rtvStartHandle;
    // device_->CreateRenderTargetView(swapChainResources[0].Get(), &rtvDesc, rtvHandles[0]);
    //// 2つめのディスクリプタハンドルを得る
    // rtvHandles[1].ptr = rtvHandles[0].ptr + device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    //// 2つめを作る
    // device_->CreateRenderTargetView(swapChainResources[1].Get(), &rtvDesc, rtvHandles[1]);

    // Fenceの生成
    CreateFence();

    // ビューポートの生成
    CreateViewport();

    // シザー矩形の生成
    CreateScissoring();

    // DXCの生成
    CreateDXC();

    // ImGuiの生成
    CreateImGui();
}

//DirectXCommon* DirectXCommon::GetInstance()
//{
//    static DirectXCommon instance;
//    return &instance;
//}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t index, uint32_t descriptorSize)
{
    D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handleCPU.ptr += descriptorSize * index;
    return handleCPU;

    return D3D12_CPU_DESCRIPTOR_HANDLE();
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t index, uint32_t descriptorSize)
{
    D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    handleGPU.ptr += descriptorSize * index;
    return handleGPU;

    return D3D12_GPU_DESCRIPTOR_HANDLE();
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVCPUDescriptorHandle(uint32_t index)
{
    return GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVGPUDescriptorHandle(uint32_t index)
{
    return GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, index);
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetDSVCPUDescriptorHandle(uint32_t index)
{
    return GetCPUDescriptorHandle(dsvDescriptorHeap, descriptorSizeDSV, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetDSVGPUDescriptorHandle(uint32_t index)
{
    return GetGPUDescriptorHandle(dsvDescriptorHeap, descriptorSizeDSV, index);
}

void DirectXCommon::PreDraw()
{
    // これから書き込むバックバファの番号を取得
    UINT backBufferindex = swapChain_->GetCurrentBackBufferIndex();
    // リソースバリアで書き込み可能に変更
    D3D12_RESOURCE_BARRIER barrier {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = swapChainResources[backBufferindex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList_->ResourceBarrier(1, &barrier);
    // 描画先のRTVを設定する
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    commandList_->OMSetRenderTargets(1, &rtvHandles[backBufferindex], false, &dsvHandle);

    float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f }; // 青っぽい色

    // 指定した色で画面全体をクリアする
    commandList_->ClearRenderTargetView(rtvHandles[backBufferindex], clearColor, 0, nullptr);
    commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // SRVディスクリプターヒープをセット
    ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap.Get() };
    commandList_->SetDescriptorHeaps(1, descriptorHeaps);

    //ビューポート領域の設定
    commandList_->RSSetViewports(1, &viewport);
    // シザー矩形の設定
    commandList_->RSSetScissorRects(1, &scissorRect);

}

void DirectXCommon::PostDraw()
{

    // これから書き込むバックバファの番号を取得
    UINT backBufferindex = swapChain_->GetCurrentBackBufferIndex();



    //リソースバリアでプレゼント可能に変更
    D3D12_RESOURCE_BARRIER barrier {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = swapChainResources[backBufferindex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    commandList_->ResourceBarrier(1, &barrier);


    hr = commandList_->Close();
    assert(SUCCEEDED(hr));

    // GPUコマンドの実行
    // GPUにコマンドリストの実行を行わせる
    ID3D12CommandList* commandLists[] = { commandList_.Get() };
    commandQueue_->ExecuteCommandLists(_countof(commandLists), commandLists);

    swapChain_->Present(1, 0); // 画面に表示する
    assert(SUCCEEDED(hr));

    // フェンスの値を更新
    fenceValue++;
    // コマンドキューにシグナルを送ります。
    commandQueue_->Signal(fence_.Get(), fenceValue);
    //コマンド完了待ち
    if (fence_->GetCompletedValue() < fenceValue) {
        fence_->SetEventOnCompletion(fenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }

    hr = commandAllocator_->Reset();
    assert(SUCCEEDED(hr));
    hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
    assert(SUCCEEDED(hr));
}



void DirectXCommon::CreateDevice()
{
    // デバックレイヤーの有効化
#ifdef _DEBUG

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController_)))) {
        debugController_->EnableDebugLayer();
        debugController_->SetEnableGPUBasedValidation(TRUE);
    }

#endif

    CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));

    assert(SUCCEEDED(hr));
    IDXGIAdapter4* uesAdapter = nullptr;

    for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&uesAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC3 adapterDesc {};
        hr = uesAdapter->GetDesc3(&adapterDesc);
        assert(SUCCEEDED(hr));
        if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
            Log(ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
            break;
        }
        uesAdapter = nullptr;
    }
    assert(uesAdapter != nullptr);

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
    };

    const char* featureLevelsStrings[] = { "12.2", "12.1", "12.0" };

    for (size_t i = 0; i < _countof(featureLevels); ++i) {
        hr = D3D12CreateDevice(uesAdapter, featureLevels[i], IID_PPV_ARGS(&device_));
        if (SUCCEEDED(hr)) {
            Log(std::format("FeatureLevel:{}\n", featureLevelsStrings[i]));
            break;
        }
    }
    assert(device_ != nullptr);

    Log("Complete create  D3D12Device!!!\n");

    // エラーの設定
#ifdef _DEBUG

    if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue_)))) {
        infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        // infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

        D3D12_MESSAGE_ID denyIds[] = {
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
        };

        D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
        D3D12_INFO_QUEUE_FILTER filter {};
        filter.DenyList.NumIDs = _countof(denyIds);
        filter.DenyList.pIDList = denyIds;
        filter.DenyList.NumSeverities = _countof(severities);
        filter.DenyList.pSeverityList = severities;

        infoQueue_->PushStorageFilter(&filter);

        infoQueue_->Release();
    }
#endif
}

void DirectXCommon::CreateCommand()
{
    // コマンドキューを生成する

    D3D12_COMMAND_QUEUE_DESC commandQueueDesc {};
    hr = device_->CreateCommandQueue(&commandQueueDesc,
        IID_PPV_ARGS(&commandQueue_));

    assert(SUCCEEDED(hr));

    // コマンドアロケータを生成する

    hr = device_->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&commandAllocator_));
    // コマンドアロケータの生成がうまくいかなかったので起動出来ない
    assert(SUCCEEDED(hr));

    // コマンドリストを生成する

    hr = device_->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator_.Get(),
        nullptr,
        IID_PPV_ARGS(&commandList_));
    // コマンドリストの生成がうまくいかなかったので起動出来ない
    assert(SUCCEEDED(hr));
}

void DirectXCommon::CreateSwapChain()
{

    // スワップチェーンを生成する

    swapChainDesc.Width = WinApp::kClinetWidth; // 画面の幅,ウィンドウのクライアント領域を同じものにする
    swapChainDesc.Height = WinApp::kClineHeigth; // 画面の高さ,ウィンドウのクライアント領域を同じものにする
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 画面の形式
    swapChainDesc.SampleDesc.Count = 1; // マルチサンプルしない
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 描画のターゲットとして利用する
    swapChainDesc.BufferCount = 2; // バッファの数
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 　モニターにうつしたら中身を破棄

    hr = dxgiFactory_->CreateSwapChainForHwnd(
        commandQueue_.Get(),
        winApp->GetHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf())); // スワップチェーンの取得
    assert(SUCCEEDED(hr));
}

// DSVの初期化
void DirectXCommon::CreateDepth()
{
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc {};

    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 深度24ビット、ステンシル8ビット
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2Dテクスチャ

    device_->CreateDepthStencilView(depthResource_.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void DirectXCommon::CreateHeap()
{

    rtvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
    srvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
    dsvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

    descriptorSizeSRV = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    descriptorSizeRTV = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    descriptorSizeDSV = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void DirectXCommon::CreateRTV()
{

    // スワップチェーンからリソースを取得
    for (uint32_t i = 0; i < swapChainResources.size(); ++i) {
        hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainResources[i]));
        assert(SUCCEEDED(hr));
    }
    // rtvの設定
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 出力結果をSRGBに変換して書き込む
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
    // RTVハンドルの先頭取得
    D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    // バックバッファ(2枚)分RTVを作成
    for (uint32_t i = 0; i < swapChainResources.size(); ++i) {
        rtvHandles[i] = rtvStartHandle;

        device_->CreateRenderTargetView(swapChainResources[i].Get(), &rtvDesc, rtvHandles[i]);

        // 次のディスクリプタ位置に進める
        rtvStartHandle.ptr += descriptorSizeRTV;
    }
}

void DirectXCommon::CreateScissoring()
{

    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = WinApp::kClinetWidth;
    scissorRect.bottom = WinApp::kClineHeigth;
}

void DirectXCommon::CreateFence()
{


    hr = device_->CreateFence(
        fenceValue,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&fence_));
    assert(SUCCEEDED(hr));

    HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(fenceEvent != nullptr);
}

void DirectXCommon::CreateViewport()
{

    viewport.Width = WinApp::kClinetWidth;

    viewport.Height = WinApp::kClineHeigth;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
}

void DirectXCommon::CreateDXC()
{
    // dxcComilerを初期化する

    hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
    assert(SUCCEEDED(hr));
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
    assert(SUCCEEDED(hr));
    // dxcCompilerの初期化がうまくいかなかったので起動出来ない

    hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
    assert(SUCCEEDED(hr));
}

void DirectXCommon::CreateImGui()
{
    //// スワップチェーンを生成する
    // DXGI_SWAP_CHAIN_DESC1 swapChainDesc {};
    // swapChainDesc.Width = WinApp::kClinetWidth; // 画面の幅,ウィンドウのクライアント領域を同じものにする
    // swapChainDesc.Height = WinApp::kClineHeigth; // 画面の高さ,ウィンドウのクライアント領域を同じものにする
    // swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 画面の形式
    // swapChainDesc.SampleDesc.Count = 1; // マルチサンプルしない
    // swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 描画のターゲットとして利用する
    // swapChainDesc.BufferCount = 2; // バッファの数
    // swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 　モニターにうつしたら中身を破棄

    //// RTVの設定
    // D3D12_RENDER_TARGET_VIEW_DESC rtvDesc {};
    // rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 出力結果をSRGBに変換して書き込む
    // rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // 2Dテクスチャ

    //  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = CreateDescriptorHeap(
    //    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // シェーダーリソースビュー用
    //    128,
    //    true); // シェーダーから使う

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(winApp->GetHwnd());
    ImGui_ImplDX12_Init(
        device_.Get(),
        swapChainDesc.BufferCount, // スワップチェーンのバッファ数
        rtvDesc.Format, // レンダーターゲットのフォーマップ
        srvDescriptorHeap.Get(), // シェーダーリソースビュー用のディスクリプタヒープ
        srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), // シェーダーリソースビュー用のCPUディスクリプタハンドル
        srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

Microsoft::WRL::ComPtr<IDxcBlob> DirectXCommon::CompileShader(const std::wstring& filePath, const wchar_t* profile)
{
    Log(ConvertString(std::format(L"Begin CompileShader, path:{},profile:{}\n", filePath, profile)));
    Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
    HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
    assert(SUCCEEDED(hr));

    DxcBuffer shaderSourceBuffer;
    shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
    shaderSourceBuffer.Size = shaderSource->GetBufferSize();
    shaderSourceBuffer.Encoding = DXC_CP_UTF8; // UTF-8エンコーディング

    LPCWSTR arguments[] = {
        filePath.c_str(), // シェーダーファイルのパス
        L"-E",
        L"main", // エントリーポイント関数名
        L"-T",
        profile, // シェーダープロファイル
        L"-Zi",
        L"-Qembed_debug",
        L"-Od", // 最適化なし
        L"-Zpr",
    };

    Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;
    hr = dxcCompiler_->Compile(

        &shaderSourceBuffer, // 読み込んだファイル
        arguments, // コンパイル時の引数
        _countof(arguments),
        includeHandler_.Get(), // インクルードハンドラー
        IID_PPV_ARGS(&shaderResult));

    assert(SUCCEEDED(hr));
    IDxcBlobUtf8* shaderErrors = nullptr;
    shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderErrors), nullptr);
    if (shaderErrors != nullptr && shaderErrors->GetStringLength() != 0) {
        Log(shaderErrors->GetStringPointer());
        assert(false);
    }
    Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
    hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
    assert(SUCCEEDED(hr));
    Log(ConvertString(std::format(L"Compile Succeeded,path:{},profile:{}\n", filePath, profile)));

    return shaderBlob;
}

// IDxcBlob* ComileShader(
//     const std::wstring& filePath,
//     const wchar_t* profile,
//     IDxcUtils* dxcUtils,
//     IDxcCompiler3* dxcCompiler,
//     IDxcIncludeHandler* includeHandler)
//{
//     Log(ConvertString(std::format(L"Begin CompileShader, path:{},profile:{}\n", filePath, profile)));
//     IDxcBlobEncoding* shaderSource = nullptr;
//     HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
//     assert(SUCCEEDED(hr));
//     DxcBuffer shaderSourceBuffer;
//     shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
//     shaderSourceBuffer.Size = shaderSource->GetBufferSize();
//     shaderSourceBuffer.Encoding = DXC_CP_UTF8;
//
//     LPCWSTR arguments[] = {
//         filePath.c_str(),
//         L"-E",
//         L"main",
//         L"-T",
//         profile,
//         L"-Zi",
//         L"-Qembed_debug",
//         L"-Od",
//         L"-Zpr",
//     };
//
//     IDxcResult* shaderResult = nullptr;
//     hr = dxcCompiler->Compile(
//         &shaderSourceBuffer,
//         arguments,
//         _countof(arguments),
//         includeHandler,
//         IID_PPV_ARGS(&shaderResult));
//     assert(SUCCEEDED(hr));
//
//     IDxcBlobUtf8* shaderErrors = nullptr;
//     shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderErrors), nullptr);
//     if (shaderErrors != nullptr && shaderErrors->GetStringLength() != 0) {
//         Log(shaderErrors->GetStringPointer());
//         assert(false);
//     }
//
//     IDxcBlob* shaderBlob = nullptr;
//     hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
//     assert(SUCCEEDED(hr));
//     Log(ConvertString(std::format(L"Compile Succeeded,path:{},profile:{}\n", filePath, profile)));
//     shaderSource->Release();
//     shaderResult->Release();
//     return shaderBlob;
// }

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DirectXCommon::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible)
{

    // ディスクリプタヒープの生成
    ID3D12DescriptorHeap* descriptorHeap = nullptr;
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc {};
    descriptorHeapDesc.Type = heapType; // レンダーターゲットビュー用
    descriptorHeapDesc.NumDescriptors = numDescriptors; // ダブルバッファように2つ,多くても別にかまわない
    descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    HRESULT hr = device_->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
    assert(SUCCEEDED(hr));

    return descriptorHeap;

    return Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>();
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateBufferResource(size_t sizeInBytes)
{
    D3D12_HEAP_PROPERTIES uploadHeapProperties {};
    uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC vertexResourceDesc {};

    vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vertexResourceDesc.Width = sizeInBytes; // 3頂点分のサイズ
    vertexResourceDesc.Height = 1;
    vertexResourceDesc.DepthOrArraySize = 1;
    vertexResourceDesc.MipLevels = 1;
    vertexResourceDesc.SampleDesc.Count = 1;
    vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    Microsoft::WRL::ComPtr<ID3D12Resource> BufferResource = nullptr;
     hr = device_->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &vertexResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&BufferResource));
    assert(SUCCEEDED(hr));

    return BufferResource;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateTextureResource(const DirectX::TexMetadata& metadata)
{
    D3D12_RESOURCE_DESC resourceDesc {};
    resourceDesc.Width = UINT(metadata.width);
    resourceDesc.Height = UINT(metadata.height);
    resourceDesc.MipLevels = UINT16(metadata.mipLevels);
    resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
    resourceDesc.Format = metadata.format;// 深度24ビット, ステンシル8ビット
    resourceDesc.SampleDesc.Count = 1; // サンプル数は1
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

    D3D12_HEAP_PROPERTIES heapProperties {};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // デフォルトヒープを使用

    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

    Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
     hr = device_->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, // コピー先
        nullptr,
        IID_PPV_ARGS(&resource));
    assert(SUCCEEDED(hr));

    return resource;
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages)
{
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    DirectX::PrepareUpload(device_.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);

    uint64_t intermediateSize = GetRequiredIntermediateSize(texture.Get(), 0, static_cast<UINT>(subresources.size()));

    Microsoft::WRL::ComPtr<ID3D12Resource> intermediate = CreateBufferResource(intermediateSize);

    UpdateSubresources(commandList_.Get(),texture.Get(),intermediate.Get(),0,0,static_cast<UINT>(subresources.size()),subresources.data());

    D3D12_RESOURCE_BARRIER barrier {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texture.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList_->ResourceBarrier(1, &barrier);

    return intermediate;
}

DirectX::ScratchImage DirectXCommon::LoadTexture(const std::string filePath)
{
    DirectX::ScratchImage image;
    std::wstring filePathW = StringUtility::ConvertString(filePath);
    hr = DirectX::LoadFromWICFile(filePathW.c_str(),DirectX::WIC_FLAGS_FORCE_SRGB,nullptr,image);
    std::wcout << L"Load Texture:" << filePathW << std::endl;
    (SUCCEEDED(hr));

    DirectX::ScratchImage mipImages {};
    hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);

    return mipImages;
}

// 震度バッファの初期化
void DirectXCommon::CreateDepthBuffer()
{

    D3D12_RESOURCE_DESC resourceDesc {};
    resourceDesc.Width = WinApp::kClinetWidth;
    resourceDesc.Height = WinApp::kClineHeigth;
    resourceDesc.MipLevels = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 深度24ビット, ステンシル8ビット
    resourceDesc.SampleDesc.Count = 1; // サンプル数は1
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // 深度ステンシル用のフラグ

    D3D12_HEAP_PROPERTIES heapProperties {};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // デフォルトヒープを使用

    // 深度値のクリア設定
    D3D12_CLEAR_VALUE depthClearValue {};
    depthClearValue.DepthStencil.Depth = 1.0f; // 深度値のクリア値
    depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 深度24ビット, ステンシル8ビット

    ID3D12Resource* resource = nullptr;
    HRESULT hr = device_->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度書き込み用の初期状態
        &depthClearValue, // 深度値のクリア設定
        IID_PPV_ARGS(&depthResource_));
    assert(SUCCEEDED(hr));
}
