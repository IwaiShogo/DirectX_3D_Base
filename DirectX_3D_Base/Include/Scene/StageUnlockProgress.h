// ============================================================
// StageUnlockProgress.h - 18ステージ対応版
// ============================================================

#pragma once

#include <string>
#include <cstdint>

/**
 * @file StageUnlockProgress.h
 * @brief ステージ解放/ベストタイム/スター(星)の永続化管理（18ステージ対応）
 */
namespace StageUnlockProgress
{
	/**
	 * @brief セーブデータ読込
	 */
	void Load();

	void ResetToNewGame();

	/**
	 * @brief 強制的にファイルから再読み込みする（既に Load 済みでも読み直す）
	 */
	void ForceReload();

	/**
	 * @brief セーブデータ保存
	 */
	void Save();

	/**
	 * @brief セーブデータを初期化して保存する（初めから用）
	 *
	 * - 解放ステージ: 1
	 * - ベストタイム: 全て 0
	 * - スター: 全て 0
	 */
	void ResetAllAndSave();

	/**
	 * @brief セーブファイルが存在するか（Continue ボタン可否など）
	 */
	bool HasSaveFile();

	/**
	 * @brief 現在解放されている最大ステージ番号を取得
	 * @return 1..18
	 */
	int GetMaxUnlockedStage();

	/**
	 * @brief ★新規追加: 指定したステージを解放する
	 * @param stageNo 解放するステージ番号 (1..18)
	 */
	void UnlockStage(int stageNo);

	/**
	 * @brief クリアしたステージIDから次ステージを解放する
	 * @param clearedStageID "ST_001" / "Stage1" など
	 * @return 新しく解放されたステージ番号(2..18)。解放が無い場合 -1
	 */
	int UnlockNextStageFromClearedStageID(const std::string& clearedStageID);

	/**
	 * @brief 指定ステージのベストタイム(ms)を取得する（未記録は 0）
	 * @param stageNo 1..18
	 */
	uint32_t GetBestTimeMs(int stageNo);

	/**
	 * @brief クリアタイム(秒)でベストタイムを更新する（速い時だけ更新）
	 * @param stageNo 1..18
	 * @param clearTimeSec クリアタイム(秒)
	 * @return true: 更新された / false: 更新なし（遅い or 無効）
	 */
	bool UpdateBestTimeIfFaster(int stageNo, float clearTimeSec);

	/**
	 * @brief ステージID文字列からステージ番号を抽出（1..18）
	 * @param stageID "ST_001" / "Stage1" など
	 * @return 1..18。抽出できない場合は -1
	 */
	int ExtractStageNo(const std::string& stageID);

	/**
	 * @brief 指定ステージのスター獲得マスクを取得する
	 *
	 * bit0: 未発見クリア
	 * bit1: お宝を順番通りに全部回収
	 * bit2: 制限時間内にクリア
	 *
	 * @param stageNo 1..18
	 * @return 0..7
	 */
	std::uint8_t GetStageStarMask(int stageNo);

	/**
	 * @brief スター獲得マスクをORで更新（後から条件達成で増える）し保存する
	 * @param stageNo 1..18
	 * @param addMask 0..7
	 */
	void UpdateStageStarMaskOr(int stageNo, std::uint8_t addMask);

	/**
	 * @brief StageSelectの「今回だけ解放演出」用（保存しない）
	 */
	void SetPendingRevealStage(int stageNo);

	/**
	 * @brief SetPendingRevealStage の値を消費して取得
	 * @return stageNo(2..18) or -1
	 */
	int ConsumePendingRevealStage();
}