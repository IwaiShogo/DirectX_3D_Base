/*****************************************************************//**
 * @file    LoadingScene.h
 * @brief   ロード画面（LOADING文字表示 + 右下UVアニメ）
 *********************************************************************/

#ifndef ___LOADING_SCENE_H___
#define ___LOADING_SCENE_H___

#include "Scene/Scene.h"
#include "ECS/ECS.h"

 // EntityHandle 推論のため、CreateEntity に渡すコンポーネント型を完全型で参照する
#include "ECS/Components/Core/TransformComponent.h"
#include "ECS/Components/UI/UIImageComponent.h"

#include <memory>
#include <utility> // std::declval
#include <vector>  // 追加：Entity管理用

class LoadingScene : public Scene
{
public:
    LoadingScene() = default;

    void Init() override;
    void Uninit() override;
    void Update(float deltaTime) override;
    void Draw() override;

private:
    /**
     * @brief CreateEntity() の戻り型を推論して保持（ECS::Entity が無い環境向け）
     * @details
     * ECS::Entity のような公開型が無い場合でも、CreateEntity() の戻り型をそのまま保持します。
     */
    using EntityHandle = decltype(std::declval<ECS::Coordinator>().CreateEntity(
        std::declval<TransformComponent>(),
        std::declval<UIImageComponent>()));

    /** @brief UI生成（EntityHandleを返す） */
    EntityHandle SpawnUI(const char* assetId,
        float x, float y,
        float w, float h,
        float depth,
        float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);

    /** @brief UI_LOAD_ANIM のUVフレームを設定（1枚を横30分割） */
    void SetLoadAnimFrame(int frameIndex);

private:
    std::shared_ptr<ECS::Coordinator> m_coordinator;

    float m_elapsed = 0.0f;
    float m_minDisplaySec = 1.0f;

    // ---- 追加: 文字アニメーション用データ ----
    std::vector<EntityHandle> m_textEntities; // 各文字のEntityID
    std::vector<float>        m_textBaseY;    // 各文字の基準Y座標

    // ---- 右下ロードアニメ（UVコマ送り） ----
    EntityHandle m_loadAnimEntity;
    bool  m_hasLoadAnim = false;

    float m_loadAnimTimer = 0.0f;
    int   m_loadAnimFrame = 0;

    // 1枚で30分割（横30×縦1）
    // enum の {} が環境によって構文エラーになるケースを避けるため、constexpr で定義
    static constexpr int kCols = 30;
    static constexpr int kRows = 1;
    static constexpr int kTotalFrames = kCols * kRows;

    static constexpr float kAnimFPS = 60.0f;
};

#endif // !___LOADING_SCENE_H___