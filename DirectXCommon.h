#pragma once
#include "WinApp.h"
#include <array>
#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <string>
#include <wrl.h>

class DirectXCommon {
public:
    void Initialize(WinApp* winApp);

    static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t index, uint32_t descriptorSize);

    static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap, uint32_t index, uint32_t descriptorSize);

    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);

    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUDescriptorHandle(uint32_t index);

    D3D12_GPU_DESCRIPTOR_HANDLE GetDSVGPUDescriptorHandle(uint32_t index);

    void PreDraw();
    void PostDraw();

private:
    void CreateDevice();
    void CreateCommand();
    void CreateSwapChain();
    void CreateDepth();
    void CreateHeap();
    void CreateRTV();
    void CreateScissoring();
    void CreateFence();
    void CreateViewport();
    void CreateDXC();
    void CreateImGui();

    // ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

    void CreateDepthBuffer();

    Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath, const wchar_t* profile);

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

    Microsoft::WRL::ComPtr<ID3D12Device> device_ = nullptr;

    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Debug1> debugController_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;

    // Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_ = nullptr;

    Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_ = nullptr;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc {};
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;

    Microsoft::WRL::ComPtr<ID3D12Resource> depthResource_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Fence> fence_ = nullptr;

    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_ = nullptr;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_ = nullptr;

    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = nullptr;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = nullptr;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = nullptr;
    uint64_t fenceValue = 0;
    HANDLE fenceEvent = nullptr;
    D3D12_VIEWPORT viewport {};
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
    // シザー矩形
    D3D12_RECT scissorRect {};
    WinApp* winApp = nullptr;

    UINT descriptorSizeSRV = 0;
    UINT descriptorSizeDSV = 0;
    UINT descriptorSizeRTV = 0;
};