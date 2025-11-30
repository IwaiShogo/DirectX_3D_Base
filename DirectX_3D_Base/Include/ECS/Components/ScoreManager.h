// ScoreManager.h
#ifndef ___SCENE_MANAGER___
#define ___SCENE_MANAGER___
#include <map>
#include <algorithm>

class ScoreManager {
public:
    // ステージごとのベストタイムを保存するマップ (ステージ番号, タイム秒)
    // タイムが 0.0f の場合は「未クリア」とみなす
    static std::map<int, float> s_bestTimes;

    // タイムを保存する（既存より速い場合のみ更新）
    static void SaveBestTime(int stageNo, float time) {
        if (s_bestTimes[stageNo] == 0.0f || time < s_bestTimes[stageNo]) {
            s_bestTimes[stageNo] = time;
        }
    }

    // ベストタイムを取得する
    static float GetBestTime(int stageNo) {
        return s_bestTimes[stageNo];
    }
};
#endif // !#endif // !___STAGE__INFORMATION_SCENE___
// .cppファイルで実体を定義する必要があります
// ScoreManager.cpp を作るか、Main.cppの上の方に以下を書いてください
// std::map<int, float> ScoreManager::s_bestTimes;