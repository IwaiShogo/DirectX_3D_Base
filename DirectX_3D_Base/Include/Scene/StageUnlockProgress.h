#pragma once

#include <string>

/**
 * @file StageUnlockProgress.h
 * @brief ステージ解放進行（アンロック）を管理するユーティリティ。
 *
 * - 初回起動時はステージ1のみ解放
 * - ステージNをクリアしたら、次ステージ(N+1)を解放（最大6）
 * - 解放状態はファイルに保存
 * - 「今回StageSelect復帰で浮かび上がらせるステージ」はメモリ上のみ保持
 */
namespace StageUnlockProgress
{
	/**
	 * @brief 進行データを読み込む。
	 *
	 * 保存ファイルが無い/壊れている場合は「初回扱い」として
	 * 最大解放ステージを 1 にフォールバックする。
	 *
	 * さらに Debug ビルドでは、毎回ステージ1のみになるよう
	 * 読み込みを無効化し、保存ファイルも削除する。
	 */
	void Load();

	/**
	 * @brief 進行データを保存する。
	 *
	 * Debug ビルドでは保存しない（毎回ステージ1に戻すため）。
	 */
	void Save();

	/**
	 * @brief 現在の最大解放ステージ番号を取得する。
	 * @return 1..6
	 */
	int GetMaxUnlockedStage();

	/**
	 * @brief クリアしたステージIDから次のステージを解放する（保存も行う）。
	 *
	 * 例："ST_001" や "Stage1" などから数字を抽出し、N+1 を解放する。
	 *
	 * @param clearedStageID クリアしたステージID
	 * @return 今回「新規に解放された」ステージ番号(2..6)。新規解放が無ければ -1
	 */
	int UnlockNextStageFromClearedStageID(const std::string& clearedStageID);

	/**
	 * @brief StageSelect復帰時の「浮かび上がり演出」対象ステージを設定する。
	 *
	 * ※保存はしない（演出用の一時情報）
	 *
	 * @param stageNo 演出したいステージ番号（2..6）。無効値ならクリア。
	 */
	void SetPendingRevealStage(int stageNo);

	/**
	 * @brief 演出対象ステージ番号を取得し、内部状態をクリアする。
	 * @return stageNo（2..6） or -1
	 */
	int ConsumePendingRevealStage();
	/**
	 * @brief 進行データをリセットする
	 */
	void ResetProgress();
}
