/*****************************************************************//**
 * @file	Transform.h
 * @brief	エンティティの空間情報（位置、回転、スケール）を保持するコンポーネント
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/22	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___TRANSFORM_H___
#define ___TRANSFORM_H___

// ===== インクルード =====
#include "ECS/Types.h"
#include <DirectXMath.h>

/**
 * @struct	Transform
 * @brief	Entityの3D空間における位置、回転、スケール、およびワールド行列を保持するコンポーネント
 * @note	ECSにおけるComponentは純粋なデータコンテナであるべきです。
 */
struct Transform
	: public IComponent
{
	// C++の組み込み型やDirectXのシンプルなベクトル型を使用
	// XMFLOAT3はDirectX Mathとの互換性を高めるために推奨されます
	DirectX::XMFLOAT3 position	= { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 rotation	= { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 scale		= { 1.0f, 1.0f, 1.0f };

	// キャッシュされたワールド行列（Systemによって毎フレーム計算・更新される）
	DirectX::XMFLOAT4X4 worldMatrix;

	/**
	 * [void - Transform]
	 * @brief	コンストラクタでワールド行列を初期化
	 */
	Transform()
	{
		DirectX::XMStoreFloat4x4(&worldMatrix, DirectX::XMMatrixIdentity());
	}
};

#endif // !___TRANSFORM_H___