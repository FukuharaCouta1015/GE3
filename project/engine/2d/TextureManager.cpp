#include "TextureManager.h"
#include "DirectXCommon.h"


uint32_t TextureManager::kSRVIndexTop = 1;
TextureManager* TextureManager::mInstance_ = nullptr;

const DirectX::TexMetadata& TextureManager::GetTextureMetadata(uint32_t textureIndex)
{
    // 範囲外指定違反チェック
    assert(textureIndex < mTextureDatas.size());

    TextureData& textureData = mTextureDatas[textureIndex];
    return textureData.metadata;
}

TextureManager* TextureManager::GetInstance()
{

    if (mInstance_ == nullptr) {
        mInstance_ = new TextureManager();
    }
    return mInstance_;
}

void TextureManager::Initialize(DirectXCommon* dxCommon)
{

    mDxCommonPtr_ = dxCommon;

    mTextureDatas.reserve(DirectXCommon::kMaxSRVCount);
}

void TextureManager::LoadTexture(const std::string& filePath)
{
    // 読み込み済みテクスチャを検索
    
    auto it = std::find_if(
        mTextureDatas.begin(),
        mTextureDatas.end(),
        [&](TextureData& textureData) { return textureData.filePath == filePath; });
    if (it != mTextureDatas.end()) {
        // 余も混み済みなら早期return
        return;
    }

    assert(mTextureDatas.size() + kSRVIndexTop < DirectXCommon::kMaxSRVCount);

    // テクスチャファイルを読んでプログラムで扱えるようにする

    // 　MipMapの作成
    DirectX::ScratchImage mipImages = mDxCommonPtr_->LoadTexture(filePath);

    // テクスチャデータを追加
    mTextureDatas.resize(mTextureDatas.size() + 1);
    // 追加したテクスチャデータの参照を取得する
    TextureData& textureData = mTextureDatas.back();

    // テクスチャデータ書き込み
    textureData.filePath = filePath;
    textureData.metadata = mipImages.GetMetadata();
    textureData.resource = mDxCommonPtr_->CreateTextureResource(textureData.metadata);

    // テクスチャデータの要素数番号をsrvのindexにする
    uint32_t srvIndex = static_cast<uint32_t>(mTextureDatas.size() - 1) + kSRVIndexTop;

    textureData.textureHandleCPU = mDxCommonPtr_->GetSRVCPUDescriptorHandle(srvIndex);
    textureData.textureHandleGPU = mDxCommonPtr_->GetSRVGPUDescriptorHandle(srvIndex);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc {};
    srvDesc.Format = textureData.metadata.format; // テクスチャのフォーマット
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // シェーダーのコンポーネントマッピング
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
    srvDesc.Texture2D.MipLevels = UINT(textureData.metadata.mipLevels); // 最初のミップマップレベル

    // テクスチャのシェーダーリソースビューを作成する
    mDxCommonPtr_->GetDevice()->CreateShaderResourceView(textureData.resource.Get(), &srvDesc, textureData.textureHandleCPU);

    mDxCommonPtr_->UploadTextureData(textureData.resource, mipImages);
}

uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath)
{
    auto it = std::find_if(
        mTextureDatas.begin(),
        mTextureDatas.end(),
        [&](TextureData& textureData) { return textureData.filePath == filePath; });

    if (it != mTextureDatas.end()) {
        // 読み込み済みなら要素番号を返す
        uint32_t textureIndex = static_cast<uint32_t>(std::distance(mTextureDatas.begin(), it));
        return textureIndex;
    }

    assert(0);
    return 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(uint32_t textureIndex)
{
    // 範囲外指定違反チェック
    assert(textureIndex < mTextureDatas.size());

    TextureData& textureData = mTextureDatas[textureIndex];
    return textureData.textureHandleGPU;
}

void TextureManager::Finalize()
{
    delete mInstance_;
    mInstance_ = nullptr;
}
