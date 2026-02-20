#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <string>
#include <vector>

#include "externals/DirectXTex/DirectXTex.h"

class DirectXCommon;

class TextureManager {
public:

    const DirectX::TexMetadata& GetTextureMetadata(uint32_t textureIndex);
    /// <summary>
    /// シングルトンとして唯一インスタンスの取得
    /// </summary>
    /// <returns></returns>
    static TextureManager* GetInstance();

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="dxCommon"></param>
    void Initialize(DirectXCommon* dxCommon);

    /// <summary>
    /// 読み込まれていない場合はDirectXCommonからSprite用に作成
    /// </summary>
    /// <param name="filePath"></param>
    void LoadTexture(const std::string& filePath);

    /// <summary>
    /// ファイルパスからテクスチャの取得
    /// </summary>
    /// <param name="filePath"></param>
    /// <returns></returns>
    uint32_t GetTextureIndexByFilePath(const std::string& filePath);

    /// <summary>
    /// テクスチャインデックスの取得
    /// </summary>
    /// <param name="textureIndex"></param>
    /// <returns></returns>
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureIndex);

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize();

private:
    static uint32_t kSRVIndexTop;
    static TextureManager* mInstance_;

    DirectXCommon* mDxCommonPtr_ = nullptr;

    struct TextureData {
        std::string filePath;
        DirectX::TexMetadata metadata;
        Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE textureHandleCPU {};
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandleGPU {};
    };


    // シングルトンのため外部生成禁止
    TextureManager() = default;
    ~TextureManager() = default;
    TextureManager(TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    std::vector<TextureData> mTextureDatas;

};