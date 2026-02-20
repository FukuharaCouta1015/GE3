#pragma once
#include <d3d12.h>
#include <dxgi.h>
#include <wrl.h>
#include "DirectXCommon.h"

class DirectXCommon;

class SpriteCommon {
public:
    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize(DirectXCommon* dxCommon);

    /// <summary>
    /// 2D描画前処理
    /// </summary>
    void PreDraw();

public:
    /// <summary>
    /// DirectXCommonのポインタのアクセサ
    /// </summary>
    /// <returns></returns>
    DirectXCommon* GetDxCommonPtr() const { return dxCommon; }

private:
    /// <summary>
    /// ルートシグネチャ作成
    /// </summary>
    void CreateRootSignature();

    /// <summary>
    /// PSO作成
    /// </summary>
    void CreatePipelineStateObject();

private:

    // 必要な変数たち
    DirectXCommon* dxCommon = nullptr;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState = nullptr;
    Microsoft::WRL::ComPtr<ID3D10Blob> vertexShaderBlob = nullptr;
    Microsoft::WRL::ComPtr<ID3D10Blob> pixelShaderBlob = nullptr;
};