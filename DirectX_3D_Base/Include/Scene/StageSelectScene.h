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
#include <unordered_map>

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

// StageSelectScene.h

class StageSelectScene : public Scene
{
private:
	std::shared_ptr<ECS::Coordinator> m_coordinator;
	static ECS::Coordinator* s_coordinator;

	std::unordered_map<int, ECS::EntityID> m_stageCards;

	ECS::EntityID m_revealingCardEntity = (ECS::EntityID)-1;

public:
	StageSelectScene() : m_coordinator(nullptr) {}
	~StageSelectScene() override {}

	void Init() override;
	void Uninit() override;
	void Update(float deltaTime) override;
	void Draw() override;

	static ECS::Coordinator* GetCoordinator() { return s_coordinator; }

private:
	// ステージマップのUIアセットID（CSVに登録されてる“正しい名前”に合わせる）
	static constexpr const char* kStageMapUI[6] =
	{
		// TextureList.csv の AssetID に合わせる（UI_STAGE1～UI_STAGE6）
		"UI_STAGE1",
		"UI_STAGE2",
		"UI_STAGE3",
		"UI_STAGE4",
		"UI_STAGE5",
		"UI_STAGE6",
	};

	// ★一覧カード(3D)のモデル AssetID（CSVに登録されてる“正しい名前”に合わせる）
	static constexpr const char* kSelectCardModel[6] =
	{
		"M_SELECT1",
		"M_SELECT2",
		"M_SELECT3",
		"M_SELECT4",
		"M_SELECT5",
		"M_SELECT6",
	};

	// 一覧カードの見た目（3Dモデル）
	std::vector<ECS::EntityID> m_listCardModelEntities;
	// Hover拡大用：一覧カード(3D)の基準スケール
	std::unordered_map<ECS::EntityID, DirectX::XMFLOAT3> m_listCardModelBaseScale;

	ECS::EntityID m_lastHiddenListCardModelEntity = (ECS::EntityID)-1;
	int m_lastHiddenListStageNo = -1;


	// ===== UI Select FX（OK / BACK）=====
	struct UISelectFxInstance
	{
		ECS::EntityID entity = (ECS::EntityID)-1;
		float remaining = 0.0f; // 手動寿命（秒）
	};
	std::vector<UISelectFxInstance> m_uiSelectFx;

	void UpdateUISelectFx(float dt);
	void UpdateButtonHoverScale(float dt);
	std::unordered_map<ECS::EntityID, DirectX::XMFLOAT3> m_buttonBaseScale;

	// ===== Card Focus Animation =====
	struct CardFocusAnim
	{
		bool active = false;
		ECS::EntityID entity = (ECS::EntityID)-1;

		float elapsed = 0.0f;
		float duration = 0.45f;

		DirectX::XMFLOAT3 startPos = { 0,0,0 };
		DirectX::XMFLOAT3 endPos = { 0,0,0 };
		DirectX::XMFLOAT3 startScale = { 1,1,1 };
		DirectX::XMFLOAT3 endScale = { 1,1,1 };

		DirectX::XMFLOAT3 baseRot = { 0,0,0 };
		float extraRollRad = 0.0f;
	};
	CardFocusAnim m_cardFocus;

	void StartCardFocusAnim(ECS::EntityID cardEntity, const DirectX::XMFLOAT3& uiPos);
	void UpdateCardFocusAnim(float dt);

	ECS::EntityID m_focusCardEntity = (ECS::EntityID)-1;
	void DestroyFocusCard();

	ECS::EntityID m_listBgEntity = (ECS::EntityID)-1;

	// データ
	void LoadStageData();
	std::map<std::string, StageData> m_stageDataMap;
	std::string m_selectedStageID;

	// UI
	void SwitchState(bool toDetail);
	DirectX::XMFLOAT3 GetListCardSlotCenterPos(int stageNo) const;
	void ResetSelectToDetailAnimState(bool unlockInput = false, bool keepFocusCard = false);
	void SetUIVisible(ECS::EntityID id, bool visible);
	void PlayUISelectEffect(ECS::EntityID uiEntity, const std::string& effectId, float scale);

	ECS::EntityID m_startBtnEntity = (ECS::EntityID)-1;
	ECS::EntityID m_finishBtnEntity = (ECS::EntityID)-1;

	std::vector<ECS::EntityID> m_listUIEntities;

	std::vector<std::vector<ECS::EntityID>> m_listStageLabelEntities;
	// 擬似ボールド用：重ね描きの追加スプライト群（文字数*(pass-1)）
	std::vector<std::vector<ECS::EntityID>> m_listStageLabelBoldEntities;
	// ★ステージ番号の背景（UI_STAGE_NUMBER）
	std::vector<ECS::EntityID> m_listStageLabelBgEntities;
	void BuildStageNumberLabels();
	void SyncStageNumberLabels(bool force = false);
	void SyncStageNumberLabel(int stageNo);
	void SetStageNumberLabelVisible(int stageNo, bool visible);

	std::vector<ECS::EntityID> m_detailUIEntities;
	std::vector<ECS::EntityID> m_bestTimeDigitEntities;

	ECS::EntityID m_detailStarOnEntities[3] = { (ECS::EntityID)-1, (ECS::EntityID)-1, (ECS::EntityID)-1 };

	bool        m_starRevealPending = false;
	std::string m_starRevealStageId;
	void UpdateStarIconsByStageId(const std::string& stageId);

	ECS::EntityID m_lastHiddenListCardEntity = (ECS::EntityID)-1;
	bool m_detailAppearActive = false;
	float m_detailAppearTimer = 0.0f;
	float DETAIL_APPEAR_DURATION = 0.25f;
	std::unordered_map<ECS::EntityID, DirectX::XMFLOAT3> m_detailBaseScale;
	std::unordered_map<ECS::EntityID, DirectX::XMFLOAT3> m_detailBasePos;
	void CacheDetailBaseTransform(ECS::EntityID id);
	void BeginDetailAppear();
	void UpdateDetailAppear(float dt);

	ECS::EntityID m_cursorEntity = (ECS::EntityID)-1;

	ECS::EntityID m_transitionEntity = (ECS::EntityID)-1;
	ECS::EntityID m_blackTransitionEntity = (ECS::EntityID)-1;

	bool m_enableSlideFade = false;

	bool m_waitingSceneFadeIn = false;
	std::function<void()> m_onFadeOutComplete = nullptr;

	bool m_inputLocked = false;
	bool m_isWaitingForTransition = false;
	float m_transitionWaitTimer = 0.0f;
	float m_transitionDelayTime = 1.0f;
	float LIST_TO_DETAIL_DELAY = 1.50f;
	float LIST_TO_DETAIL_ANIM_SPEED = 1.5f;
	std::string m_pendingStageID = "";

	void  KillAllShootingStars();

	// ★StageMap
	ECS::EntityID m_stageMapEntity = (ECS::EntityID)-1;
	ECS::EntityID m_stageMapOverlayEntity = (ECS::EntityID)-1;
	int StageIdToStageNo(const std::string& stageId) const;
	void UpdateBestTimeDigitsByStageId(const std::string& stageId);
	std::string GetStageMapTextureAssetId(int stageNo) const;
	void ApplyStageMapTextureByStageId(const std::string& stageId);

	// ===== Shooting Star =====
	struct ShootingStarInstance
	{
		ECS::EntityID star = (ECS::EntityID)-1;
		ECS::EntityID trails[3] = { (ECS::EntityID)-1,(ECS::EntityID)-1,(ECS::EntityID)-1 };
		DirectX::XMFLOAT2 velocity = { -320.0f, 110.0f };
		float remaining = 0.0f;
		float life = 0.0f;
	};

	std::vector<ShootingStarInstance> m_activeShootingStars;
	void UpdateActiveShootingStars(float dt);

	ECS::EntityID m_debugStarEntity = (ECS::EntityID)-1;
	bool m_debugShowGlowOnMap = true;

	bool m_isDetailMode = false;

	// ===== Unlock / Reveal =====
	int m_maxUnlockedStage = 1;
	int m_pendingRevealStage = -1;
	int   m_scheduledRevealStage = -1;
	float m_revealDelayTimer = 0.0f;

	std::vector<int> m_listStageNos;

	struct StageRevealAnim
	{
		bool active = false;
		ECS::EntityID entity = (ECS::EntityID)-1;
		float elapsed = 0.0f;
		float duration = 0.90f;

		float startY = 0.0f;
		float endY = 0.0f;
		float startAlpha = 0.0f;
		float endAlpha = 1.0f;

		DirectX::XMFLOAT3 baseScale = { 1,1,1 };
	};

	std::unordered_map<int, StageRevealAnim> m_stageReveal;

	bool IsStageUnlocked(int stageNo) const { return stageNo >= 1 && stageNo <= m_maxUnlockedStage; }
	void BeginStageReveal(int stageNo);
	void UpdateStageReveal(float dt);
	void ApplyListVisibility(bool listVisible);
	void ReflowUnlockedCardsLayout();

	bool m_isWaitingForGameStart = false;
	bool m_isWaitingForBackToList = false;

	float m_gameStartTimer = 0.0f;
	const float GAME_START_DELAY = 1.0f;

	float m_shootingStarIntervalMin = 3.0f;
	float m_shootingStarIntervalMax = 8.0f;

	float m_shootingStarTimer = 0.0f;
	float m_nextShootingStarWait = 0.0f;
	bool  m_enableShootingStar = true;

	bool  m_spawnStarOnEnterDetail = false;

	std::mt19937 m_rng;

	void UpdateShootingStar(float dt);
	void SpawnShootingStar();
	void EnsureDebugEffectOnMap();

	bool GetUIRect(ECS::EntityID id, float& left, float& top, float& right, float& bottom) const;

	std::vector<std::pair<ECS::EntityID, float>> m_activeEyeLights;
	float m_eyeLightTimer = 0.0f;
	float m_eyeLightNextInterval = 0.0f;
	void UpdateEyeLight(float dt);

	std::vector<ECS::EntityID> m_stageSpecificEntities;
	void CreateStageInfoUI(const std::string& stageID);
	void ClearStageInfoUI();
};

#endif // !___STAGE_SELECT_SCENE_H___
