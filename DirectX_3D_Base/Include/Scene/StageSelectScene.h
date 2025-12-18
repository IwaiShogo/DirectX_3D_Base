/*****************************************************************//**
 * @file	StageSelectScene.h
 * @brief	ステージセレクト
 *********************************************************************/

#ifndef ___STAGE_SELECT_SCENE_H___
#define ___STAGE_SELECT_SCENE_H___

#include "Scene/Scene.h"
#include "ECS/Coordinator.h"

#include <DirectXMath.h>   // ★DirectX::XMFLOAT2 用


#include <functional>
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <random>

struct StageData {
	std::string name;
	std::string imageID;
	float timeLimitStar;
	std::vector<std::string> items;

	struct GimmickInfo {
		std::string type;
		int count;
	};
	std::vector<GimmickInfo> gimmicks;
};

class StageSelectScene : public Scene
{
private:
	std::shared_ptr<ECS::Coordinator> m_coordinator;
	static ECS::Coordinator* s_coordinator;

public:
	StageSelectScene() : m_coordinator(nullptr) {}
	~StageSelectScene() override {}

	void Init() override;
	void Uninit() override;
	void Update(float deltaTime) override;
	void Draw() override;

	static ECS::Coordinator* GetCoordinator() { return s_coordinator; }

private:
	// データ
	void LoadStageData();
	std::map<std::string, StageData> m_stageDataMap;
	std::string m_selectedStageID;

	// UI
	void SwitchState(bool toDetail);
	void SetUIVisible(ECS::EntityID id, bool visible);

	std::vector<ECS::EntityID> m_listUIEntities;
	std::vector<ECS::EntityID> m_detailUIEntities;

	// カーソルは常駐（消さない）
	ECS::EntityID m_cursorEntity = (ECS::EntityID)-1;

	// ===== Fade =====
	enum class FadeState { None, FadingOut, FadingIn };

	ECS::EntityID m_fadeEntity = (ECS::EntityID)-1;
	FadeState m_fadeState = FadeState::None;

	float m_fadeTimer = 0.0f;
	float m_fadeDuration = 1.0f;
	float m_fadeAlpha = 0.0f;

	bool m_inputLocked = false;
	std::function<void()> m_onBlack = nullptr;
	bool m_autoFadeInAfterBlack = true;

	void CreateFadeOverlay();
	void StartFadeIn(float durationSec = 1.0f);
	void StartFadeOut(float durationSec, std::function<void()> onBlack, bool autoFadeIn);
	void UpdateFade(float dt);
	void ApplyFadeAlpha(float a);

	void  KillAllShootingStars();

	// ===== Shooting Star（UI_STAGE_MAP内でたまに）=====
	ECS::EntityID m_stageMapEntity = (ECS::EntityID)-1;

	// ===== Shooting Star instance =====
	struct ShootingStarInstance
	{
		ECS::EntityID star = (ECS::EntityID)-1;   // 本体

		// 軌跡（重ねる）
		ECS::EntityID trails[3] = {
			(ECS::EntityID)-1,
			(ECS::EntityID)-1,
			(ECS::EntityID)-1
		};

		DirectX::XMFLOAT2 velocity = { -320.0f, 110.0f }; // 右上→左下（※座標系により符号は調整）
		float remaining = 0.0f; // 残り時間
		float life = 0.0f;      // 生成時寿命（進行度計算用）
	};

	std::vector<ShootingStarInstance> m_activeShootingStars;

	void UpdateActiveShootingStars(float dt);


	// ★デバッグ用：マップ上に確実に見えるエフェクト（TREASURE_GLOW）を常駐表示して切り分け
	ECS::EntityID m_debugStarEntity = (ECS::EntityID)-1;
	bool m_debugShowGlowOnMap = true;

	bool m_isDetailMode = false;

	// ★出現間隔（秒）ここを変えると頻度が変わる
	float m_shootingStarIntervalMin = 3.0f; // 例：頻繁=1.0f、レア=3.0f
	float m_shootingStarIntervalMax = 8.0f; // 例：頻繁=2.5f、レア=8.0f

	float m_shootingStarTimer = 0.0f;
	float m_nextShootingStarWait = 0.0f; // ★0で開始し、最初の抽選で決める
	bool  m_enableShootingStar = true;

	// ★追加：詳細に入ったあと、フェードが終わった瞬間に1回だけ出す
	bool  m_spawnStarOnEnterDetail = false;

	std::mt19937 m_rng;

	void UpdateShootingStar(float dt);
	void SpawnShootingStar();
	void EnsureDebugEffectOnMap(); // ★追加：切り分け用にGLOWを常駐
	bool GetUIRect(ECS::EntityID id, float& left, float& top, float& right, float& bottom) const;

};

#endif // !___STAGE_SELECT_SCENE_H___
