#include "DirectXCommon.h"

void DirectXCommon::Initialize()
{
   
    // デバックレイヤーの有効化

#ifdef _DEBUG

    ID3D12Debug1* debugController = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        debugController->SetEnableGPUBasedValidation(TRUE);
    }

#endif


    // ファクトリーの生成
    // アダプターの選別
    // デバイスの生成

    IDXGIFactory7* dxgiFactory = nullptr;

    CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

    assert(SUCCEEDED(hr));

    IDXGIAdapter4* uesAdapter = nullptr;

    for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&uesAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
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
    //  assert(false && "テスト");
    ID3D12Device* device = nullptr;

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
    };

    const char* featureLevelsStrings[] = { "12.2", "12.1", "12.0" };

    for (size_t i = 0; i < _countof(featureLevels); ++i) {
        hr = D3D12CreateDevice(uesAdapter, featureLevels[i], IID_PPV_ARGS(&device));
        if (SUCCEEDED(hr)) {
            Log(std::format("FeatureLevel:{}\n", featureLevelsStrings[i]));
            break;
        }
    }
    assert(device != nullptr);

    Log("Complete create  D3D12Device!!!\n");

    //エラーの設定
#ifdef _DEBUG

    ID3D12InfoQueue* infoQueue = nullptr;
    if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
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

        infoQueue->PushStorageFilter(&filter);

        infoQueue->Release();
    }
#endif

    //==================
    //コマンド周りの生成
    //==================
    
    //  コマンドキューを生成する
    ID3D12CommandQueue* commandQueue = nullptr;
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc {};
    hr = device->CreateCommandQueue(&commandQueueDesc,
        IID_PPV_ARGS(&commandQueue));

    assert(SUCCEEDED(hr));

    // コマンドアロケータを生成する
    ID3D12CommandAllocator* commandAllocator = nullptr;
    hr = device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&commandAllocator));
    // コマンドアロケータの生成がうまくいかなかったので起動出来ない
    assert(SUCCEEDED(hr));

    // コマンドリストを生成する

    ID3D12GraphicsCommandList* commandList = nullptr;
    hr = device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator,
        nullptr,
        IID_PPV_ARGS(&commandList));
    // コマンドリストの生成がうまくいかなかったので起動出来ない
    assert(SUCCEEDED(hr));


    //====================================
    // スワップチェインの生成
    // スワップチェイン用のリソースの生成
    //====================================

    // スワップチェーンを生成する
    IDXGISwapChain4* swapChain = nullptr;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc {};
    swapChainDesc.Width = WinApp::kClinetWidth; // 画面の幅,ウィンドウのクライアント領域を同じものにする
    swapChainDesc.Height = WinApp::kClineHeigth; // 画面の高さ,ウィンドウのクライアント領域を同じものにする
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 画面の形式
    swapChainDesc.SampleDesc.Count = 1; // マルチサンプルしない
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 描画のターゲットとして利用する
    swapChainDesc.BufferCount = 2; // バッファの数
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 　モニターにうつしたら中身を破棄

    hr = dxgiFactory->CreateSwapChainForHwnd(
        commandQueue,
        winApp->GetHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        reinterpret_cast<IDXGISwapChain1**>(&swapChain)); // スワップチェーンの取得
    assert(SUCCEEDED(hr));


    // ディスクリプタヒープの生成

    ID3D12DescriptorHeap* rtvDescriptorHeap = CreateDescriptorHeap(
        device,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV, // レンダーターゲットビュー用
        2, // ダブルバッファ用に2つ
        false); // シェーダーからは使わない

    ID3D12DescriptorHeap* srvDescriptorHeap = CreateDescriptorHeap(
        device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // シェーダーリソースビュー用
        128,
        true); // シェーダーから使う

    
    // SwapChainからResourceを取得する
    ID3D12Resource* swapChainResources[2] = { nullptr };
    hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
    assert(SUCCEEDED(hr));
    hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
    assert(SUCCEEDED(hr));


    //=============================
    // 震度バッファの生成
    // 震度バッファのテクスチャ生成
    // DSVの生成
    //=============================

    ID3D12Resource* depthStencilResource = CreateDepthStencilTextureResource(device, WinApp::kClinetWidth, WinApp::kClineHeigth);
    ID3D12DescriptorHeap* dsvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false); // シェーダーからは使わない
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 深度24ビット、ステンシル8ビット
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
    device->CreateDepthStencilView(depthStencilResource, &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());


    // RTVの生成

     ID3D12DescriptorHeap* rtvDescriptorHeap = CreateDescriptorHeap(
        device,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV, // レンダーターゲットビュー用
        2, // ダブルバッファ用に2つ
        false); // シェーダーからは使わない

    ID3D12DescriptorHeap* srvDescriptorHeap = CreateDescriptorHeap(
        device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // シェーダーリソースビュー用
        128,
        true); // シェーダーから使う

    // SwapChainからResourceを取得する
    ID3D12Resource* swapChainResources[2] = { nullptr };
    hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
    assert(SUCCEEDED(hr));
    hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
    assert(SUCCEEDED(hr));

    // RTVの設定
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc {};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 出力結果をSRGBに変換して書き込む
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
    // ディスクリプタの先頭を取得する
    D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    // 2つ用意
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
    // 1つめを作る
    rtvHandles[0] = rtvStartHandle;
    device->CreateRenderTargetView(swapChainResources[0], &rtvDesc, rtvHandles[0]);
    // 2つめのディスクリプタハンドルを得る
    rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    // 2つめを作る
    device->CreateRenderTargetView(swapChainResources[1], &rtvDesc, rtvHandles[1]);


    // Fenceの生成
    ID3D12Fence* fence = nullptr;
    uint64_t fenceValue = 0;
    hr = device->CreateFence(
        fenceValue,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&fence));

    assert(SUCCEEDED(hr));
    HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(fenceEvent != nullptr);
}

