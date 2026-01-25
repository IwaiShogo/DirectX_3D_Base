#pragma once

#include <string>

namespace StageUnlockProgress
{
	void Load();

	void Save();

	int GetMaxUnlockedStage();
	int UnlockNextStageFromClearedStageID(const std::string& clearedStageID);

	void SetPendingRevealStage(int stageNo);
	int ConsumePendingRevealStage();

	int GetBestTimeMs(int stageNo);

	bool UpdateBestTimeMs(int stageNo, int clearTimeMs);

	bool UpdateBestTimeFromStageID(const std::string& stageID, float clearTimeSec);

	float GetBestTimeSec(int stageNo);

}
