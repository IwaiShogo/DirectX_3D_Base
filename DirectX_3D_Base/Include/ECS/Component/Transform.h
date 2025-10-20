/*****************************************************************//**
 * @file	TransformComponent.h
 * @brief	位置・回転・スケールコンポーネントの定義
 * 
 * @details	このコンポーネントは3D空間における「場所」「向き」「大きさ」
 *			を表す基本データです。
 *			すべての3Dオブジェクトに必要となる空間情報を保持し、
 *			他のSystemの計算基盤となります。
 * 
 *			### 座標系について：
 *			X軸：右方向が正、Y軸：上方向が正、Z軸：奥方向が正
 *			（DirectXの左手座標系を想定）
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/17	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___TRANSFORMCOMPONENT_H___
#define ___TRANSFORMCOMPONENT_H___

// ===== インクルード =====
#include <DirectXMath.h>
#include "ECS/Component.h"	// DEFINE_DATA_COMPONENTを使用

/**
 * @struct	Transform
 * @brief	3D空間におけるエンティティの位置・回転・スケールを管理するデータコンポーネント
 */
DEFINE_DATA_COMPONENT(Transform,
	/**
	 * @var		position
	 * @brief	エンティティの3D空間における位置座標
	 */
	DirectX::XMFLOAT3 position{ 0, 0, 0 };

	/**
	 * @var		rotation
	 * @brief	エンティティの回転角度（度数法）
	 * 
	 * @details	オイラー角による回転を表します。
	 *			- rotation.x: ピッチ（X軸周り）
	 *			- rotation.y: ヨー（Y軸周り）
	 *			- rotation.z: ロール（Z軸周り）
	 * 
	 * @note	角度は**度数法**（0-360度）で指定します。World内の行列計算時にラジアンに自動変換されます。
	 * @warning 大きな回転を行う場合、**ジンバルロック**が発生する可能性があります。
	 */
	DirectX::XMFLOAT3 rotation{ 0, 0, 0 };

	/**
	 * @var		scale
	 * @brief	エンティティのスケール（大きさ）
	 */
	DirectX::XMFLOAT3 scale{ 1, 1, 1 };

	/**
	 * @var		worldMatrix
	 * @brief	位置・回転・スケールから計算されたワールド行列
	 * 
	 * @details	この行列は、RenderSystemなどのシステムによって計算され、GPUに渡されます。
	 *			通常、ユーザーコードからは読み取り専用として扱われます。
	 */
	DirectX::XMFLOAT4X4 worldMatrix;

	/**
	 * [int - Transform]
	 * @brief	全パラメータを指定するコンストラクタ
	 * 
	 * @param	[in] pos	初期位置
	 * @param	[in] rot	初期回転角度（度数法）
	 * @param	[in] scl	初期スケール
	 * 
	 * @par		使用例（ピルダーパターン）：
	 * @code
	 *	// Entity Builderの.With<>で初期化に使用
	 *	Entity cube = w.Create()
	 *		.With<Transform>(
	 *			DirectX::XMFLOAT3{10, 5, 0},	// 位置
	 *			DirectX::XMFLOAT3{0, 45, 0},	// 回転
	 *			DirectX::XMFLOAT3{2, 2, 2}		// スケール
	 *		)
	 *		.Build();
	 * @endcode	
	 */
	Transform(DirectX::XMFLOAT3 pos = { 0,0,0 }, DirectX::XMFLOAT3 rot = { 0,0,0 }, DirectX::XMFLOAT3 scl = { 1,1,1 })
		: position(pos), rotation(rot), scale(scl)
	{
		// WorldMatrixを単位行列で初期化
		DirectX::XMStoreFloat4x4(&worldMatrix, DirectX::XMMatrixIdentity());
	}
);

#endif // !___TRANSFORMCOMPONENT_H___