//ResultScene.h
#pragma once
#include "Scene/Scene.h"
#include "ECS/ECS.h"
#include "Scene/SceneManager.h"
#include "Scene/TitleScene.h"	//遷移先
#include "Scene/GameScene.h"

struct ResultData
{
	float clearTime = 0.0f;         // クリアタイム
	int collectedCount = 0;         // 獲得アイテム数
	int totalItems = 0;             // 総アイテム数
	bool isCleared = false;         // クリアしたか（falseならゲームオーバー）
	bool wasSpotted = false;        // 見つかったか
	bool clearedInTime = false;     // 制限時間内か
	bool collectedAllOrdered = false; // 順序通りコンプリートしたか
	std::string stageID = "";       // プレイしたステージID (リトライ用)
};

class ResultScene :public Scene
{
public:
	ResultScene() = default;
	void Init()override;
	void Uninit() override;
	void Update(float deltaTime) override;
	void Draw() override;

	void CreateTimeDisplay(float time, DirectX::XMFLOAT2 pos);

	void CreateButtons();

	static void SetResultData(const ResultData& data) { s_resultData = data; }

	static bool isClear;
	static int finalItenCount;

private:
	std::shared_ptr<ECS::Coordinator> m_coordinator;
	static ResultData s_resultData;
};