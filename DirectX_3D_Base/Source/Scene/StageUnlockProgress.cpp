// ============================================================
// StageUnlockProgress.cpp - 18ステージ対応版
// ============================================================

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
#include <shlobj.h>
#endif

namespace
{
	// ★18ステージに対応
	constexpr int kMaxStages = 18;  // 6 → 18 に変更
	constexpr int kMinStages = 1;
	constexpr const char* kSaveFileName = "stage_unlock_progress.txt";

	int  g_maxUnlocked = 1;
	bool g_loaded = false;
	int  g_pendingReveal = -1;
	uint32_t g_bestTimeMs[kMaxStages + 1] = {};           // 1..18, 0=未記録
	std::uint8_t g_stageStarMask[kMaxStages + 1] = {};    // 1..18, 0=未取得(3bit)

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
		char path[MAX_PATH] = {};
		GetModuleFileNameA(nullptr, path, MAX_PATH);
		std::string exePath(path);
		size_t pos = exePath.find_last_of("\\/");
		if (pos != std::string::npos)
		{
			return exePath.substr(0, pos);
		}
		return "";
#else
		return ".";
#endif
	}

	std::string GetSaveFilePath()
	{
		std::string dir = GetExeDirectory();
		if (!dir.empty())
		{
			return dir + "\\" + kSaveFileName;
		}
		return kSaveFileName;
	}

	bool FileExists(const std::string& path)
	{
		std::ifstream f(path);
		return f.good();
	}

	void LoadFromFile()
	{
		std::string path = GetSaveFilePath();
		if (!FileExists(path))
		{
			g_maxUnlocked = 1;
			for (int i = 0; i <= kMaxStages; i++)
			{
				g_bestTimeMs[i] = 0;
				g_stageStarMask[i] = 0;
			}
			return;
		}

		std::ifstream ifs(path);
		if (!ifs.is_open())
		{
			g_maxUnlocked = 1;
			return;
		}

		ifs >> g_maxUnlocked;
		g_maxUnlocked = ClampStage(g_maxUnlocked);

		for (int i = 1; i <= kMaxStages; i++)
		{
			uint32_t t = 0;
			ifs >> t;
			g_bestTimeMs[i] = t;
		}

		for (int i = 1; i <= kMaxStages; i++)
		{
			int m = 0;
			ifs >> m;
			g_stageStarMask[i] = static_cast<std::uint8_t>(m & 0x7);
		}
	}

	void SaveToFile()
	{
		std::string path = GetSaveFilePath();
		std::ofstream ofs(path);
		if (!ofs.is_open())
		{
			return;
		}

		ofs << g_maxUnlocked << "\n";

		for (int i = 1; i <= kMaxStages; i++)
		{
			ofs << g_bestTimeMs[i];
			if (i < kMaxStages) ofs << " ";
		}
		ofs << "\n";

		for (int i = 1; i <= kMaxStages; i++)
		{
			ofs << static_cast<int>(g_stageStarMask[i]);
			if (i < kMaxStages) ofs << " ";
		}
		ofs << "\n";
	}
}

namespace StageUnlockProgress
{
	void Load()
	{
		if (g_loaded) return;
		LoadFromFile();
		g_loaded = true;
	}

	void ResetToNewGame()
	{
		g_maxUnlocked = 1;
		for (int i = 0; i <= kMaxStages; i++)
		{
			g_bestTimeMs[i] = 0;
			g_stageStarMask[i] = 0;
		}
		g_loaded = true;
	}

	void ForceReload()
	{
		g_loaded = false;
		Load();
	}

	void Save()
	{
		if (!g_loaded)
		{
			Load();
		}
		SaveToFile();
	}

	void ResetAllAndSave()
	{
		g_maxUnlocked = 1;
		for (int i = 0; i <= kMaxStages; i++)
		{
			g_bestTimeMs[i] = 0;
			g_stageStarMask[i] = 0;
		}
		g_loaded = true;
		SaveToFile();
	}

	bool HasSaveFile()
	{
		return FileExists(GetSaveFilePath());
	}

	int GetMaxUnlockedStage()
	{
		Load();
		return g_maxUnlocked;
	}

	// ★新規追加: 指定したステージを解放する関数
	void UnlockStage(int stageNo)
	{
		Load();

		if (stageNo < kMinStages || stageNo > kMaxStages) return;

		// すでに解放済みなら何もしない
		if (stageNo <= g_maxUnlocked) return;

		g_maxUnlocked = ClampStage(stageNo);
		Save();
	}

	int UnlockNextStageFromClearedStageID(const std::string& clearedStageID)
	{
		Load();

		int cleared = ParseStageNo(clearedStageID);
		if (cleared < 1 || cleared >= kMaxStages)
		{
			return -1;
		}

		int next = cleared + 1;
		if (next > g_maxUnlocked)
		{
			g_maxUnlocked = next;
			Save();
			return next;
		}
		return -1;
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
		if (clearTimeSec <= 0.0f) return false;

		uint32_t newMs = static_cast<uint32_t>(clearTimeSec * 1000.0f);
		uint32_t old = g_bestTimeMs[stageNo];

		if (old == 0 || newMs < old)
		{
			g_bestTimeMs[stageNo] = newMs;
			Save();
			return true;
		}
		return false;
	}

	int ExtractStageNo(const std::string& stageID)
	{
		return ParseStageNo(stageID);
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

		g_stageStarMask[stageNo] |= (addMask & 0x7);
		Save();
	}

	void SetPendingRevealStage(int stageNo)
	{
		// ★18ステージ対応
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
		int result = g_pendingReveal;
		g_pendingReveal = -1;
		return result;
	}
}