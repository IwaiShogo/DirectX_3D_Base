/*****************************************************************//**
 * @file	OneShotSoundComponent.h
 * @brief	単発SE（効果音）の再生要求を保持する一時的なコンポーネント。
 * 
 * @details	
 * このコンポーネントを持つエンティティは、AudioSystemによって
 * サウンドが再生された後、直ちに破棄されることを前提とする。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/18	初回作成日
 * 			作業内容：	- 追加：OneShotSoundComponent構造体の定義
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___ONE_SHOT_SOUND_COMPONENT_H___
#define ___ONE_SHOT_SOUND_COMPONENT_H___

// ===== インクルード =====
#include <string>
#include <DirectXMath.h>

/**
 * @struct	OneShotSoundComponent
 * @brief	単発のサウンド再生要求を
 */
struct OneShotSoundComponent
{
	// ----------------------------------------
	// 再生指示データ
	// ----------------------------------------
	std::string assetID = "";	// AssetManagerに登録されたサウンドID
	float volume = 1.0f;		// 再生ボリューム（0.0f - 1.0f）

	// 3D空間サウンド対応を考慮し、位置情報も保持
	DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };

	// ----------------------------------------
	// 実行時データ/状態
	// ----------------------------------------
	// 再生要求が処理された後、エンティティを破棄するためのフラグ
	bool processed = false;

	// ----------------------------------------
	// コンストラクタ
	// ----------------------------------------
	OneShotSoundComponent() = default;

	/**
	 * @brief	初期再生設定を行うコンストラクタ
	 * @param	[in] id アセットID
	 * @param	[in] vol ボリューム
	 * @param	[in] pos 3D空間上の位置
	 */
	OneShotSoundComponent(const std::string& id, float vol = 1.0f, const DirectX::XMFLOAT3& pos = { 0.0f, 0.0f, 0.0f })
		: assetID(id), volume(vol), position(pos)
	{
	}
};

// Component登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(OneShotSoundComponent)

#endif // !___ONE_SHOT_SOUND_COMPONENT_H___