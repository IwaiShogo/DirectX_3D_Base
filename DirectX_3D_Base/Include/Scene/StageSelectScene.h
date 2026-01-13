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

	// ===== UI Select FX（OK / BACK）=====
	struct UISelectFxInstance
	{
		ECS::EntityID entity = (ECS::EntityID)-1;
		float remaining = 0.0f; // 手動寿命（秒）
	};
	std::vector<UISelectFxInstance> m_uiSelectFx;

	void UpdateUISelectFx(float dt);
	// Hover中だけボタンを少し拡大（UIButtonSystem側の演出が無い/効かない場合の保険）
	void UpdateButtonHoverScale(float dt);
	std::unordered_map<ECS::EntityID, DirectX::XMFLOAT3> m_buttonBaseScale;


	// ===== Card Focus Animation（ボタン押下→カードが画面中央へ出てくる）=====
	struct CardFocusAnim
	{
		bool active = false;
		ECS::EntityID entity = (ECS::EntityID)-1;

		float elapsed = 0.0f;
		float duration = 0.45f; // ここを短く/長くすると「中央へ来る速さ」が変わる

		DirectX::XMFLOAT3 startPos = { 0,0,0 };
		DirectX::XMFLOAT3 endPos = { 0,0,0 };
		DirectX::XMFLOAT3 startScale = { 1,1,1 };
		DirectX::XMFLOAT3 endScale = { 1,1,1 };

		DirectX::XMFLOAT3 baseRot = { 0,0,0 }; // 通常は (0, PI, 0)
		float extraRollRad = 0.0f;             // 追加で少しだけZ回転（演出）
	};
	CardFocusAnim m_cardFocus;

	void StartCardFocusAnim(ECS::EntityID cardEntity, const DirectX::XMFLOAT3& uiPos);
	void UpdateCardFocusAnim(float dt);

	// 毎回新規作成する「集中カード」(情報画面へ行く際の回転/拡大演出用)
	ECS::EntityID m_focusCardEntity = (ECS::EntityID)-1;
	void DestroyFocusCard();

	ECS::EntityID m_listBgEntity = (ECS::EntityID)-1;

	// データ
	void LoadStageData();
	std::map<std::string, StageData> m_stageDataMap;
	std::string m_selectedStageID;

	// UI
	void SwitchState(bool toDetail);
	// 一覧カードの「スロット中心座標」を返す（押下時の一瞬のズレ対策用）
	DirectX::XMFLOAT3 GetListCardSlotCenterPos(int stageNo) const;
	// ★画面(一覧↔詳細)の切替時や再入場時に、演出状態を必ず初期化する
	// keepFocusCard=true の場合は、フォーカスカードの Destroy を呼ばない（EntityID 再利用によるアニメ状態持ち越し対策）
	void ResetSelectToDetailAnimState(bool unlockInput = false, bool keepFocusCard = false);
	void SetUIVisible(ECS::EntityID id, bool visible);
	// ★UIエンティティの中心にワンショットエフェクトを出す
	void PlayUISelectEffect(ECS::EntityID uiEntity, const std::string& effectId, float scale);
	// START / FINISH 押下エフェクト用
	ECS::EntityID m_startBtnEntity = (ECS::EntityID)-1;
	ECS::EntityID m_finishBtnEntity = (ECS::EntityID)-1;

	// UIエンティティの中心にワンショットエフェクトを出す
// 既存のフェードを呼ぶための接続口（既にあるならそれを使う）
// 既存のシーン切替関数（既にあるならそれを使う）
	std::vector<ECS::EntityID> m_listUIEntities;
	std::vector<ECS::EntityID> m_detailUIEntities;

	// ===== 詳細UIを「一覧の上に重ねる」ための補助 =====
	ECS::EntityID m_lastHiddenListCardEntity = (ECS::EntityID)-1; // 集中演出で一時的に隠したカード
	bool m_detailAppearActive = false;
	float m_detailAppearTimer = 0.0f;
	float DETAIL_APPEAR_DURATION = 0.25f; // 詳細UIがふわっと出るまでの時間
	std::unordered_map<ECS::EntityID, DirectX::XMFLOAT3> m_detailBaseScale;
	std::unordered_map<ECS::EntityID, DirectX::XMFLOAT3> m_detailBasePos;
	void CacheDetailBaseTransform(ECS::EntityID id);
	void BeginDetailAppear();
	void UpdateDetailAppear(float dt);


	// カーソルは常駐（消さない）
	ECS::EntityID m_cursorEntity = (ECS::EntityID)-1;

	// ===== Screen Transition (System + Component) =====
	// どのシーンでも「呼ぶだけ」で使える共通フェード。
	// StageSelectScene 側の直書きフェードは撤去し、このEntityに依頼する。
	ECS::EntityID m_transitionEntity = (ECS::EntityID)-1;

	ECS::EntityID m_blackTransitionEntity = (ECS::EntityID)-1; // ゲーム遷移専用：全面黒フェード
	// 入力ロック（多重押し防止）。基本は遷移開始時に true、
	// 「同一シーン内で戻ってくる遷移」の完了コールバックで false。

	bool m_enableSlideFade = false;

	// ===== FadeManager（カメラ非依存のフェード）=====
	bool m_waitingSceneFadeIn = false;
	std::function<void()> m_onFadeOutComplete = nullptr;


	bool m_inputLocked = false;
	bool m_isWaitingForTransition = false;
	float m_transitionWaitTimer = 0.0f;
	float m_transitionDelayTime = 1.0f; // アニメーション開始から何秒後に遷移するか
	// 調整ポイント：一覧→情報（詳細）へ切り替わるまでの待ち時間（秒）
	float LIST_TO_DETAIL_DELAY = 1.50f; // 0.80=現状(0.45+0.35)相当
	// 調整ポイント：カード回転アニメ（A_CARD_COMEON）の再生速度
	float LIST_TO_DETAIL_ANIM_SPEED = 1.5f; // 1.0=等速, 2.0=2倍速
	std::string m_pendingStageID = "";  // 遷移予定のステージID

	void  KillAllShootingStars();

	// ===== Shooting Star（UI_STAGE_MAP内でたまに）=====
	ECS::EntityID m_stageMapEntity = (ECS::EntityID)-1;
	ECS::EntityID m_stageMapSiroEntity = (ECS::EntityID)-1; // 城(手前)

	// ===== Stage Map Texture (per-stage) =====
	int StageIdToStageNo(const std::string& stageId) const;
	std::string GetStageMapTextureAssetId(int stageNo) const;
	void ApplyStageMapTextureByStageId(const std::string& stageId);

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


	// ===== Stage Unlock / Reveal (Stage Select) =====
	// 起動時は Stage1 のみ表示。クリア後に次ステージを解放し、StageSelect復帰時に「浮かび上がり」演出を出す。
	int m_maxUnlockedStage = 1;      // 1..6
	int m_pendingRevealStage = -1;   // -1 or 2..6（今回の復帰で演出するステージ）
	// ★追加：既に解放済みは先に表示し、新規解放だけを後から浮かせる（遅延Reveal）
	int   m_scheduledRevealStage = -1; // -1 or 2..6
	float m_revealDelayTimer = 0.0f;


	// m_listUIEntities と同じ順（ST_001..ST_006）
	std::vector<int> m_listStageNos;

	struct StageRevealAnim
	{
		bool active = false;
		ECS::EntityID entity = (ECS::EntityID)-1;
		float elapsed = 0.0f;
		float duration = 0.90f; // 演出時間（秒）

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

	// ★追加: ゲーム開始遷移待ち用
	bool m_isWaitingForGameStart = false;

	// ★追加: 一覧へ戻る遷移待ち用
	bool m_isWaitingForBackToList = false;

	float m_gameStartTimer = 0.0f;
	const float GAME_START_DELAY = 1.0f; // ★ここで待機時間を調整（秒）

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