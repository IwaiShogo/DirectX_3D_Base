/* UIWallComponent.h */

#pragma once

#include "ECS/Types.h"
#include "DirectXMath.h" 
#include <string> 

namespace ECS
{
    /**
     * @struct UIWallComponent
     * @brief 壁に張り付くUIのデータを持つコンポーネント
     */
    struct UIWallComponent
    {
        // 矩形UIの左上座標 (0.0f to 1.0f の範囲)
        DirectX::XMFLOAT2 UIOffset = { 0.25f, 0.25f };

        // 矩形UIのサイズ (0.0f to 1.0f の範囲)
        DirectX::XMFLOAT2 UISize = { 0.5f, 0.5f };

        // 使用するテクスチャID (AssetID)
        // ここに "UI_STAGE_BACKGROUND" が設定されることを想定
        std::string TextureID = "UI_STAGE_BACKGROUND";

        // UIを壁から少し浮かせた描画深度オフセット（Z-Fighting対策）
        float DepthOffset = 0.001f;
    };
}