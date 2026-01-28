#include "Scene/StageUnlockProgress.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>

#if defined(_WIN32)
#define NOMINMAX
#include <Windows.h>
#endif

namespace
{
	constexpr int kMaxStages = 6;
	constexpr int kMinStages = 1;
	constexpr const char* kSaveFileName = "stage_unlock_progress.txt";

	int  g_maxUnlocked = 1;
	bool g_loaded = false;
	int  g_pendingReveal = -1;
	uint32_t g_bestTimeMs[kMaxStages + 1] = {};           // 1..6, 0=未記録
	std::uint8_t g_stageStarMask[kMaxStages + 1] = {};    // 1..6, 0=未取得(3bit)

	int ClampStage(int v)
	{
		return std::max(kMinStages, std::min(kMaxStages, v));
	}

	// "ST_001" / "ST_1" / "Stage1" などから数字部分を抽出
	int ParseStageNo(const std::string& stageID)
	{
		int  value = 0;
		bool inDigits = false;

		for (char ch : stageID)
		{
			if (std::isdigit(static_cast<unsigned char>(ch)))
			{
				inDigits = true;
				value = value * 10 + (ch - '0');
			}
			else if (inDigits)
			{
				break;
			}
		}
		if (!inDigits) return -1;
		return value;
	}

	std::string GetExeDirectory()
	{
#if defined(_WIN32)
		char modulePath[MAX_PATH] = {};
		const DWORD len = ::GetModuleFileNameA(nullptr, modulePath, MAX_PATH);
		if (len == 0 || len >= MAX_PATH)
		{
			return std::string();
		}

		std::string path(modulePath, modulePath + len);
		const size_t slash = path.find_last_of("\\/");
		if (slash == std::string::npos) return std::string();

		return path.substr(0, slash + 1);
#else
		return std::string();
#endif
	}

	std::string GetSaveFilePath()
	{
		// exe の隣に固定して、作業ディレクトリ依存を避ける
		const std::string exeDir = GetExeDirectory();
		if (!exeDir.empty())
		{
			return exeDir + kSaveFileName;
		}
		return std::string(kSaveFileName);
	}

	bool FileExists(const std::string& path)
	{
		std::ifstream ifs(path);
		return ifs.is_open();
	}
}

namespace StageUnlockProgress
{
	void Load()
	{
		if (g_loaded) return;
		g_loaded = true;

		// --- 起動ごと初期化（未記録/未取得） ---
		for (int i = 1; i <= kMaxStages; ++i)
		{
			g_bestTimeMs[i] = 0;
			g_stageStarMask[i] = 0;
		}
		g_pendingReveal = -1;

		std::ifstream ifs(GetSaveFilePath());
		if (!ifs.is_open())
		{
			g_maxUnlocked = 1;
			return;
		}

		int v = 1;
		ifs >> v;
		if (!ifs.fail())
		{
			g_maxUnlocked = ClampStage(v);
		}
		else
		{
			g_maxUnlocked = 1;
		}

		// ベストタイム(ms)
		for (int i = 1; i <= kMaxStages; ++i)
		{
			uint32_t t = 0;
			ifs >> t;
			if (ifs.fail()) break;
			g_bestTimeMs[i] = t;
		}

		// スター(3bit)（古いファイルなら無いので 0 のまま）
		for (int i = 1; i <= kMaxStages; ++i)
		{
			int s = 0;
			ifs >> s;
			if (ifs.fail()) break;
			g_stageStarMask[i] = static_cast<std::uint8_t>(s & 0x7);
		}
	}

	/**
	 * @brief 「はじめから」：進捗を完全初期化して保存する
	 * - 解放ステージ: 1 (1-1のみ)
	 * - ベストタイム: 全て 0
	 * - スター: 全て 0
	 * - 解放演出の保留もクリア
	 */
	void ResetToNewGame()
	{
		ResetAllAndSave();
	}

	void ForceReload()
	{
		g_loaded = false;
		Load();
	}

	bool HasSaveFile()
	{
		return FileExists(GetSaveFilePath());
	}

	void ResetAllAndSave()
	{
		// 既にロード済みでも確実に初期化する
		g_loaded = true;
		g_maxUnlocked = 1;
		g_pendingReveal = -1;

		for (int i = 1; i <= kMaxStages; ++i)
		{
			g_bestTimeMs[i] = 0;
			g_stageStarMask[i] = 0;
		}

		Save(); // ファイルも上書きして「次回起動」でも最初からになる
	}

	void Save()
	{
		g_loaded = true;

		std::ofstream ofs(GetSaveFilePath(), std::ios::trunc);
		if (!ofs.is_open()) return;

		// v2形式:
		// 1行目: maxUnlocked
		// 2行目: bestTimeMs[1..6]
		// 3行目: stageStarMask[1..6]
		ofs << ClampStage(g_maxUnlocked) << "\n";

		for (int i = 1; i <= kMaxStages; ++i)
		{
			ofs << g_bestTimeMs[i];
			if (i != kMaxStages) ofs << ' ';
		}
		ofs << "\n";

		for (int i = 1; i <= kMaxStages; ++i)
		{
			ofs << static_cast<int>(g_stageStarMask[i] & 0x7);
			if (i != kMaxStages) ofs << ' ';
		}
	}

	int GetMaxUnlockedStage()
	{
		Load();
		return g_maxUnlocked;
	}

	uint32_t GetBestTimeMs(int stageNo)
	{
		Load();
		if (stageNo < 1 || stageNo > kMaxStages) return 0;
		return g_bestTimeMs[stageNo];
	}

	bool UpdateBestTimeIfFaster(int stageNo, float clearTimeSec)
	{
		Load();
		if (stageNo < 1 || stageNo > kMaxStages) return false;
		if (!(clearTimeSec > 0.05f)) return false;

		const uint32_t newMs = static_cast<uint32_t>(clearTimeSec * 1000.0f + 0.5f);
		if (newMs == 0) return false;

		const uint32_t oldMs = g_bestTimeMs[stageNo];
		if (oldMs != 0 && newMs >= oldMs) return false;

		g_bestTimeMs[stageNo] = newMs;
		Save();
		return true;
	}

	int ExtractStageNo(const std::string& stageID)
	{
		Load();
		const int v = ParseStageNo(stageID);
		if (v < 1 || v > kMaxStages) return -1;
		return v;
	}

	std::uint8_t GetStageStarMask(int stageNo)
	{
		Load();
		if (stageNo < 1 || stageNo > kMaxStages) return 0;
		return g_stageStarMask[stageNo];
	}

	void UpdateStageStarMaskOr(int stageNo, std::uint8_t addMask)
	{
		Load();
		if (stageNo < 1 || stageNo > kMaxStages) return;
		addMask &= 0x7;
		g_stageStarMask[stageNo] = static_cast<std::uint8_t>(g_stageStarMask[stageNo] | addMask);
		Save(); // ★星も永続化する
	}

	int UnlockNextStageFromClearedStageID(const std::string& clearedStageID)
	{
		Load();

		const int cleared = ParseStageNo(clearedStageID);
		if (cleared < 1) return -1;

		const int next = std::min(kMaxStages, cleared + 1);
		if (next <= g_maxUnlocked) return -1;

		g_maxUnlocked = ClampStage(next);
		Save();
		return g_maxUnlocked;
	}

	void SetPendingRevealStage(int stageNo)
	{
		if (stageNo < 2 || stageNo > kMaxStages)
		{
			g_pendingReveal = -1;
			return;
		}
		if (stageNo > GetMaxUnlockedStage())
		{
			g_pendingReveal = -1;
			return;
		}
		g_pendingReveal = stageNo;
	}

	int ConsumePendingRevealStage()
	{
		const int v = g_pendingReveal;
		g_pendingReveal = -1;
		return v;
	}
}
