/*****************************************************************//**
 * @file    RenderComponent.h
 * @brief   描画に必要な最低限の情報を保持するComponent。
 *
 * @details
 *  - 形状種別（プリミティブ/モデル）
 *  - 乗算カラー（RGBA）
 *  - カリング設定（両面描画など）
 *
 * 以前は type と color のみでしたが、
 * 「左右反転（負スケール）」で裏面が正面扱いになった際に真っ黒に見える問題の対策として、
 * エンティティ単位でカリングを制御できるよう CullMode を追加しています。
 *********************************************************************/

#ifndef ___RENDER_COMPONENT_H___
#define ___RENDER_COMPONENT_H___

#include <DirectXMath.h>
#include <cstdint>

 /**
  * @enum MeshType
  * @brief 描画するメッシュ種別。
  */
enum MeshType : uint8_t
{
    MESH_BOX,       ///< 立方体
    MESH_SPHERE,    ///< 球
    MESH_MODEL,     ///< 外部モデル（FBX等）
    MESH_NONE,      ///< 描画しない
};

/**
 * @enum CullMode
 * @brief ラスタライザのカリング設定。
 */
enum class CullMode : uint8_t
{
    Default = 0,    ///< エンジン既定（通常はBack）
    None,           ///< 両面描画
    Back,           ///< 裏面をカリング
    Front,          ///< 表面をカリング
};

/**
 * @struct RenderComponent
 * @brief Entity の描画情報。
 */
struct RenderComponent
{
    MeshType type;                  ///< メッシュ種別
    DirectX::XMFLOAT4 color;        ///< 乗算カラー (R,G,B,A)
    CullMode cullMode;              ///< カリング設定

    /**
     * @brief コンストラクタ
     * @param type     メッシュ種別
     * @param color    乗算カラー
     * @param cullMode カリング設定（省略時はDefault）
     */
    RenderComponent(
        MeshType type = MESH_BOX,
        DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
        CullMode cullMode = CullMode::Default
    )
        : type(type)
        , color(color)
        , cullMode(cullMode)
    {
    }
};

// Component の自動登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(RenderComponent)

#endif // ___RENDER_COMPONENT_H___
