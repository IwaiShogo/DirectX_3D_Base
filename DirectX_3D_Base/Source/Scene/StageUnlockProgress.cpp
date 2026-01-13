#include "Scene/StageUnlockProgress.h"

#include <algorithm>
#include <cctype>
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

	int ClampStage(int v)
	{
		return std::max(kMinStages, std::min(kMaxStages, v));
	}

	// "ST_001" / "ST_1" / "Stage1" などから数字を抜く（最初に見つかった連続数字を採用）
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
		return value; // 001 のような形式はそのまま 1 になる
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
		// 実行場所（カレントディレクトリ）に依存しないように、exeと同じフォルダへ固定する。
		const std::string exeDir = GetExeDirectory();
		if (!exeDir.empty())
		{
			return exeDir + kSaveFileName;
		}
		return std::string(kSaveFileName);
	}
}

namespace StageUnlockProgress
{
	void Load()
	{
		if (g_loaded) return;
		g_loaded = true;

#if defined(_DEBUG)
		// Debugビルドでは「毎回初回起動扱い」に固定する。
		// ＝デバッグ起動するたびに、ステージ数が必ず1個になる。
		g_maxUnlocked = 1;
		g_pendingReveal = -1;

		// 保存ファイルが残っていたら消す（失敗してもOK）
		{
			const std::string path = GetSaveFilePath();
			std::remove(path.c_str());
		}
		return;
#endif

		std::ifstream ifs(GetSaveFilePath());
		if (!ifs.is_open())
		{
			// 保存ファイルが無い＝初回起動扱い
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
			// 壊れている場合も初回扱いにフォールバック
			g_maxUnlocked = 1;
		}
	}

	void Save()
	{
#if defined(_DEBUG)
		// Debugビルドでは保存しない（次回起動に残さない）
		return;
#endif

		g_loaded = true;

		std::ofstream ofs(GetSaveFilePath(), std::ios::trunc);
		if (!ofs.is_open()) return;

		ofs << ClampStage(g_maxUnlocked);
	}

	int GetMaxUnlockedStage()
	{
		Load();
		return g_maxUnlocked;
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
		// ここは「演出用」なので保存しない
		if (stageNo < 2 || stageNo > kMaxStages)
		{
			g_pendingReveal = -1;
			return;
		}

		// 未解放は演出不要
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
	void ResetProgress()
	{
		g_maxUnlocked = 1; // ステージ1のみ解放状態にする
		Save();            // その状態でファイルに書き込む
	}
}
