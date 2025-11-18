/*****************************************************************//**
 * @file	AudioSystem.cpp
 * @brief	AudioSystenクラスの実装
 * 
 * @details	
 * SoundComponentのPlay/Stop要求を処理する
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/18	初回作成日
 * 			作業内容：	- 追加：AudioSystemクラスの実装
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

 // ===== インクルード =====
#include "ECS/Systems/Core/AudioSystem.h"
#include <iostream>

// グローバルコーディネータの取得を前提とする

/**
 * [void - UpdatePersistentSound]
 * @brief SoundComponent (永続サウンド) の更新ロジック。
 */
void AudioSystem::UpdatePersistentSound(ECS::EntityID entity)
{
	// 永続サウンド (BGM/Ambient) のロジック
	auto& soundComp = m_coordinator->GetComponent<SoundComponent>(entity);

	Asset::AssetInfo* info = Asset::AssetManager::GetInstance().LoadSound(soundComp.assetID);
	if (!info)
	{
		soundComp.isPlaying = false;
		return;
	}

	Audio::SoundEffect* soundEffect = static_cast<Audio::SoundEffect*>(info->pResource);

	// 1. 停止要求の処理
	if (soundComp.stopRequested)
	{
		soundEffect->Stop();
		soundComp.isPlaying = false;
		soundComp.stopRequested = false;

		if (m_playingPersistentSounds.count(entity))
		{
			m_playingPersistentSounds.erase(entity);
		}
		return;
	}

	// 2. 再生要求の処理
	if (soundComp.playRequested)
	{
		// 既に再生中なら二重再生を防ぐ
		if (soundComp.isPlaying)
		{
			soundComp.playRequested = false;
			return;
		}

		soundEffect->Play(soundComp.loopCount, soundComp.volume);
		soundComp.isPlaying = true;
		soundComp.playRequested = false;

		// 継続的なサウンドとしてトラッキングに登録
		m_playingPersistentSounds[entity] = soundEffect;
	}

	// 3. 再生中状態の維持/終了チェック (基本的に永続サウンドはループしているため、IsPlayingチェックは不要だが、SE利用のために残す)
	if (soundComp.isPlaying && soundComp.type == SoundType::SE)
	{
		if (!soundEffect->IsPlaying())
		{
			soundComp.isPlaying = false;
		}
	}
}

/**
 * [void - UpdateOneShotSound]
 * @brief OneShotSoundComponent (単発SE) の更新ロジック。
 */
void AudioSystem::UpdateOneShotSound(ECS::EntityID entity)
{
	// 単発SEのロジック
	auto& oneShotComp = m_coordinator->GetComponent<OneShotSoundComponent>(entity);

	// 1. AssetManagerからSoundEffectリソースを取得
	Asset::AssetInfo* info = Asset::AssetManager::GetInstance().LoadSound(oneShotComp.assetID);
	if (!info)
	{
		// ロード失敗時、このエンティティを破棄
		m_entitiesToDestroy.insert(entity);
		return;
	}

	Audio::SoundEffect* soundEffect = static_cast<Audio::SoundEffect*>(info->pResource);

	// 2. まだ処理されていなければ再生要求 (Play and Auto-Destroy Pattern)
	if (!oneShotComp.processed)
	{
		// ループなし (0) で再生
		soundEffect->Play(0, oneShotComp.volume);
		oneShotComp.processed = true;
		// 再生をキューに投入した後は、そのエンティティを次のフレームで破棄するためにマークする
		// ここでは即座に破棄すると他のシステムに影響が出る可能性があるため、
		// 処理済みフラグを立てた後、次のフレームで破棄する。
		// ※ XAudio2はバッファキュー投入後、すぐに再生を開始する。
	}
	else
	{
		// 既に処理されたエンティティは即座に破棄し、リソースの追跡から外す
		// SEは基本的に即時再生・即時破棄で運用し、再生完了はチェックしない。
		// 再生完了を待つ必要があるSE(例: アニメーション同期)は、専用のシステムやComponentで管理するか、
		// SoundComponentを短時間だけ利用するなどの工夫が必要。
		m_entitiesToDestroy.insert(entity);
	}
}


/**
 * [void - Update]
 * @brief	毎フレームの更新処理。SoundComponentの状態をチェックし、サウンドを制御する。
 */
void AudioSystem::Update()
{
	//(void)dt;

	// 永続サウンドと単発SEのエンティティを両方処理できるように、mEntitiesのループ内で処理を振り分ける
	for (auto const& entity : m_entities)
	{
		// 永続サウンドのコンポーネントを持っているか
		if (m_coordinator->HasComponent<SoundComponent>(entity))
		{
			UpdatePersistentSound(entity);
		}

		// 単発SEのコンポーネントを持っているか
		if (m_coordinator->HasComponent<OneShotSoundComponent>(entity))
		{
			UpdateOneShotSound(entity);
		}

		// 注: OneShotSoundComponentを持つエンティティは、UpdateOneShotSound内で破棄される。
	}

	for (ECS::EntityID entity : m_entitiesToDestroy)
	{
		m_coordinator->DestroyEntity(entity);
	}
	m_entitiesToDestroy.clear();
}

/**
 * [void - OnEntityDestroyed]
 * @brief	エンティティが破壊された際のクリーンアップ処理。
 */
void AudioSystem::OnEntityDestroyed(ECS::EntityID entity)
{
	// 継続的なサウンドのエンティティが破壊された場合、再生を停止する
	if (m_playingPersistentSounds.count(entity))
	{
		Audio::SoundEffect* soundEffect = m_playingPersistentSounds.at(entity);
		if (soundEffect)
		{
			soundEffect->Stop();
		}
		m_playingPersistentSounds.erase(entity);
	}

	if (m_entitiesToDestroy.count(entity))
	{
		m_entitiesToDestroy.erase(entity);
	}
}