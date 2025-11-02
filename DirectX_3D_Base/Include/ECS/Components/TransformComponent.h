/*****************************************************************//**
 * @file	TransformComponent.h
 * @brief	Entityの空間的な位置、回転、スケールを定義するComponent。
 * 
 * @details	
 * DirectXMathのベクトル型を使用し、3D空間におけるEntityの状態を保持する。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	初回作成日
 * 			作業内容：	- 追加：位置、回転、スケールをDirectX::XMFLOAT3で保持する `TransformComponent` を作成。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___TRANSFORM_COMPONENT_H___
#define ___TRANSFORM_COMPONENT_H___

#include <DirectXMath.h>

 /**
  * @struct TransformComponent
  * @brief Entityの移動、回転、スケール情報（ワールド座標変換データ）
  */
struct TransformComponent
{
	DirectX::XMFLOAT3 position;		///< ワールド座標系における位置 (X, Y, Z)
	DirectX::XMFLOAT3 rotation;		///< オイラー角での回転量（ラジアン）
	DirectX::XMFLOAT3 scale;		///< 各軸のスケール（大きさ）

	/**
	 * @brief コンストラクタ
	 * @param pos - 初期位置
	 * @param rot - 初期回転
	 * @param scale - 初期スケール
	 */
	TransformComponent(
		DirectX::XMFLOAT3 pos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
		DirectX::XMFLOAT3 rot = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
		DirectX::XMFLOAT3 scale = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f)
	) : position(pos), rotation(rot), scale(scale)
	{}
};

// Componentの自動登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(TransformComponent)

#endif // !___TRANSFORM_COMPONENT_H___