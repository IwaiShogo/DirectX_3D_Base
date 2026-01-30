// ResultScene.h
#pragma once
#include "Scene/Scene.h"
#include "ECS/ECS.h"
#include "Scene/SceneManager.h"
#include "Scene/TitleScene.h"
#include "Scene/GameScene.h"
#include <vector>      
#include <string>      

// GameControlSystem n郊Ug
struct ResultData
{
    float clearTime = 0.0f;          // NA^C
    int   collectedCount = 0;        // lACe
    int   totalItems = 0;            // ACe
    bool  isCleared = false;         // NAifalseȂQ[I[o[j
    bool  wasSpotted = false;        // 
    bool  clearedInTime = false;     // ԓ
    bool  collectedAllOrdered = false; // ʂRv[g
    std::string stageID = "";        // vCXe[WID (gCp)

    float timeLimitStar; //目標タイム 

    //  łɃUgʂŎgĂuꂽ󂾂ṽXg
    std::vector<std::string> collectedItemIcons;

    //  Q[I[o[pFXe[ŴiԂǂjS
    //   orderedItemIcons[i] c i Ԗڂ̂̃ACR
    //   orderedItemCollected[i] c ̂ꂽH
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

    // {̒Ƀ^C\p̐XvCgׂ
    void CreateTimeDisplay(float time, DirectX::XMFLOAT2 pos);
    void CreateNumberDisplay(int number, DirectX::XMFLOAT2 pos); // 整数をシンプルに表示する関数


    // ʉ̃{^(RETRY / SELECT / TITLE)
    void CreateButtons();

    // GameControlSystem  ResultData 󂯎邽߂̑
    static void SetResultData(const ResultData& data);
    // ǉFStageSelect ǂޗp
    static const ResultData& GetResultData() { return s_resultData; }

    // ݊piKvȂgj
    static bool isClear;
    static int  finalItenCount;

    static ResultData                 s_resultData;



private:
    std::shared_ptr<ECS::Coordinator> m_coordinator;
    bool m_isClear = false;
    ECS::EntityID m_gameoverBGM = ECS::INVALID_ENTITY_ID;
    // 直前のクリアで新規解放されたステージ番号（2..6）。演出はStageSelectへ戻るときだけ出す。
    static int                      s_newlyUnlockedStageNo;

    ECS::EntityID m_bgmEntity = ECS::INVALID_ENTITY_ID;
    ECS::EntityID m_DecisionSe = ECS::INVALID_ENTITY_ID;
    // DcF{^GeBeB̂߂̕ϐ
private:

    struct ButtonPair {
        ECS::EntityID textEntity; // {^̕
        ECS::EntityID frameEntity; // wi̘g
    };


    std::vector<ButtonPair> m_buttons;

    float m_elapsedTime = 0.0f; // Aj\Woߎ

    const float BUTTON_NORMAL_SCALE = 1.0f; // ʏ펞̔{
    const float PULSE_CENTER_SCALE = 1.15f; // CȎ̔{
    const float PULSE_AMPLITUDE = 0.05f; // hꕝ
    const float PULSE_SPEED = 10.0f; // h̑
    const float LERP_SPEED = 10.0f; // ω̒Ǐ]x


};
