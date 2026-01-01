/*****************************************************************//**
 * @file	AnimationComponent.h
 * @brief	モデルのアニメーション再生に必要な状態を定義するComponent。
 * 
 * @details	
 * 再生中のアニメーションID、再生時間などを保持する。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/23	初回作成日
 * 			作業内容：	- 追加：アニメーション再生状態を管理する`AnimationComponent`を作成
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___ANIMATION_COMPONENT_H___
#define ___ANIMATION_COMPONENT_H___

// ===== インクルード =====
#include <string>
#include <vector>

/**
 * @struct	AnimationComponent
 * @brief	アニメーション再生状態
 */
struct AnimationComponent
{
    // --- データ定義 ---

    // 1. ロード予約リスト
    std::vector<std::string> preloadList;

    // 2. 再生リクエスト情報構造体
    struct PlayRequest {
        std::string animeID;
        bool loop;
        float speed;
        bool isBlend;
        float blendTime;

        // コンストラクタ
        PlayRequest()
            : animeID(""), loop(true), speed(1.0f), isBlend(false), blendTime(0.0f) {
        }

        PlayRequest(std::string id, bool l, float s, bool blend, float time)
            : animeID(id), loop(l), speed(s), isBlend(blend), blendTime(time) {
        }
    };

    PlayRequest currentRequest;
    bool hasRequest = false; // std::optionalの代わりのフラグ

    // --- コンストラクタ ---
    AnimationComponent() : hasRequest(false) {}

    AnimationComponent(std::initializer_list<std::string> animeIDs)
        : hasRequest(false)
    {
        preloadList.insert(preloadList.end(), animeIDs.begin(), animeIDs.end());
    }

    // --- ヘルパー関数 ---

    // 事前にロードしておきたいアニメーションIDを登録
    void RegisterAnimation(const std::string& animeID)
    {
        preloadList.push_back(animeID);
    }

    void RegisterAnimations(std::initializer_list<std::string> animeIDs)
    {
        preloadList.insert(preloadList.end(), animeIDs.begin(), animeIDs.end());
    }

    // 再生命令
    void Play(const std::string& animeID, bool loop = true, float speed = 1.0f)
    {
        currentRequest = PlayRequest(animeID, loop, speed, false, 0.0f);
        hasRequest = true;
    }

    // ブレンド再生命令
    void PlayBlend(const std::string& animeID, float blendTime, bool loop = true, float speed = 1.0f)
    {
        currentRequest = PlayRequest(animeID, loop, speed, true, blendTime);
        hasRequest = true;
    }
};

// Component登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(AnimationComponent)

#endif // !___ANIMATION_COMPONENT_H___