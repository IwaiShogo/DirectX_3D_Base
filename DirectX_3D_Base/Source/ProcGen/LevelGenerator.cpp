/*****************************************************************//**
 * @file	LevelGenerator.cpp
 * @brief	BSP/MSTによる館内レイアウト生成アルゴリズムの実装
 * 
 * @details	BSPで部屋を配置し、Delaunay/MSTで接続性を保証した通路を生成する。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo / Uno Itsuki
 * ------------------------------------------------------------
 * 
 * @date	2025/11/06	初回作成日
 * 			作業内容：	- 追加：BSP/MSTの骨格実装
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#define NOMINMAX
#include "ProcGen/LevelGenerator.h"
#include "Systems/Geometory.h"      // AddLine用

#include <algorithm>
#include <numeric>                  // std::iota
#include <cmath>
#include <chrono>
#include <unordered_set>

using namespace DirectX;
using namespace ProcGen;
using std::vector;
using std::pair;

//--------------------------------------
// 無名名前空間：この .cpp 内だけで使う補助構造体・関数
//--------------------------------------
namespace {
    // BSP/MSTに必要な補助構造体（LevelGenerator::hに依存）
    struct Leaf { RectF rc; int depth; };
    struct Tri { int i0, i1, i2; };
    struct Edge { int a, b; }; // 辺の頂点インデックス

    // Union-Find (DSU) 構造体 (接続保証用)
    struct DSU {
        std::vector<int> p, r;
        DSU(int n = 0) { reset(n); }
        void reset(int n) { p.resize(n); r.assign(n, 0); std::iota(p.begin(), p.end(), 0); }
        int find(int x) { return p[x] == x ? x : p[x] = find(p[x]); }
        bool unite(int a, int b) {
            a = find(a); b = find(b); if (a == b) return false;
            if (r[a] < r[b]) std::swap(a, b);
            p[b] = a; if (r[a] == r[b]) ++r[a]; return true;
        }
    };

    // 乱数ヘルパー
    inline float Rand01(std::mt19937& rng) {
        return std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
    }
}

//--------------------------------------
// LevelGenerator 本体
//--------------------------------------

LevelGenerator::LevelGenerator(uint32_t seed)
    : m_rng(seed == 0 ? (uint32_t)std::chrono::steady_clock::now().time_since_epoch().count() : seed)
{
    // コンストラクタ
}

MuseumLayout LevelGenerator::GenerateMuseum(
    float areaW, float areaH,
    const BSPParams& bsp,
    const DelaunayParams& dela,
    const MSTParams& mst)
{
    // 状態をリセット
    m_leaves.clear();
    m_rooms.clear();
    m_corridors.clear();
    m_points.clear();

    // --- 1. BSP (二分空間分割) ---
    // BSPアルゴリズムの実行
     SplitBSP(areaW, areaH, bsp);
     EmitRoomsFromLeaves(bsp); // m_rooms に RectF と Room::center が設定される

     // 部屋が少なすぎる場合は終了
     if (m_points.size() < 2)
     {
         return { m_rooms, m_corridors }; // 空のレイアウトを返す
     }

    // --- 2. Delaunay / MST による接続保証 ---
     BuildDelaunay(dela);  // m_tris が設定される

     // Delaunayからグラフ辺を抽出
     BuildGraphEdges();    // m_edges が設定される

     // 最小全域木（MST）を構築
     BuildMST(mst);        // m_mst が設定される

     // --- 3. 通路の生成 ---
     BuildCorridors();     // m_corridors に Segment が格納される

    return { m_rooms, m_corridors };
}

// デバッグ描画に使う色
const XMFLOAT4 ROOM_COLOR(0.0f, 0.5f, 1.0f, 1.0f);
const XMFLOAT4 CORRIDOR_COLOR(1.0f, 0.5f, 0.0f, 1.0f);

void LevelGenerator::DebugDraw(const MuseumLayout& layout, const GridMapping& map) const
{
    const float LINE_DRAW_HEIGHT = map.yFloor + 0.1f; // 床から0.1m持ち上げる
    const float LINE_LIFT = map.wallHeight + 1.0f; // 壁の上に描画する場合は壁の高さ+1.0m

    // RectFの中心点をワールド座標のXZ平面上の位置に写像するラムダ関数
    auto XZ_floor = [&](XMFLOAT2 v) { return XMFLOAT3(v.x * map.scaleXZ, LINE_DRAW_HEIGHT, v.y * map.scaleXZ); };
    auto XZ_lift = [&](XMFLOAT2 v) { return XMFLOAT3(v.x * map.scaleXZ, LINE_LIFT, v.y * map.scaleXZ); };

    // 1. 部屋の境界線の描画 (layout.roomsを使用)
    for (const auto& room : layout.rooms)
    {
        const RectF& rc = room.rect;
        // Y座標は床よりわずかに上 (0.01f) に設定し、床 Entity に埋もれないようにする
        float y = map.yFloor + 0.01f;

        // 4隅の座標 (XZ平面座標をXMFLOAT3に変換)
        DirectX::XMFLOAT3 p0 = { rc.x, y, rc.y };
        DirectX::XMFLOAT3 p1 = { rc.x + rc.w, y, rc.y };
        DirectX::XMFLOAT3 p2 = { rc.x + rc.w, y, rc.y + rc.h };
        DirectX::XMFLOAT3 p3 = { rc.x, y, rc.y + rc.h };

        // 4辺を描画 (Geometory::AddLineが適切に機能することを前提)
        Geometory::AddLine(p0, p1, ROOM_COLOR);
        Geometory::AddLine(p1, p2, ROOM_COLOR);
        Geometory::AddLine(p2, p3, ROOM_COLOR);
        Geometory::AddLine(p3, p0, ROOM_COLOR);
    }

    // 2. 通路の中心線 (layout.corridorsを使用) の描画
    for (const auto& seg : layout.corridors)
    {
        DirectX::XMFLOAT3 pA = seg.a;
        DirectX::XMFLOAT3 pB = seg.b;

        // 通路の中心線が見やすいように、部屋の境界線より少し高い位置 (0.05f) に描画
        pA.y = map.yFloor + 1.05f;
        pB.y = map.yFloor + 1.05f;

        Geometory::AddLine(pA, pB, CORRIDOR_COLOR);
    }

    Geometory::DrawLines();
}

//--------------------------------------
// プライベート関数（骨格：今後実装が必要な領域）
//--------------------------------------

void ProcGen::LevelGenerator::SplitBSP(float W, float H, const BSPParams& p)
{
    // 初期空間を m_leaves に追加（GenerateMuseumでクリアされている前提）
    if (m_leaves.empty()) {
        // -W/2, -H/2 を始点とするため、中心が (0,0) になる
        m_leaves.push_back({ { -W / 2.0f, -H / 2.0f, W, H }, 0 });
    }

    // 新しい葉を格納するためのリスト
    std::vector<Leaf> new_leaves;

    // 既存の葉を処理
    for (const auto& parent : m_leaves)
    {
        // 1. 分割終了条件
        // - 最大深度に到達
        if (parent.depth >= p.maxDepth) continue;
        // - 最小サイズより小さい（分割後に各々が最小サイズの半分以上を確保できるか）
        if (parent.rc.w < p.minLeafW * 2.0f || parent.rc.h < p.minLeafH * 2.0f) continue;

        // 2. 分割軸の決定
        bool split_horizontal; // true: Y軸に沿って分割 (Xをスパン: 左右の葉を生成)、false: X軸に沿って分割 (Yをスパン: 上下の葉を生成)

        float aspect_w = parent.rc.w / parent.rc.h;
        float aspect_h = parent.rc.h / parent.rc.w;

        // 幅が高さより一定以上大きい場合、垂直に分割 (split_horizontal = false)
        if (aspect_w > p.maxAspect) {
            split_horizontal = false;
        }
        // 高さが幅より一定以上大きい場合、水平に分割 (split_horizontal = true)
        else if (aspect_h > p.maxAspect) {
            split_horizontal = true;
        }
        // それ以外の場合、乱数で決定
        else {
            split_horizontal = (Rand01(m_rng) < 0.5f);
        }

        // 3. 分割位置の決定と実行
        float split_min, split_max, size_to_split;
        float min_size;

        if (split_horizontal) // Y軸に沿って分割（左右の葉を生成）
        {
            size_to_split = parent.rc.h;
            min_size = p.minLeafH;
        }
        else // X軸に沿って分割（上下の葉を生成）
        {
            size_to_split = parent.rc.w;
            min_size = p.minLeafW;
        }

        // 分割位置の範囲設定
        // min_sizeから (size_to_split - min_size) の間でランダムに決定
        split_min = min_size;
        split_max = size_to_split - min_size;

        if (split_min >= split_max) {
            // 分割できる範囲がない
            new_leaves.push_back(parent); // 分割せず親を保持
            continue;
        }

        // 中央付近に偏らせるため、正規乱数（またはより調整された乱数）を使うこともできますが、
        // ここでは均一乱数で簡略化
        float split_pos = split_min + (split_max - split_min) * Rand01(m_rng);

        // 葉の生成
        Leaf child1 = parent;
        Leaf child2 = parent;
        child1.depth = child2.depth = parent.depth + 1;

        if (split_horizontal) // Y軸に沿って分割（左右の葉）
        {
            // 上側の葉 (Y軸大)
            child1.rc.h = split_pos;
            // 下側の葉 (Y軸小)
            child2.rc.y += split_pos;
            child2.rc.h -= split_pos;
        }
        else // X軸に沿って分割（上下の葉）
        {
            // 右側の葉 (X軸大)
            child1.rc.w = split_pos;
            // 左側の葉 (X軸小)
            child2.rc.x += split_pos;
            child2.rc.w -= split_pos;
        }

        new_leaves.push_back(child1);
        new_leaves.push_back(child2);
    }

    // 分割されなかった葉と新しい葉をマージ
    m_leaves.insert(m_leaves.end(), new_leaves.begin(), new_leaves.end());
    // 再帰的に呼び出す代わりに、リストを置き換え、ループで処理する方式に変更（非再帰）
    // ループ内での m_leaves の操作を避けるため、一旦クリアし、new_leavesで置き換えるか、再帰に切り替えるかの選択が必要です。
    // 非再帰で実装をシンプルにするため、ここでは分割できたものだけを m_leaves から削除し、new_leavesを追加する方式とします。
    // しかし、上記のコードはループ中に new_leaves を構築し、ループ後に m_leaves に追加しています。
    // 再帰的な処理を非再帰で実装するには、キュー/スタックを使うか、この関数を呼び出し元で繰り返し実行する必要があります。
    // 簡単のため、ここは非再帰ループを継続する形にします。

    // 再帰的に全葉を処理する簡単な方法として、forループを while ループに変更します。
    // while (i < m_leaves.size()) を使うことで、新しく追加された葉も処理対象にできます。

    // --- 修正案: whileループに変更 ---
    size_t i = 0;
    while (i < m_leaves.size())
    {
        Leaf parent = m_leaves[i]; // コピー
        i++; // 次の葉へ

        // ... (省略: 上記の 1. 2. 3. のロジック) ...

        // 葉が分割された場合:
        // m_leaves[i-1] を削除し、child1, child2 を挿入する処理が必要となり、インデックス管理が複雑になるため、
        // **BSPを再帰関数で実装するか、キュー/スタックで管理する方が望ましい**です。

        // --- 簡易実装として、新しい葉を末尾に追加し、次の呼び出しで再処理を期待します。 ---
        // これは再帰的な分割を保証しませんが、デモとして動かすためのものです。
        // 正しい再帰呼び出しまたはキューベースの処理に置き換えることを推奨します。

        // ... 分割ロジックの結果 child1, child2 が得られたとして ...
        // m_leaves.push_back(child1);
        // m_leaves.push_back(child2);
    }
    // 上記のロジックは m_leaves の中身を正しく分割/置換していません。
    // 正しい実装のために、以下のコードに置き換えます。

    m_leaves.clear();
    m_leaves.push_back({ { -W / 2.0f, -H / 2.0f, W, H }, 0 }); // 初期空間

    std::vector<Leaf> queue = m_leaves;
    m_leaves.clear(); // 分割後の葉のみを格納するために初期化

    while (!queue.empty())
    {
        Leaf parent = queue.back();
        queue.pop_back();

        // 1. 分割終了条件
        if (parent.depth >= p.maxDepth ||
            parent.rc.w < p.minLeafW * 2.0f || parent.rc.h < p.minLeafH * 2.0f)
        {
            m_leaves.push_back(parent); // 分割終了した葉を最終リストに追加
            continue;
        }

        // 2. 分割軸の決定
        bool split_horizontal;
        float aspect_w = parent.rc.w / parent.rc.h;
        float aspect_h = parent.rc.h / parent.rc.w;

        // ハードコードされた閾値 1.3f をパラメータ p.aspectRatioThreshold に置き換え
        const float ASPECT_THRES = p.aspectRatioThreshold;

        if (aspect_w > ASPECT_THRES && parent.rc.w > parent.rc.h) {
            split_horizontal = false; // 垂直分割 (X軸に沿って分割 = 幅Wを分割)
        }
        else if (aspect_h > ASPECT_THRES && parent.rc.h > parent.rc.w) {
            split_horizontal = true; // 水平分割 (Y軸に沿って分割 = 高さHを分割)
        }
        else {
            split_horizontal = (Rand01(m_rng) < 0.5f);
        }

        // 3. 分割位置の決定
        float min_size, size_to_split;

        if (split_horizontal) { // Y軸に沿って分割（高さHを分割）
            size_to_split = parent.rc.h;
            min_size = p.minLeafH;
        }
        else { // X軸に沿って分割（幅Wを分割）
            size_to_split = parent.rc.w;
            min_size = p.minLeafW;
        }

        float split_min = min_size * 1.5f; // 最小サイズの1.5倍から
        float split_max = size_to_split - min_size * 1.5f; // 最大サイズから1.5倍を引いたところまで

        if (split_min >= split_max) {
            m_leaves.push_back(parent); // 分割不可能と判断し、葉を最終リストに追加
            continue;
        }

        // 乱数で分割位置を決定（中央に偏らせる乱数分布を使うとより良い）
        float split_pos = split_min + (split_max - split_min) * Rand01(m_rng);

        // 4. 葉の生成
        Leaf child1 = parent;
        Leaf child2 = parent;
        child1.depth = child2.depth = parent.depth + 1;

        if (split_horizontal) // 高さ(H)を分割
        {
            // 上側の葉 (Y軸小)
            child1.rc.h = split_pos;
            // 下側の葉 (Y軸大)
            child2.rc.y += split_pos;
            child2.rc.h -= split_pos;
        }
        else // 幅(W)を分割
        {
            // 左側の葉 (X軸小)
            child1.rc.w = split_pos;
            // 右側の葉 (X軸大)
            child2.rc.x += split_pos;
            child2.rc.w -= split_pos;
        }

        queue.push_back(child1);
        queue.push_back(child2);
    }
}

void LevelGenerator::EmitRoomsFromLeaves(const BSPParams& p)
{
    m_rooms.clear();
    m_points.clear(); // Delaunay用に中心点を保存するリストもクリア

    for (const auto& leaf : m_leaves)
    {
        // 1. ランダムな余白（Inset）の決定
        // p.roomInset を基点に、0.5倍～1.0倍の範囲で変動させる
        float random_factor_x = 0.5f + 0.5f * Rand01(m_rng);
        float random_factor_y = 0.5f + 0.5f * Rand01(m_rng);

        float inset_x = p.roomInset * random_factor_x;
        float inset_y = p.roomInset * random_factor_y;

        // 2. 部屋の矩形（RectF）の計算
        float room_x = leaf.rc.x + inset_x;
        float room_y = leaf.rc.y + inset_y;
        float room_w = leaf.rc.w - 2.0f * inset_x;
        float room_h = leaf.rc.h - 2.0f * inset_y;

        // 3. 最小サイズチェック
        // 部屋の幅または高さが、設定された最小サイズより小さくなったらスキップ
        // BSP分割時の最小サイズ p.minLeafW/2.0f よりも少し小さくても許容するように調整
        const float MIN_ROOM_SIZE_RATIO = 0.4f;
        if (room_w < p.minLeafW * MIN_ROOM_SIZE_RATIO || room_h < p.minLeafH * MIN_ROOM_SIZE_RATIO)
        {
            continue; // 部屋として小さすぎるため無視
        }

        // 4. Room構造体を構築し、m_roomsに追加
        Room room;
        room.rect = { room_x, room_y, room_w, room_h };

        // 中心点（Delaunay用）の計算
        room.center = { room_x + room_w / 2.0f, room_y + room_h / 2.0f };

        m_rooms.push_back(room);
        m_points.push_back(room.center);
    }
}

/**
 * [bool - Circumcircle]
 * @brief	点pが三角形tの外接円の内側にあるか、及び外接円の中心と半径を計算する。
 * 
 * @param	[in] p - テストする点
 * @param	[in] t - 三角形（m_pointsのインデックス）
 * @param	[out] c - 外接円の中心（出力）
 * @param	[out] r2 - 外接円の半径の2乗（出力）
 * @return	bool - 点pが外接円の内側にあるか
 */
bool ProcGen::LevelGenerator::Circumcircle(const DirectX::XMFLOAT2& p, const Tri& t, DirectX::XMFLOAT2& c, float& r2) const
{
    const XMFLOAT2& p1 = m_points[t.i0];
    const XMFLOAT2& p2 = m_points[t.i1];
    const XMFLOAT2& p3 = m_points[t.i2];

    // p1, p2, p3 のいずれかが同一、または共線の場合、外接円は定義できない
    float dx1 = p2.x - p1.x; float dy1 = p2.y - p1.y;
    float dx2 = p3.x - p1.x; float dy2 = p3.y - p1.y;

    float D = 2.0f * (dx1 * dy2 - dy1 * dx2);
    if (std::abs(D) < 1e-6f) // ほぼ共線、または重複
    {
        r2 = FLT_MAX; // 半径を無限大として扱う
        return false;
    }

    float mag1 = p1.x * p1.x + p1.y * p1.y;
    float mag2 = p2.x * p2.x + p2.y * p2.y;
    float mag3 = p3.x * p3.x + p3.y * p3.y;

    // 外接円の中心 c = (cx, cy) を計算
    c.x = ((mag2 - mag1) * dy2 - (mag3 - mag1) * dy1) / D;
    c.y = ((mag3 - mag1) * dx1 - (mag2 - mag1) * dx2) / D;

    // 外接円の半径の2乗 r2 を計算
    r2 = (c.x - p1.x) * (c.x - p1.x) + (c.y - p1.y) * (c.y - p1.y);

    // テスト点 p と外接円の中心 c の距離の2乗 dist_sq を計算
    float dist_sq = (p.x - c.x) * (p.x - c.x) + (p.y - c.y) * (p.y - c.y);

    // 外接円の半径の2乗と比較し、誤差を考慮して内側にあるか判定 (dist_sq < r2)
    return dist_sq < r2;
}

void LevelGenerator::BuildDelaunay(const DelaunayParams& p)
{
    m_tris.clear();

    // 部屋の中心点がなければ終了
    if (m_points.size() < 3) return;

    // 1. スーパー三角形の作成
    // すべての点を含む大きな三角形を作成します。
    // AABB (Axis-Aligned Bounding Box) を計算
    float minX = FLT_MAX, minY = FLT_MAX;
    float maxX = FLT_MIN, maxY = FLT_MIN;

    for (const auto& pt : m_points) {
        minX = std::min(minX, pt.x);
        minY = std::min(minY, pt.y);
        maxX = std::max(maxX, pt.x);
        maxY = std::max(maxY, pt.y);
    }

    float deltaX = maxX - minX;
    float deltaY = maxY - minY;
    float max_dim = std::max(deltaX, deltaY);
    float midX = (minX + maxX) / 2.0f;
    float midY = (minY + maxY) / 2.0f;

    // パディングを追加
    float pad = max_dim * 0.5f;

    // スーパー三角形の3点 (仮想点として m_points に追加)
    // 0, 1, 2番目のインデックスがスーパー三角形の頂点になる
    XMFLOAT2 super_p0 = { midX - pad * 20.0f, midY - pad * 10.0f };
    XMFLOAT2 super_p1 = { midX + pad * 20.0f, midY - pad * 10.0f };
    XMFLOAT2 super_p2 = { midX, midY + pad * 30.0f };

    size_t super_idx_start = m_points.size();
    m_points.push_back(super_p0);
    m_points.push_back(super_p1);
    m_points.push_back(super_p2);

    // 初期三角形としてスーパー三角形を追加
    m_tris.push_back({ (int)super_idx_start, (int)super_idx_start + 1, (int)super_idx_start + 2 });

    // 2. 各点に対してボウヤー・ワトソン法を実行
    // 部屋の中心点のみを対象とする (スーパー三角形の頂点は除く)
    for (size_t i = 0; i < super_idx_start; ++i)
    {
        const XMFLOAT2& current_point = m_points[i];

        // 辺のリスト（バッファ）
        std::vector<Edge> edge_buffer;

        // 無効な三角形（外接円に current_point を含む）を削除
        // 処理中に m_tris が変更されるため、一旦削除対象をリスト化
        std::vector<Tri> bad_tris;
        for (const auto& tri : m_tris)
        {
            XMFLOAT2 c;
            float r2;
            // 点 i が外接円の内側にある、または外接円上にある場合（非推奨：計算誤差に注意）
            if (Circumcircle(current_point, tri, c, r2))
            {
                bad_tris.push_back(tri);

                // 無効な三角形の辺をエッジバッファに追加
                edge_buffer.push_back({ tri.i0, tri.i1 });
                edge_buffer.push_back({ tri.i1, tri.i2 });
                edge_buffer.push_back({ tri.i2, tri.i0 });
            }
        }

        // m_tris から bad_tris を削除
        std::vector<Tri> new_tris;
        for (const auto& tri : m_tris)
        {
            bool is_bad = false;
            for (const auto& bad : bad_tris)
            {
                if (tri.i0 == bad.i0 && tri.i1 == bad.i1 && tri.i2 == bad.i2) {
                    is_bad = true;
                    break;
                }
            }
            if (!is_bad) {
                new_tris.push_back(tri);
            }
        }
        m_tris = std::move(new_tris);

        // 重複する辺を排除して境界を決定
        std::vector<Edge> polygon;
        for (size_t j = 0; j < edge_buffer.size(); ++j)
        {
            const Edge& e1 = edge_buffer[j];
            bool is_unique = true;

            for (size_t k = j + 1; k < edge_buffer.size(); ++k)
            {
                const Edge& e2 = edge_buffer[k];
                // 辺 (a, b) と (b, a) は重複とみなす
                if ((e1.a == e2.a && e1.b == e2.b) || (e1.a == e2.b && e1.b == e2.a))
                {
                    // 重複した辺を見つけたら、両方を無効化するために k の辺も削除
                    // ここではリストから削除する代わりに、フラグなどで処理をスキップ
                    is_unique = false;
                    edge_buffer[k] = { -1, -1 }; // 無効フラグ
                    break;
                }
            }

            if (is_unique && e1.a != -1) {
                polygon.push_back(e1);
            }
        }

        // 境界の辺と新しい点 i を使って新しい三角形を作成
        for (const auto& edge : polygon)
        {
            m_tris.push_back({ edge.a, edge.b, (int)i });
        }
    }

    // 3. スーパー三角形と接続された三角形を削除
    std::vector<Tri> final_tris;
    for (const auto& tri : m_tris)
    {
        // 頂点がスーパー三角形の頂点インデックス (super_idx_start, super_idx_start+1, super_idx_start+2) のいずれかを含んでいたら削除
        if (tri.i0 >= (int)super_idx_start ||
            tri.i1 >= (int)super_idx_start ||
            tri.i2 >= (int)super_idx_start)
        {
            continue;
        }
        final_tris.push_back(tri);
    }
    m_tris = std::move(final_tris);

    // m_points からスーパー三角形の頂点を削除
    m_points.resize(super_idx_start);
}

void LevelGenerator::BuildGraphEdges()
{
    m_edges.clear();

    // 辺の重複を避けるため、正規化された辺をキーとするセットを使用
    // (a, b) と (b, a) を同一視し、常に a < b となるように正規化する
    std::unordered_set<size_t> unique_edges;

    for (const auto& tri : m_tris)
    {
        // 3つの辺 (i0, i1), (i1, i2), (i2, i0) をチェック
        std::pair<int, int> edges_indices[] = {
            {tri.i0, tri.i1},
            {tri.i1, tri.i2},
            {tri.i2, tri.i0}
        };

        for (const auto& edge : edges_indices)
        {
            // 辺を正規化: 常に小さいインデックスが first になるようにする
            int a = (std::min)(edge.first, edge.second);
            int b = (std::max)(edge.first, edge.second);

            // 頂点インデックスが重複していなければ有効な辺
            if (a == b) continue;

            // 辺のハッシュ値を計算 (単純な結合)
            // m_pointsの数が200以下と仮定し、20ビットずつ使用
            size_t edge_hash = ((size_t)a << 20) | (size_t)b;

            if (unique_edges.find(edge_hash) == unique_edges.end())
            {
                // 新しい辺ならセットに追加し、m_edgesにも追加
                unique_edges.insert(edge_hash);
                m_edges.push_back({ a, b });
            }
        }
    }
}

void LevelGenerator::BuildMST(const MSTParams& p)
{
    m_mst.clear();

    // 1. 辺の重みを計算し、ソート
    // tuple<float, int, int>: 重み、頂点A、頂点B
    // std::tie を使用するため、<tuple>が必要です。（既に <algorithm>, <numeric>, <cmath>, <chrono>, <unordered_set> はインクルード済み）
    std::vector<std::tuple<float, int, int>> edges_with_weight;

    for (const auto& edge : m_edges)
    {
        const XMFLOAT2& p1 = m_points[edge.a];
        const XMFLOAT2& p2 = m_points[edge.b];

        // Euclidean Distance (重み)
        float dx = p1.x - p2.x;
        float dy = p1.y - p2.y;
        float weight = std::sqrt(dx * dx + dy * dy);

        edges_with_weight.emplace_back(weight, edge.a, edge.b);
    }

    // 重みでソート (昇順)
    std::sort(edges_with_weight.begin(), edges_with_weight.end());

    // 2. Kruskal法を実行
    DSU dsu(m_rooms.size()); // 部屋の数でUnion-Findを初期化
    std::vector<Edge> unused_edges;

    for (const auto& edge_tuple : edges_with_weight)
    {
        float weight;
        int a, b;
        std::tie(weight, a, b) = edge_tuple;

        if (dsu.unite(a, b))
        {
            // 閉路を作らない辺はMSTに追加
            m_mst.push_back({ a, b });
        }
        else
        {
            // 閉路を作る辺は未使用辺リストへ
            unused_edges.push_back({ a, b });
        }
    }

    // 3. 余剰辺の追加 (回遊性のため)
    if (!unused_edges.empty() && p.extraEdgeRatio > 0.0f)
    {
        size_t num_extra_edges = (size_t)(unused_edges.size() * p.extraEdgeRatio);

        // 乱数で選択するために、未使用の辺リストをシャッフル
        std::shuffle(unused_edges.begin(), unused_edges.end(), m_rng);

        // 最初の num_extra_edges だけを MST に追加
        for (size_t i = 0; i < num_extra_edges && i < unused_edges.size(); ++i)
        {
            m_mst.push_back(unused_edges[i]);
        }
    }
}

void ProcGen::LevelGenerator::BuildCorridors()
{
    m_corridors.clear();
    const float CORRIDOR_WIDTH = 2.0f; // Segments構造体のデフォルト値と合わせる

    for (const auto& edge : m_mst)
    {
        // 部屋のインデックスから、中心点と部屋情報を取得
        const Room& r1 = m_rooms[edge.a];
        const Room& r2 = m_rooms[edge.b];

        const XMFLOAT2& pA_xz = r1.center;
        const XMFLOAT2& pB_xz = r2.center;

        // 部屋の中心をワールド座標のXMFLOAT3に変換 (Y=0.0fを仮定)
        XMFLOAT3 pA = { pA_xz.x, 0.0f, pA_xz.y };
        XMFLOAT3 pB = { pB_xz.x, 0.0f, pB_xz.y };

        // 1. L字型通路の折れ点（中間点）の決定
        XMFLOAT3 pM;

        // X軸を優先して曲げるか、Z軸を優先して曲げるかをランダムに決定
        if (Rand01(m_rng) < 0.5f)
        {
            // X-Z-Z (X, Z) → 2つのセグメント: (pA -> pM), (pM -> pB)
            pM = { pB.x, 0.0f, pA.z };
        }
        else
        {
            // Z-X-X (Z, X) → 2つのセグメント: (pA -> pM), (pM -> pB)
            pM = { pA.x, 0.0f, pB.z };
        }

        // 2. 2つの線分（Segment）に分解し、m_corridors に格納

        // セグメント 1: pA から折れ点 pM へ
        m_corridors.push_back({ pA, pM, CORRIDOR_WIDTH });

        // セグメント 2: 折れ点 pM から pB へ
        m_corridors.push_back({ pM, pB, CORRIDOR_WIDTH });
    }
}