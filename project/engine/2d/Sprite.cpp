#include "Sprite.h"
#include "MyMath.h"
#include "SpriteCommon.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>
#include "StringUtility.h"
#include "DirectXCommon.h"
#include "TextureManager.h"

using namespace StringUtility;

void Sprite::Initialize(SpriteCommon* spriteCommon ,std::string textureFilePath)
{
    this->spriteCommonPtr = spriteCommon;
    DirectXCommon* dxCommon = spriteCommonPtr->GetDxCommonPtr();
   // spriteCommonPtr = spriteCommon;

    mVertexResource = spriteCommon->GetDxCommonPtr()->CreateBufferResource(sizeof(VertexData) * 6); // 3頂点分のサイズ
   // D3D12_VERTEX_BUFFER_VIEW vertexBufferView {};
    vertexBufferView.BufferLocation = mVertexResource->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = sizeof(VertexData) * 4; // 3頂点分のサイズ
    vertexBufferView.StrideInBytes = sizeof(VertexData); // 1頂点分のサイズ

   
    mVertexResource->Map(0, nullptr, reinterpret_cast<void**>(&mVertexDataPtr));

    // スプライトの頂点データを設定する
    mVertexDataPtr[0].position = { 0.0f, 360.0f, 0.0f, 1.0f };
    mVertexDataPtr[0].texcoord = { 0.0f, 1.0f };
    mVertexDataPtr[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };
    mVertexDataPtr[1].texcoord = { 0.0f, 0.0f };
    mVertexDataPtr[2].position = { 640.0f, 360.0f, 0.0f, 1.0f };
    mVertexDataPtr[2].texcoord = { 1.0f, 1.0f };
    mVertexDataPtr[3].position = { 640.0f, 0.0f, 0.0f, 1.0f };
    mVertexDataPtr[3].texcoord = { 1.0f, 0.0f };


      // 頂点インデックス
    mIndexResource = spriteCommon->GetDxCommonPtr()->CreateBufferResource(sizeof(uint32_t) * 6); // 3頂点分のサイズ

   // D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite {};
    indexBufferView.BufferLocation = mIndexResource->GetGPUVirtualAddress();
    indexBufferView.SizeInBytes = sizeof(uint32_t) * 6; // 3頂点分のサイズ
    indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 32ビット整数のインデックス


    mIndexResource->Map(0, nullptr, reinterpret_cast<void**>(&mIndexDataPtr));
    mIndexDataPtr[0] = 0; // 1つ目の頂点を参照
    mIndexDataPtr[1] = 1; // 2つ目の頂点を参照
    mIndexDataPtr[2] = 2; // 3つ目の頂点を参照
    mIndexDataPtr[3] = 1; // 4つ目の頂点を参照
    mIndexDataPtr[4] = 3; // 5つ目の頂点を参照
    mIndexDataPtr[5] = 2; // 6つ目の頂点を参照


       // マテリアル用のリソースを作成する
    mMaterialResource = spriteCommon->GetDxCommonPtr()->CreateBufferResource(sizeof(Material));

    textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);


    mMaterialResource->Map(0, nullptr, reinterpret_cast<void**>(&mMaterialDataPtr));

    // マテリアルデータを設定する
    mMaterialDataPtr->color = MyMath::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    mMaterialDataPtr->enableLighting = true;
    mMaterialDataPtr->uvTransform = MyMath::MakeIdentity4x4();

    mTransform.scale = MyMath::Vector3(1.0f, 1.0f, 1.0f);
    mTransform.rotate = MyMath::Vector3(0.0f, 0.0f, 0.0f);
    mTransform.translate = MyMath::Vector3(0.0f, 0.0f, 0.0f);


    mTransformationMatrixResource = spriteCommon->GetDxCommonPtr()->CreateBufferResource(sizeof(TransformationMatrix));
  //  Matrix4x4* transformationMatrixDataSprite = nullptr;
    mTransformationMatrixResource->Map(0, nullptr, reinterpret_cast<void**>(&mTransformationMatrixDataPtr));

    mTransformationMatrixDataPtr->WVP = MyMath::MakeIdentity4x4();
    mTransformationMatrixDataPtr->World = MyMath::MakeIdentity4x4();

   MyMath::Matrix4x4 worldMatrixSprite = MyMath::MakeAffine(mTransform.scale, mTransform.rotate, mTransform.translate);
    MyMath::Matrix4x4 viewMatrixSprite = MyMath::MakeIdentity4x4();
   MyMath::Matrix4x4 projectionMatrixSprite = MyMath::Orthographic(0.0f, 0.0f, float(WinApp::kClinetWidth), float(WinApp::kClineHeigth), 0.0f, 100.0f);
   MyMath::Matrix4x4 worldViewProjectionMatrixSprite = Multipty(worldMatrixSprite, Multipty(viewMatrixSprite, projectionMatrixSprite));
   mTransformationMatrixDataPtr->WVP = worldViewProjectionMatrixSprite;
   
   AdjustTextureSize();
     
   // DirectX::ScratchImage mipImages = LoadTexture("Resources/uvChecker.png");
   //// DirectX::ScratchImage mipImages = sprite->LoadTexture(modelData.material.textureFilePath);
   //const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
   // textureResource = spriteCommonPtr->GetDxCommonPtr()->CreateTextureResource(metadata);
   //spriteCommonPtr->GetDxCommonPtr()->UploadTextureData(textureResource.Get(), mipImages);

   //D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc {};
   //srvDesc.Format = metadata.format; // テクスチャのフォーマット
   //srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // シェーダーのコンポーネントマッピング
   //srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
   //srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels); // 最初のミップマップレベル

   //D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = dxCommon->GetSRVCPUDescriptorHandle(1);
   //textureSrvHandleGPU = dxCommon->GetSRVGPUDescriptorHandle(1);

   // textureSrvHandleCPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   //   textureSrvHandleGPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

   // テクスチャのシェーダーリソースビューを作成する

   //dxCommon->GetDevice()->CreateShaderResourceView(textureResource.Get(), &srvDesc, textureSrvHandleCPU);


}

void Sprite::Update()
{

    mTransform.rotate = { 0.0f, 0.0f, rotation };
    mTransform.translate = { position.x, position.y, 0.0f };
    MyMath::Matrix4x4 worldMatrixSprite = MyMath::MakeAffine(mTransform.scale, mTransform.rotate, mTransform.translate);
    MyMath::Matrix4x4 viewMatrixSprite = MyMath::MakeIdentity4x4();
    MyMath::Matrix4x4 projectionMatrixSprite = MyMath::Orthographic(0.0f, 0.0f, float(WinApp::kClinetWidth), float(WinApp::kClineHeigth), 0.0f, 100.0f);
    MyMath::Matrix4x4 worldViewProjectionMatrixSprite = Multipty(worldMatrixSprite, Multipty(viewMatrixSprite, projectionMatrixSprite));
    mTransformationMatrixDataPtr->WVP = worldViewProjectionMatrixSprite;
  
    const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetTextureMetadata(textureIndex);
    float tex_Left = textureLeftTop.x / metadata.width;
    float tex_Right = (textureLeftTop.x + textureSize.x) / metadata.width;
    float tex_Top = textureLeftTop.y / metadata.height;
    float tex_Bottom = (textureLeftTop.y + textureSize.y) / metadata.height;

    float left = 0.0f - anchorPoint.x;
    float right = 1.0f - anchorPoint.x;
    float top = 0.0f - anchorPoint.y;
    float bottom = 1.0f - anchorPoint.y;

    // 左下
    mVertexDataPtr[0].position = { left, bottom, 0.0f, 1.0f }; 
    mVertexDataPtr[0].texcoord = { tex_Left, tex_Bottom};
    mVertexDataPtr[0].normal = { 0.0f, 0.0f, -1.0f };

    // 左上
    mVertexDataPtr[1].position = { left, top, 0.0f, 1.0f }; 
    mVertexDataPtr[1].texcoord = { tex_Left, tex_Top };
    mVertexDataPtr[1].normal = { 0.0f, 0.0f, -1.0f };

    // 右下
    mVertexDataPtr[2].position = { right, bottom, 0.0f, 1.0f }; 
    mVertexDataPtr[2].texcoord = { tex_Right,tex_Bottom};
    mVertexDataPtr[2].normal = { 0.0f, 0.0f, -1.0f };

    // 右上
    mVertexDataPtr[3].position = { right, top, 0.0f, 1.0f };
    mVertexDataPtr[3].texcoord = { tex_Right, tex_Top };
    mVertexDataPtr[3].normal = { 0.0f, 0.0f, -1.0f };

    //左右反転
    if (isFlipX_){
        left = -left;
        right = -right;
    }

    // 上下反転 
    if (isFlipY_) {
        top = -top;
        bottom = -bottom;
    }

    mTransform.scale = { size.x, size.y, 1.0f };

}

void Sprite::Draw()
{

    DirectXCommon* dxCommon = spriteCommonPtr->GetDxCommonPtr();

    // スプライト
    dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
    // インデックスを指定
    dxCommon->GetCommandList()->IASetIndexBuffer(&indexBufferView);
    dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, mMaterialResource->GetGPUVirtualAddress());
    dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, mTransformationMatrixResource->GetGPUVirtualAddress());
    dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex));
    // ドローコール
    dxCommon->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

DirectX::ScratchImage Sprite::LoadTexture(const std::string& filePath)
{

      DirectX::ScratchImage image {};
    std::wstring filePathW = ConvertString(filePath);
    HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
    assert(SUCCEEDED(hr));

    DirectX::ScratchImage mipImage {};
    hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImage);
    assert(SUCCEEDED(hr));

    return mipImage;
}

void Sprite::AdjustTextureSize()
{
    // テクスチャメタデータ取得
    const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetTextureMetadata(textureIndex);

    textureSize.x = static_cast<float>(metadata.width);
    textureSize.y = static_cast<float>(metadata.height);
    // 画像サイズをテクスチャサイズに合わせる
    size = textureSize;
}
