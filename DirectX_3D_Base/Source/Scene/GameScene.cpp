/*****************************************************************//**
 * @file	GameScene.cpp
 * @brief
 *
 * @details
 *
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 *
 * @date	2025/11/30	쐬
 * 			ƓeF	- ǉF
 *
 * @update	2025/xx/xx	ŏIXV
 * 			ƓeF	- XXF
 *
 * @note	iȗj
 *********************************************************************/

#include "Scene/GameScene.h"

#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"
#include "Systems/Input.h"
#include <DirectXMath.h>
#include <iostream>
#include <typeindex> // SystemManagerRenderSystem擾Ɏgp
#include <sstream> 
#include <vector>
#include <limits>
#include <cmath>
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <random>

using namespace DirectX;

namespace
{
    struct AABB
    {
        DirectX::XMFLOAT3 c; // center
        DirectX::XMFLOAT3 e; // half extents
    };

    static bool AABBOverlap(const AABB& a, const AABB& b)
    {
        return (fabsf(a.c.x - b.c.x) <= (a.e.x + b.e.x)) &&
            (fabsf(a.c.y - b.c.y) <= (a.e.y + b.e.y)) &&
            (fabsf(a.c.z - b.c.z) <= (a.e.z + b.e.z));
    }

    static bool PointInsideAABB(const DirectX::XMFLOAT3& p, const AABB& b, float margin = 0.0f)
    {
        return (fabsf(p.x - b.c.x) <= (b.e.x + margin)) &&
            (fabsf(p.y - b.c.y) <= (b.e.y + margin)) &&
            (fabsf(p.z - b.c.z) <= (b.e.z + margin));
    }

    // Robust segment vs AABB test (slab method). Treats p0->p1 as a segment (t in [0,1]).
    static bool SegmentIntersectsAABB(const DirectX::XMFLOAT3& p0, const DirectX::XMFLOAT3& p1, const AABB& b, float padding = 0.0f)
    {
        const float minX = b.c.x - (b.e.x + padding);
        const float maxX = b.c.x + (b.e.x + padding);
        const float minY = b.c.y - (b.e.y + padding);
        const float maxY = b.c.y + (b.e.y + padding);
        const float minZ = b.c.z - (b.e.z + padding);
        const float maxZ = b.c.z + (b.e.z + padding);

        const DirectX::XMFLOAT3 d{ p1.x - p0.x, p1.y - p0.y, p1.z - p0.z };
        float tmin = 0.0f;
        float tmax = 1.0f;

        auto slab = [&](float p, float dp, float mn, float mx) -> bool
            {
                const float eps = 1e-6f;
                if (fabsf(dp) < eps)
                {
                    return (p >= mn && p <= mx);
                }
                const float ood = 1.0f / dp;
                float t1 = (mn - p) * ood;
                float t2 = (mx - p) * ood;
                if (t1 > t2) std::swap(t1, t2);
                tmin = std::max(tmin, t1);
                tmax = std::min(tmax, t2);
                return (tmin <= tmax);
            };

        if (!slab(p0.x, d.x, minX, maxX)) return false;
        if (!slab(p0.y, d.y, minY, maxY)) return false;
        if (!slab(p0.z, d.z, minZ, maxZ)) return false;
        return true;
    }

    struct GridKey
    {
        int x;
        int z;
        bool operator==(const GridKey& o) const { return x == o.x && z == o.z; }
    };

    struct GridKeyHash
    {
        size_t operator()(const GridKey& k) const noexcept
        {
            // Simple 2D hash
            return (static_cast<size_t>(k.x) * 73856093u) ^ (static_cast<size_t>(k.z) * 19349663u);
        }
    };

    static bool IsBlockedBetween(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b, const std::vector<AABB>& blockers)
    {
        for (const auto& blk : blockers)
        {
            // Use segment test so we don't miss blockers when the midpoint isn't inside the AABB.
            if (SegmentIntersectsAABB(a, b, blk, 0.25f))
                return true;
        }
        return false;
    }

    static DirectX::XMFLOAT3 ChooseMapGimmickPosition(ECS::Coordinator* coordinator)
    {
        // Default fallback (old hardcoded value)
        DirectX::XMFLOAT3 fallback{ 7.5f, 0.0f, 2.5f };

        if (!coordinator) return fallback;

        // Player position (prefer placing the gimmick on a reachable tile near the player)
        DirectX::XMFLOAT3 playerPos = fallback;
        {
            ECS::EntityID playerID = ECS::FindFirstEntityWithComponent<PlayerControlComponent>(coordinator);
            if (playerID != ECS::INVALID_ENTITY_ID && coordinator->HasComponent<TransformComponent>(playerID))
            {
                playerPos = coordinator->GetComponent<TransformComponent>(playerID).position;
            }
        }

        // Gimmick collider half-extents (CreateMapGimmick uses Collision size = 2.5)
        const DirectX::XMFLOAT3 gimmickHalf{ 2.5f, 2.5f, 2.5f };

        // Prefer a tile not too close / not too far (in meters)
        const float minDist = 10.0f;  // about 2 tiles if tileSize=5
        const float maxDist = 60.0f;  // avoid placing too deep if you want it early
        const float minDistSq = minDist * minDist;
        const float maxDistSq = maxDist * maxDist;

        // Collect candidates, then pick one at random (but still safe/reachable).
        std::vector<DirectX::XMFLOAT3> candidatesPreferred;
        std::vector<DirectX::XMFLOAT3> candidatesAll;
        candidatesPreferred.reserve(128);
        candidatesAll.reserve(256);

        // Pre-collect all blocking AABBs (walls/doors)
        std::vector<AABB> blockers;
        blockers.reserve(256);

        for (auto const& e : coordinator->GetActiveEntities())
        {
            if (!coordinator->HasComponent<TagComponent>(e)) continue;
            if (!coordinator->HasComponent<TransformComponent>(e)) continue;
            if (!coordinator->HasComponent<CollisionComponent>(e)) continue;

            const auto& tag = coordinator->GetComponent<TagComponent>(e).tag;
            if (tag != "wall" && tag != "door") continue;

            const auto& tr = coordinator->GetComponent<TransformComponent>(e);
            const auto& col = coordinator->GetComponent<CollisionComponent>(e);

            AABB b;
            b.c = { tr.position.x + col.offset.x, tr.position.y + col.offset.y, tr.position.z + col.offset.z };
            b.e = col.size;
            blockers.push_back(b);
        }

        // --- Build reachable ground set from player (avoid placing behind locked doors) ---
        // Tile size: your config uses 5.0, but we infer it from the level too.
        float tileSize = 5.0f;
        {
            // Try to infer tile step from ground positions (min non-zero delta).
            float best = std::numeric_limits<float>::infinity();
            std::vector<float> xs;
            xs.reserve(256);
            for (auto const& e : coordinator->GetActiveEntities())
            {
                if (!coordinator->HasComponent<TagComponent>(e)) continue;
                if (coordinator->GetComponent<TagComponent>(e).tag != "ground") continue;
                if (!coordinator->HasComponent<TransformComponent>(e)) continue;
                xs.push_back(coordinator->GetComponent<TransformComponent>(e).position.x);
            }
            // Compare neighbors (O(n^2) small enough for 15x15)
            for (size_t i = 0; i < xs.size(); ++i)
            {
                for (size_t j = i + 1; j < xs.size(); ++j)
                {
                    float d = fabsf(xs[i] - xs[j]);
                    if (d > 0.001f && d < best) best = d;
                }
            }
            if (std::isfinite(best) && best > 0.1f && best < 100.0f) tileSize = best;
        }

        // Map: grid coord -> world pos
        // NOTE: Ground centers in this project are often placed at (tileSize/2) offset (e.g. 2.5, 7.5, 12.5 ...).
        // Using raw (pos / tileSize) will produce half-integer indices and break connectivity.
        // We anchor the grid to the minimum ground position so indices become 0..N.
        float baseX = 0.0f;
        float baseZ = 0.0f;
        bool hasBase = false;

        for (auto const& e : coordinator->GetActiveEntities())
        {
            if (!coordinator->HasComponent<TagComponent>(e)) continue;
            if (coordinator->GetComponent<TagComponent>(e).tag != "ground") continue;
            if (!coordinator->HasComponent<TransformComponent>(e)) continue;

            const auto p = coordinator->GetComponent<TransformComponent>(e).position;
            if (!hasBase)
            {
                baseX = p.x;
                baseZ = p.z;
                hasBase = true;
            }
            else
            {
                baseX = std::min(baseX, p.x);
                baseZ = std::min(baseZ, p.z);
            }
        }

        std::unordered_map<GridKey, DirectX::XMFLOAT3, GridKeyHash> groundCells;
        groundCells.reserve(512);

        auto ToGrid = [tileSize, baseX, baseZ](const DirectX::XMFLOAT3& p) -> GridKey
            {
                return GridKey{ (int)lrintf((p.x - baseX) / tileSize), (int)lrintf((p.z - baseZ) / tileSize) };
            };

        for (auto const& e : coordinator->GetActiveEntities())
        {
            if (!coordinator->HasComponent<TagComponent>(e)) continue;
            if (coordinator->GetComponent<TagComponent>(e).tag != "ground") continue;
            if (!coordinator->HasComponent<TransformComponent>(e)) continue;

            DirectX::XMFLOAT3 p = coordinator->GetComponent<TransformComponent>(e).position;
            p.y = 0.0f;
            groundCells[ToGrid(p)] = p;
        }

        // Find start cell = nearest ground to player
        GridKey start{ 0,0 };
        bool hasStart = false;
        float bestD = std::numeric_limits<float>::infinity();
        for (const auto& kv : groundCells)
        {
            const auto& p = kv.second;
            float dx = p.x - playerPos.x;
            float dz = p.z - playerPos.z;
            float d2 = dx * dx + dz * dz;
            if (d2 < bestD)
            {
                bestD = d2;
                start = kv.first;
                hasStart = true;
            }
        }

        std::unordered_map<GridKey, bool, GridKeyHash> reachable;
        reachable.reserve(512);
        if (hasStart)
        {
            std::queue<GridKey> q;
            q.push(start);
            reachable[start] = true;

            const int dx4[4] = { 1, -1, 0, 0 };
            const int dz4[4] = { 0, 0, 1, -1 };

            while (!q.empty())
            {
                GridKey cur = q.front();
                q.pop();

                auto itCur = groundCells.find(cur);
                if (itCur == groundCells.end()) continue;
                const auto& pCur = itCur->second;

                for (int i = 0; i < 4; ++i)
                {
                    GridKey nb{ cur.x + dx4[i], cur.z + dz4[i] };
                    if (reachable.find(nb) != reachable.end()) continue;

                    auto itNb = groundCells.find(nb);
                    if (itNb == groundCells.end()) continue;

                    const auto& pNb = itNb->second;
                    // If there's a wall/door between these two tiles, do not connect.
                    if (IsBlockedBetween(pCur, pNb, blockers)) continue;

                    reachable[nb] = true;
                    q.push(nb);
                }
            }
        }

        // Scan all reachable ground tiles and collect safe candidates
        for (auto const& e : coordinator->GetActiveEntities())
        {
            if (!coordinator->HasComponent<TagComponent>(e)) continue;
            const auto& tag = coordinator->GetComponent<TagComponent>(e).tag;
            if (tag != "ground") continue;

            if (!coordinator->HasComponent<TransformComponent>(e)) continue;

            auto pos = coordinator->GetComponent<TransformComponent>(e).position;
            pos.y = 0.0f; // keep on floor

            // Reachability filter: same connected component as the player
            if (hasStart)
            {
                const auto gk = ToGrid(pos);
                if (reachable.find(gk) == reachable.end()) continue;
            }

            const float dx = pos.x - playerPos.x;
            const float dz = pos.z - playerPos.z;
            const float distSq = dx * dx + dz * dz;

            // Preferred window
            const bool preferred = (distSq >= minDistSq && distSq <= maxDistSq);

            AABB g;
            g.c = pos;
            g.e = gimmickHalf;

            bool hit = false;
            for (const auto& b : blockers)
            {
                if (AABBOverlap(g, b))
                {
                    hit = true;
                    break;
                }
            }
            if (hit) continue;

            candidatesAll.push_back(pos);
            if (preferred) candidatesPreferred.push_back(pos);
        }

        // Pick random from preferred set (fallback to all)
        static std::mt19937 rng{ std::random_device{}() };

        auto pickRandom = [&](const std::vector<DirectX::XMFLOAT3>& v) -> DirectX::XMFLOAT3
            {
                if (v.empty()) return fallback;
                std::uniform_int_distribution<size_t> dist(0, v.size() - 1);
                return v[dist(rng)];
            };

        if (!candidatesPreferred.empty())
            return pickRandom(candidatesPreferred);

        if (!candidatesAll.empty())
            return pickRandom(candidatesAll);

        return fallback;

    }
}


// ===== ÓIo[ϐ̒` =====u
ECS::Coordinator* GameScene::s_coordinator = nullptr;
std::string GameScene::s_StageNo = "";

void GameScene::Init()
{
    // ECS
    m_coordinator = std::make_shared<ECS::Coordinator>();
    s_coordinator = m_coordinator.get();
    ECS::ECSInitializer::InitECS(m_coordinator);

    // --- 3. JSONRtBOgĈꌂI ---
    ECS::EntityFactory::GenerateStageFromConfig(m_coordinator.get(), s_StageNo);

    // Map-check gimmick (green cube). Touch it in ACTION_MODE to force TopView.
    // Position is a simple default near the assumed start; adjust if needed.
    ECS::EntityFactory::CreateMapGimmick(m_coordinator.get(), ChooseMapGimmickPosition(m_coordinator.get()));



    // --- 4. ̑̋Entity̍쐬 ---
    //ECS::EntityFactory::CreateAllDemoEntities(m_coordinator.get());

    ECS::EntityID gameController = ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator.get());
    auto& gameState = m_coordinator->GetComponent<GameStateComponent>(gameController);

    // 1. gbvr[p
    ECS::EntityID scoutingBGM = ECS::EntityFactory::CreateLoopSoundEntity(
        m_coordinator.get(),
        "BGM_TEST",
        0.5f
    );
    // ^O "BGM_SCOUTING" ɕύX
    if (m_coordinator->HasComponent<TagComponent>(scoutingBGM)) {
        m_coordinator->GetComponent<TagComponent>(scoutingBGM).tag = "BGM_SCOUTING";
    }

    // 2. ANVp
    ECS::EntityID actionBGM = ECS::EntityFactory::CreateLoopSoundEntity(
        m_coordinator.get(),
        "BGM_TEST2",
        0.5f
    );
    // ^O "BGM_ACTION" ɕύX
    if (m_coordinator->HasComponent<TagComponent>(actionBGM)) {
        m_coordinator->GetComponent<TagComponent>(actionBGM).tag = "BGM_ACTION";
    }

    // ANVp͎~߂Ă
    if (m_coordinator->HasComponent<SoundComponent>(actionBGM)) {
        m_coordinator->GetComponent<SoundComponent>(actionBGM).RequestStop();
    }

    ECS::EntityID topviewBG = m_coordinator->CreateEntity(
        TransformComponent(
            /* Position	*/	XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 200000.0f),
            /* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
            /* Scale	*/	XMFLOAT3(SCREEN_WIDTH, SCREEN_HEIGHT, 1)
        ),
        UIImageComponent(
            /* AssetID		*/	"BG_TOPVIEW",
            /* Depth		*/	-1.0f,
            /* IsVisible	*/	true
        )
    );
    gameState.topviewBgID = topviewBG;

    ECS::EntityID tpsBG = m_coordinator->CreateEntity(
        TransformComponent(
            /* Position	*/	XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f),
            /* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
            /* Scale	*/	XMFLOAT3(SCREEN_WIDTH, SCREEN_HEIGHT, 1)
        ),
        UIImageComponent(
            /* AssetID		*/	"UI_SCAN_LINE",
            /* Depth		*/	-1.0f,
            /* IsVisible	*/	false,
            /* Color		*/	XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)
        )
    );
    gameState.tpsBgID = tpsBG;

    // ʕς̍ג_
    float lineWidth = (float)SCREEN_WIDTH;
    float lineHeight = 5.0f; // ̑

    // XLC
    ECS::EntityID scanLine = m_coordinator->CreateEntity(
        TransformComponent(
            /* Position	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
            /* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
            /* Scale	*/	XMFLOAT3(lineWidth, lineHeight, 1)
        ),
        UIImageComponent(
            /* AssetID		*/	"UI_SCAN_LINE",
            /* Depth		*/	2.0f,
            /* IsVisible	*/	true,
            /* Color		*/	XMFLOAT4(0.0f, 1.0f, 0.0f, 0.5f)
        ),
        ScanLineComponent(
            /* Speed	*/	300.0f,
            /* Start	*/	0.0f,
            /* End		*/	(float)SCREEN_HEIGHT
        )
    );

    // fW^Obh
    ECS::EntityID grid = m_coordinator->CreateEntity(
        TransformComponent(
            /* Position	*/	XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 0.0f),
            /* Rotation	*/	XMFLOAT3(0.0f, 0.0f, 0.0f),
            /* Scale	*/	XMFLOAT3(SCREEN_WIDTH, SCREEN_HEIGHT, 1)
        ),
        UIImageComponent(
            /* AssetID		*/	"UI_SCAN_LINE",
            /* Depth		*/	5.0f,
            /* IsVisible	*/	true,
            /* Color		*/	XMFLOAT4(0.0f, 1.0f, 1.0f, 0.1f)
        )
    );

    m_isFadeIn = false;
    m_fadeTimer = 0.0f;

    m_fadeEntity = m_coordinator->CreateEntity(
        TransformComponent(
            XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 200000.0f),
            XMFLOAT3(0.0f, 0.0f, 0.0f),
            XMFLOAT3(SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f)
        ),
        UIImageComponent(
            "BG_GAME_OVER",
            200000.0f,
            true,
            XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f)
        ),
        TagComponent("SCREEN_FADE")
    );

    // ------------------------------
    // gbvr[JñtF[hCi\j
    // ------------------------------
    m_isFadeIn = true;
    m_fadeTimer = 0.0f;

    // tXN[iUI_WHITE ŏZĎgzj
    m_fadeEntity = m_coordinator->CreateEntity(
        TransformComponent(
            XMFLOAT3(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f, 200000.0f),
            XMFLOAT3(0.0f, 0.0f, 0.0f),
            XMFLOAT3(SCREEN_WIDTH, SCREEN_HEIGHT, 1.0f)
        ),
        UIImageComponent(
            "UI_WHITE",
            200000.0f,
            true,
            XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) // ESsX^[g
        )
    );



//#ifdef _DEBUG
//    // ★デバッグ: 開始時にアイテムを全獲得状態にする
//    {
//        // 1. トラッカー(進行管理)を取得し、カウントを最大にする
//        ECS::EntityID trackerID = ECS::FindFirstEntityWithComponent<ItemTrackerComponent>(m_coordinator.get());
//        if (trackerID != ECS::INVALID_ENTITY_ID)
//        {
//            auto& tracker = m_coordinator->GetComponent<ItemTrackerComponent>(trackerID);
//
//            // 現在の獲得数を総数と同じにする
//            tracker.collectedItems = tracker.totalItems;
//
//            // ターゲット順序も完了状態にしておく（順序ありモード対策）
//            tracker.currentTargetOrder = tracker.totalItems + 1;
//
//            // 2. 画面上のアイテム(Collectable)を全て物理的に消去する
//            // (ループ中に削除すると不具合が出る可能性があるため、一度リストアップしてから削除)
//            std::vector<ECS::EntityID> itemsToRemove;
//            for (auto const& entity : m_coordinator->GetActiveEntities())
//            {
//                if (m_coordinator->HasComponent<CollectableComponent>(entity))
//                {
//                    itemsToRemove.push_back(entity);
//                }
//            }
//
//            for (auto entity : itemsToRemove)
//            {
//                m_coordinator->DestroyEntity(entity);
//            }
//
//            std::cout << "[DEBUG] Init: All items forced collected (" << tracker.collectedItems << "/" << tracker.totalItems << ")" << std::endl;
//        }
//    }
//#endif


}

void GameScene::Uninit()
{
    auto effectSystem = ECS::ECSInitializer::GetSystem<EffectSystem>();
    if (effectSystem)
    {
        effectSystem->Uninit();
    }

    ECS::ECSInitializer::UninitECS();

    m_coordinator.reset();
    s_coordinator = nullptr;
}

void GameScene::Update(float deltaTime)
{
    m_coordinator->UpdateSystems(deltaTime);

#ifdef _DEBUG
    // デバッグ：Cキーでクリア扱い（GameControlSystem が ResultScene へ遷移する）
    if (IsKeyTrigger('C'))
    {
        ECS::EntityID gameController =
            ECS::FindFirstEntityWithComponent<GameStateComponent>(m_coordinator.get());
        auto& gameState = m_coordinator->GetComponent<GameStateComponent>(gameController);

        gameState.isGameClear = true;   // ✅ これが正しい 
        gameState.isGameOver = false;  // 念のため
    }
#endif


    UpdateFadeIn(deltaTime);
}

void GameScene::UpdateFadeIn(float deltaTime)
{
    if (!m_isFadeIn) return;
    if (m_fadeEntity == ECS::INVALID_ENTITY_ID) { m_isFadeIn = false; return; }
    if (!m_coordinator || !m_coordinator->HasComponent<UIImageComponent>(m_fadeEntity)) { m_isFadeIn = false; return; }

    m_fadeTimer += deltaTime;

    float t = (m_fadeInDuration <= 0.0f) ? 1.0f : (m_fadeTimer / m_fadeInDuration);
    if (t > 1.0f) t = 1.0f;
    if (t < 0.0f) t = 0.0f;

    // SmoothStep: t^2(3-2t)
    float eased = t * t * (3.0f - 2.0f * t);
    float alpha = 1.0f - eased; // 10

    auto& img = m_coordinator->GetComponent<UIImageComponent>(m_fadeEntity);
    img.isVisible = true;
    img.color = XMFLOAT4(0.0f, 0.0f, 0.0f, alpha);

    if (t >= 1.0f)
    {
        m_isFadeIn = false;
        img.isVisible = false;
    }
}


void GameScene::Draw()
{
    if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
    {
        system->Render(true);
    }

    if (auto system = ECS::ECSInitializer::GetSystem<RenderSystem>())
    {
        system->DrawSetup();
        system->DrawEntities();
    }

    if (auto system = ECS::ECSInitializer::GetSystem<EffectSystem>())
    {
        system->Render();
    }

    if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
    {
        system->Render(false);
    }
}