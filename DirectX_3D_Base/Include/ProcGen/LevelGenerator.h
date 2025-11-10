/*****************************************************************//**
 * @file	LevelGenerator.h
 * @brief	BSP/MSTアルゴリズムのコアデータ構造とユーティリティクラスの定義
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo / Uno Itsuki
 * ------------------------------------------------------------
 * 
 * @date	2025/11/06	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/11/10	最終更新日
 * 			作業内容：	- 追加：BSPParamsに aspectRatioThreshold を追加し、BSP分割の偏り制御をパラメータ化。
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___LEVEL_GENERATOR_H___
#define ___LEVEL_GENERATOR_H___

// ===== インクルード =====
#include <vector>
#include <random>
#include <cstdint>
#include <DirectXMath.h>
#include "Systems/Geometory.h" // DebugDrawでAddLineを使用するため

namespace ProcGen {

    // --- データ構造 ---
    struct RectF {
        float x, y, w, h; // 左上(x,y)、幅w、高さh（右手系XZに対応）
    };

    struct Segment {
        DirectX::XMFLOAT3 a;
        DirectX::XMFLOAT3 b;
        float width = 2.0f; // 通路の幅
    };

    struct ConnectionInfo {
        float start = 0.0f; // 辺に沿った開口部の開始位置（0.0～1.0）
        float end = 0.0f;   // 辺に沿った開口部の終了位置（0.0～1.0）
        bool isConnected = false;
    };

    struct Room {
        RectF rect;                 // 実際に部屋として使う矩形
        DirectX::XMFLOAT2 center;   // 中心（Delaunay用）

        // 接続情報（0:南(-Z), 1:北(+Z), 2:西(-X), 3:東(+X)）
        ConnectionInfo connections[4];
    };

    struct MuseumLayout {
        std::vector<Room>    rooms;      // BSPから抽出した部屋
        std::vector<Segment> corridors;  // MSTから決まった通路
    };

    struct GridMapping {
        float scaleXZ = 1.0f;
        float yFloor = 0.0f;
        float wallHeight = 2.5f; // Backroomsの天井高
    };

    // --- パラメータ構造体 ---
    struct BSPParams {
        float  minLeafW = 2.0f;     // 最小部屋サイズ
        float  minLeafH = 2.0f;
        float  maxAspect = 1.8f;    // 分割の偏りを決定するアスペクト比
        int    maxDepth = 5;        // 分割の回数（推奨：）
        float  roomInset = 1.5f;    // 部屋の壁からの余白
        float  aspectRatioThreshold = 1.3f;
    };
    struct DelaunayParams {};
    struct MSTParams { float extraEdgeRatio = 0.15f; }; // 回遊性

    // --- メインジェネレーター ---
    class LevelGenerator {
    public:
        LevelGenerator(uint32_t seed = 0);

        MuseumLayout GenerateMuseum(
            float areaW, float areaH,
            const BSPParams& bsp,
            const DelaunayParams& dela,
            const MSTParams& mst);

        void DebugDraw(const MuseumLayout& layout, const GridMapping& map) const;

    private:
        // --- BSP ---
        struct Leaf { RectF rc; int depth; };
        void SplitBSP(float W, float H, const BSPParams& p);
        void EmitRoomsFromLeaves(const BSPParams& p);

        // --- Delaunay（Bowyer–Watson） ---
        struct Tri { int i0, i1, i2; };
        struct Edge { int a, b; };
        bool Circumcircle(const DirectX::XMFLOAT2& p, const Tri& t, DirectX::XMFLOAT2& c, float& r2) const;
        void BuildDelaunay(const DelaunayParams& p);

        // --- MST（Kruskal） ---
        void BuildGraphEdges();              // Delaunayから無向グラフ辺列挙
        void BuildMST(const MSTParams& p);   // MST + 余剰辺

        // --- Corridors（L字分解） ---
        void BuildCorridors();

        std::mt19937                     m_rng;
        std::vector<Leaf>                m_leaves;
        std::vector<Room>                m_rooms;   // 生成済み部屋
        std::vector<DirectX::XMFLOAT2>   m_points;  // 部屋中心群
        std::vector<Tri>                 m_tris;    // Delaunay三角形
        std::vector<Edge>                m_edges;   // Delaunay由来のグラフ辺（重複排除後）
        std::vector<Edge>                m_mst;     // Kruskal結果（＋余剰）
        std::vector<Segment>             m_corridors;
    };

} // namespace ProcGen

#endif // !___LEVEL_GENERATOR_H___