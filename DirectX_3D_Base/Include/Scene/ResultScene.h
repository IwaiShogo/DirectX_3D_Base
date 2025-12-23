// ResultScene.h
#pragma once
#include "Scene/Scene.h"
#include "ECS/ECS.h"
#include "Scene/SceneManager.h"
#include "Scene/TitleScene.h"
#include "Scene/GameScene.h"
#include <vector>      
#include <string>      

// GameControlSystem から渡されるリザルト情報
struct ResultData
{
    float clearTime = 0.0f;          // クリアタイム
    int   collectedCount = 0;        // 獲得アイテム数
    int   totalItems = 0;            // 総アイテム数
    bool  isCleared = false;         // クリアしたか（falseならゲームオーバー）
    bool  wasSpotted = false;        // 見つかったか
    bool  clearedInTime = false;     // 制限時間内か
    bool  collectedAllOrdered = false; // 順序通りコンプリートしたか
    std::string stageID = "";        // プレイしたステージID (リトライ用)

    // ★ すでにリザルト画面で使っている「取れたお宝だけ」のリスト
    std::vector<std::string> collectedItemIcons;

    // ★ ゲームオーバー用：ステージ内のお宝（順番どおり）全部
    //   orderedItemIcons[i] … i 番目のお宝のアイコン名
    //   orderedItemCollected[i] … そのお宝を取れたか？
    std::vector<std::string> orderedItemIcons;
    std::vector<bool>        orderedItemCollected;
};
class ResultScene : public Scene
{
public:
    ResultScene() = default;

    void Init() override;
    void Uninit() override;
    void Update(float deltaTime) override;
    void Draw() override;

    // 本の中央にタイム表示用の数字スプライトを並べる
    void CreateTimeDisplay(float time, DirectX::XMFLOAT2 pos);

    // 画面下部のボタン(RETRY / SELECT / TITLE)生成
    void CreateButtons();

    // GameControlSystem から ResultData を受け取るための窓口
    static void SetResultData(const ResultData& data) { s_resultData = data; }
    // 追加：StageSelect 等から読む用
    static const ResultData& GetResultData() { return s_resultData; }

    // 互換用（必要なら使う）
    static bool isClear;
    static int  finalItenCount;




private:
    std::shared_ptr<ECS::Coordinator> m_coordinator;
    static ResultData                 s_resultData;

    // 織田：ボタンエンティティのための変数
private:

    struct ButtonPair {
        ECS::EntityID textEntity; // ボタンの文字
        ECS::EntityID frameEntity; // 背景の枠
    };


    std::vector<ButtonPair> m_buttons;

    float m_elapsedTime = 0.0f; // アニメ―ジョン経過時間

    const float BUTTON_NORMAL_SCALE = 1.0f; // 通常時の倍率
    const float PULSE_CENTER_SCALE  = 1.15f; // 元気な時の倍率
    const float PULSE_AMPLITUDE     = 0.05f; // 揺れ幅
    const float PULSE_SPEED         = 10.0f; // 揺れの速さ
    const float LERP_SPEED          = 10.0f; // 変化の追従速度


};
