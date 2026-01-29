/*****************************************************************//**
 * @file	GameStateComponent.h
 * @brief	ゲームの現在の状態（モード）を定義するComponent
 * 
 * @details	偵察モードとアクションモードの切り替えを管理する。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/06	初回作成日
 * 			作業内容：	- 追加：ゲームモードを定義する `GameMode` enumと、現在のモードを保持する `GameStateComponent` を作成。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/


#ifndef ___GAME_STATE_COMPONENT_H___
#define ___GAME_STATE_COMPONENT_H___

#include <DirectXMath.h>

/**
 * @enum GameMode
 * @brief ゲームの主要な状態: 偵察フェーズ/アクションフェーズ
 */
enum class GameMode
{
	SCOUTING_MODE, // 偵察フェーズ: トップビュー視点
	ACTION_MODE,   // アクションフェーズ: 三人称視点
};

enum class GameSequenceState
{
	None,
	Starting,	// TPS開始時の画像演出
	Entering,	// スタート演出
	Playing,	// 通常プレイ
	Exiting,	// 脱出演出
	Caught,		// 捕まった
	Taser
};
/**
 * @struct GameStateComponent
 * @brief ゲーム全体の状態を保持する
 */
struct GameStateComponent
{
	GameMode currentMode = GameMode::SCOUTING_MODE; ///< 現在のゲームモード

	ECS::EntityID topviewBgID = ECS::INVALID_ENTITY_ID;
	ECS::EntityID tpsBgID = ECS::INVALID_ENTITY_ID;

	// --- 時間管理 ---
	float timeLimit = 180.0f;	// 制限時間
	float timeLimitStar = 60.0f;	// 評価用目標タイム
	float elapsedTime = 0.0f;	// 経過時間

	// ゲームの終了状態を保持
	bool isGameOver = false;		// 警備員に追いつかれた
	bool isGameClear = false;		// アイテム全回収後に脱出地点に到達
	bool requestRestart = false;	// 次のフレームでリトライを要求
	bool isPlayerTrapped = false;
	float playerTrappedTimer = 0.0f;


	bool requestNextStage = false;	// 次のフレームで次のステージへ遷移を要求
	bool wasSpotted = false;		// 警備員に見つかったか
	bool isPaused = false;
	ECS::EntityID startLogoID = ECS::INVALID_ENTITY_ID;
	ECS::EntityID taserEffectEntity = 0;

	GameSequenceState sequenceState = GameSequenceState::None;
	float sequenceTimer = 0.0f;

	// コンストラクタ
	GameStateComponent(GameMode initialMode = GameMode::SCOUTING_MODE)
		: currentMode(initialMode) {}
};

#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(GameStateComponent)

#endif // !___GAME_STATE_COMPONENT_H___