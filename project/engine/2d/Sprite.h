#pragma once
#include <d3d12.h>
#include <string>
#include <wrl.h>

#include "math/MyMath.h"
#include "externals/DirectXTex/DirectXTex.h"

class SpriteCommon;

class Sprite {
public:
    // mainからコピペ
    struct VertexData {
        MyMath::Vector4 position;
        MyMath::Vector2 texcoord;
        MyMath::Vector3 normal;
    };

    // mainからコピペ
    struct Material {
        MyMath::Vector4 color;
        int32_t enableLighting;
        float padding[3];
        MyMath::Matrix4x4 uvTransform;
    };

    // mainからコピペ
    struct TransformationMatrix {
        MyMath::Matrix4x4 WVP;
        MyMath::Matrix4x4 World;
    };

    // mainからコピペ
    struct Transform {
        MyMath::Vector3 scale;
        MyMath::Vector3 rotate;
        MyMath::Vector3 translate;
    };

    MyMath::Vector2 position = { 0.0f, 0.0f };

    MyMath::Vector2 size { 640.0f, 360.0f };

    float rotation = 0.0f;

    //ゲッター
    const MyMath::Vector2& GetPosition() const { return position; }
    float GetRotation() const { return rotation;}
    const MyMath::Vector4& GetColor() const { return mMaterialDataPtr->color; }
    const MyMath::Vector2& GetSize() const { return size; }
    const MyMath::Vector2& GetAnchorPoint() const { return anchorPoint; }
    const bool IsFlipX() const { return isFlipX_; }
    const bool IsFlipY() const { return isFlipY_; }
    const MyMath::Vector2& GetTextureLeftTop() const { return textureLeftTop; }
    const MyMath::Vector2& GetTextureSize() const { return textureSize; }
    // セッター
    void SetPosition(const MyMath::Vector2& Position) { position = Position; }
    void SetRotation(float Rotation) { this->rotation = Rotation;}
    void SetColor(const MyMath::Vector4& color) { mMaterialDataPtr->color = color; }
    void SetSize(const MyMath::Vector2& size) { this->size = size; }
    void SetAnchorPoint(const MyMath::Vector2& anchorPoint) { this->anchorPoint = anchorPoint; }
    void SetFlipX(bool isFlipX) { this->isFlipX_ = isFlipX; }
    void SetFlipY(bool isFlipY) { this->isFlipY_ = isFlipY; }
    void SetTextureLeftTop(const MyMath::Vector2& textureLeftTop) { this->textureLeftTop = textureLeftTop; }
    void SetTextureSize(const MyMath::Vector2& textureSize) { this->textureSize = textureSize; }


public:
    void Initialize(SpriteCommon* spriteCommon,std::string textureFilePath);
    void Update();
    void Draw();

    // 静的にまとめて管理（テクスチャ）
    static DirectX::ScratchImage LoadTexture(const std::string& filePath);

private:
    // 必要な変数たち
    SpriteCommon* spriteCommonPtr = nullptr;

    void AdjustTextureSize();

    Microsoft::WRL::ComPtr<ID3D12Resource> mVertexResource = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> mIndexResource = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> mMaterialResource = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> mTransformationMatrixResource = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = nullptr;

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
    D3D12_INDEX_BUFFER_VIEW indexBufferView {};

    VertexData* mVertexDataPtr = nullptr;
    uint32_t* mIndexDataPtr = nullptr;
    Material* mMaterialDataPtr = nullptr;
    TransformationMatrix* mTransformationMatrixDataPtr = nullptr;
    uint32_t textureIndex = 0;


    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU{};

    Transform mTransform{};
    Transform mUvTransform{};
    
    MyMath::Vector2 anchorPoint = { 0.0f, 0.0f };

    // 左右フリップ
    bool isFlipX_ = false;
    // 上下フリップ
    bool isFlipY_ = false;

    MyMath::Vector2 textureLeftTop = { 0.0f, 0.0f };

    MyMath::Vector2 textureSize = { 100.0f, 100.0f };

};