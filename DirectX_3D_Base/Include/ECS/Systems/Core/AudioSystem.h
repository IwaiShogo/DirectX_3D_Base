/*****************************************************************//**
 * @file	AudioSystem.h
 * @brief	SoundComponentを処理し、サウンドの再生・停止を制御するECSシステム
 * 
 * @details	
 * SoundComponentの要求に基づき、AssetManagerからSoundEffectを取得し、
 * 再生、停止、ループ処理を管理する。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/18	初回作成日
 * 			作業内容：	- 追加：AudioSystemクラスの定義
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___AUDIO_SYSTEM_H___
#define ___AUDIO_SYSTEM_H___

// ===== インクルード =====
#include "ECS/ECS.h"
#include "Systems/AssetManager.h"
#include "Systems/XAudio2/SoundEffect.h"
#include <map>
#include <iostream>

/**
 * @class	AudioSystem
 * @brief	サウンドの再生・停止を制御する
 */
class AudioSystem
	: public ECS::System
{
private:
	// BGMなど、継続定期に再生されるサウンドをトラッキングするためのマップ。
	// Entityが持つSoundEffectのポインタを保持し、停止/クリーンアップに利用する。
	std::map<ECS::EntityID, Audio::SoundEffect*> m_playingPersistentSounds;

	// 破棄対象のエンティティIDを保持するセット
	std::set<ECS::EntityID> m_entitiesToDestroy;

	ECS::Coordinator* m_coordinator = nullptr;

public:
	void Init(ECS::Coordinator* coordinator) override
	{
		m_coordinator = coordinator;
	}

	void Update(float deltaTime) override;

	void OnEntityDestroyed(ECS::EntityID entity);

private:
	/**
	 * @brief	SoundComponent（永続サウンド）の更新ロジック
	 */
	void UpdatePersistentSound(ECS::EntityID entity);

	/**
	 * @brief	OneShotSoundComponent（単発SE）の更新ロジック
	 */
	void UpdateOneShotSound(ECS::EntityID entity);
};

#endif // !___AUDIO_SYSTEM_H___