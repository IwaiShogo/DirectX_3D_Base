/*****************************************************************//**
 * @file	MapComponent.h
 * @brief	ランダム生成されたステージ構造データを保持するComponent
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/06	初回作成日
 * 			作業内容：	- 追加：マップ生成ロジックに必要なグリッド構造とタイルタイプを定義。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___MAP_COMPONENT_H___
#define ___MAP_COMPONENT_H___

// ===== インクルード =====
#include "ProcGen/LevelGenerator.h" // LevelGeneratorのデータ構造に依存

/**
 * @struct MapComponent
 * @brief BSP/MSTによるマップのレイアウトデータとワールド座標へのマッピングを保持する
 */
struct MapComponent
{
    ProcGen::MuseumLayout layout;	///< BSP/MSTで生成された部屋と通路のデータ
    ProcGen::GridMapping mapping;	///< ワールド座標への変換情報

    // マップサイズ
    float areaW;
    float areaH;

    MapComponent(float w = 50.0f, float h = 50.0f)
        : areaW(w)
        , areaH(h)
    {
        // 初期パラメータ設定
        mapping.scaleXZ = 1.0f;
        mapping.yFloor = 0.0f;
        mapping.wallHeight = 2.5f;
    }
};

// Componentの自動登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(MapComponent)

#endif // !___MAP_COMPONENT_H___