/*****************************************************************//**
 * @file	StageSelectScene.h
 * @brief	ステージセレクトのメインロジックを含むシーンクラス
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
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

#ifndef ___STAGE_SELECT_SCENE_H___
#define ___STAGE_SELECT_SCENE_H___

// ===== インクルード =====
#include "Scene/Scene.h"
#include "ECS/Coordinator.h"

#include <memory>
#include <map>

// ステージ情報の構造体
struct StageData {
	std::string name;
	std::string imageID;      // ステージイメージ画像のID
	float timeLimitStar;      // 目標タイム

	std::vector<std::string> items; // アイテムIDリスト

	struct GimmickInfo {
		std::string type;
		int count;
	};
	std::vector<GimmickInfo> gimmicks; // ギミック情報
};

/**
 * @class	StageSelectScene
 * @brief	ステージセレクトロジックとECSを管理するシーン
 */
class StageSelectScene
	: public Scene
{
private:
	// ECSの中心となるコーディネーター (シーンがECSのライフサイクルを管理)
	std::shared_ptr<ECS::Coordinator> m_coordinator;

	// ECSのグローバルアクセス用 (SystemなどがECS操作を行うための窓口)
	static ECS::Coordinator* s_coordinator;

public:
	// コンストラクタとデストラクタ（Sceneを継承しているため仮想デストラクタはScene側で定義済みと仮定）
	StageSelectScene()
		: m_coordinator(nullptr)
	{
	}
	~StageSelectScene() override {} // 仮想デストラクタを実装

	// Sceneインターフェースの実装
	void Init() override;
	void Uninit() override;
	void Update(float deltaTime) override;
	void Draw() override;


	/**
	 * @brief Coordinatorインスタンスへのポインタを取得する静的アクセサ
	 * @return ECS::Coordinator* - 現在アクティブなシーンのCoordinator
	 */
	static ECS::Coordinator* GetCoordinator() { return s_coordinator; }

private:
	// JSONデータの読み込み
	void LoadStageData();
	// UIの表示切り替え
	void SwitchState(bool toDetail);
	void CreateDetailUI();

	// データ
	std::map<std::string, StageData> m_stageDataMap;
	std::string m_selectedStageID;

	// エンティティ管理
	std::vector<ECS::EntityID> m_listUIEntities;   // 一覧画面のボタン等
	std::vector<ECS::EntityID> m_detailUIEntities; // 詳細画面の表示物

};

#endif // !___STAGE_SELECT_SCENE_H___