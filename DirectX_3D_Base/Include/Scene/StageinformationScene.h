/*****************************************************************//**
 * @file	StageSelectScene.h
 * @brief	ステージセレクトのメインロジックを含むシーンクラス
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Rituki
 * ------------------------------------------------------------
 *
 * @date	2025/11/13	初回作成日
 * 			作業内容：	- 追加：
 *
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 *
 * @note	（省略可）
 *********************************************************************/

#ifndef ___STAGE__INFORMATION_SCENE___
#define ___STAGE__INFORMATION_SCENE___

 // ===== インクルード =====
#include "Scene/Scene.h"
#include "ECS/Coordinator.h"

#include <memory>

/**
 * @class	StageSelectScene
 * @brief	ステージセレクトロジックとECSを管理するシーン
 */
class StageinformationScene
	: public Scene
{
public:
	StageinformationScene()
		: m_coordinator(nullptr)
	{
	}
	~StageinformationScene()override {}



	void Init() override;
	void Uninit() override;
	void Update(float deltaTime) override;
	void Draw() override;
	void UpdateTimeDisplay(float time);
	static ECS::Coordinator* GetCoordinator() { return s_coordinator; }

private:
	// ECSの中心となるコーディネーター (シーンがECSのライフサイクルを管理)
	std::shared_ptr<ECS::Coordinator> m_coordinator;

	ECS::EntityID m_cursorEntity = ECS::INVALID_ENTITY_ID;


	ECS::EntityID m_MapImage = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_BestTime = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_Treasure = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_Security = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_OK = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_BACK = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_selectA = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_selectB = ECS::INVALID_ENTITY_ID;

	ECS::EntityID m_timeDigits[5] = { ECS::INVALID_ENTITY_ID };

	ECS::EntityID m_bg = ECS::INVALID_ENTITY_ID;


	ECS::EntityID m_selectcork = ECS::INVALID_ENTITY_ID;

	// ECSのグローバルアクセス用 (SystemなどがECS操作を行うための窓口)
	static ECS::Coordinator* s_coordinator;
	
	//アニメーション進行時間
	float m_animTime = 0.0f;

	//アニメーション完了フラグ
	bool m_isAnimationFinished = false;

	//ヘルパー関数:線形補間(AからBへ t(0～1)の割合で移動)
	float Lerp(float start, float end, float t)
	{
		return start + (end - start) * t;
	}

	// 値を0~1の範囲に収めるクランプ関数
	float Clamp01(float value)
	{
		if (value < 0.0f) return 0.0f;
		if (value > 1.0f) return 1.0f;
		return value;
	}

	bool m_isSceneChanging = false;
	enum class NextScene {
		NONE,
		GAME,
		SELECT
	};
	NextScene m_nextScene = NextScene::NONE;
};
#endif // !___STAGE__INFORMATION_SCENE___