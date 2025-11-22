/*****************************************************************//**
 * @file	AnimationComponent.h
 * @brief	モデルのアニメーション再生に必要な状態を定義するComponent。
 * 
 * @details	
 * 再生中のアニメーションID、再生時間などを保持する。
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/23	初回作成日
 * 			作業内容：	- 追加：アニメーション再生状態を管理する`AnimationComponent`を作成
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___ANIMATION_COMPONENT_H___
#define ___ANIMATION_COMPONENT_H___

// ===== インクルード =====
#include "Systems/Model.h"
#include <map>

/**
 * @struct	AnimationComponent
 * @brief	アニメーション再生状態
 */
struct AnimationComponent
{
	// アニメーションの全状態を保持する構造体
	Model::AnimationState m_state;

	/**
	 * @brief コンストラクタ
	 */
	AnimationComponent()
	{
		// 初期化
		m_state.playNo = Model::ANIME_NONE;
		m_state.blendNo = Model::ANIME_NONE;
		m_state.parametric[0] = Model::ANIME_NONE;
		m_state.parametric[1] = Model::ANIME_NONE;
		m_state.blendTime = 0.0f;
		m_state.blendTotalTime = 0.0f;
		m_state.parametricBlend = 0.0f;
	}

	// ----------------------------------------
	// アニメーション再生API（ラッパー）
	// ----------------------------------------

	/**
	 * @brief アニメーションの再生を開始する
	 * @param[in] no 再生するアニメーション番号
	 * @param[in] loop ループ再生するかどうか
	 * @param[in] speed 再生速度
	 */
	void Play(Model::AnimeNo no, bool loop = true, float speed = 1.0f)
	{
		if (no == Model::ANIME_NONE) return;
		if (m_state.playNo == no) return; // 同じアニメーションなら無視

		// 通常再生の初期化
		m_state.playNo = no;
		m_state.infoMain.nowTime = 0.0f;
		m_state.infoMain.speed = speed;
		m_state.infoMain.isLoop = loop;

		// ブレンド等はリセット
		m_state.blendNo = Model::ANIME_NONE;
		m_state.blendTime = 0.0f;
		m_state.blendTotalTime = 0.0f;
	}

	/**
	 * @brief アニメーションのブレンド再生を開始する
	 * @param[in] no ブレンド再生するアニメーション番号
	 * @param[in] blendTime ブレンドに掛ける時間
	 * @param[in] loop ループフラグ
	 * @param[in] speed 再生速度
	 */
	void PlayBlend(Model::AnimeNo no, float blendTime, bool loop = true, float speed = 1.0f)
	{
		if (no == Model::ANIME_NONE) return;

		// ブレンドアニメの初期化
		m_state.blendNo = no;
		m_state.blendTime = 0.0f;
		m_state.blendTotalTime = blendTime;

		m_state.infoBlend.nowTime = 0.0f;
		m_state.infoBlend.speed = speed;
		m_state.infoBlend.isLoop = loop;
	}

	/**
	 * @brief アニメーションの合成設定 (パラメトリックアニメーション用)
	 * @param[in] no1 合成元アニメ1
	 * @param[in] no2 合成元アニメ2
	 * @param[in] loop ループ設定
	 */
	void SetParametric(Model::AnimeNo no1, Model::AnimeNo no2, bool loop = true)
	{
		m_state.playNo = Model::PARAMETRIC_ANIME;

		m_state.parametric[0] = no1;
		m_state.parametric[1] = no2;
		m_state.parametricBlend = 0.0f; // 初期ブレンド率

		// 両方のアニメーションを初期化
		for (int i = 0; i < 2; ++i) {
			m_state.infoParametric[i].nowTime = 0.0f;
			m_state.infoParametric[i].speed = 1.0f; // 速度はブレンド率更新時に調整されるのが理想だが一旦1.0
			m_state.infoParametric[i].isLoop = loop;
		}
	}

	/**
	 * @brief パラメトリックアニメーションのブレンド率設定
	 * @param[in] blendRate 合成割合 (0.0f ~ 1.0f)
	 */
	void SetParametricBlend(float blendRate)
	{
		m_state.parametricBlend = blendRate;
		// 必要に応じて、ここで再生速度の再計算などを入れることも可能
	}
};

// Component登録
#include "ECS/ComponentRegistry.h"
REGISTER_COMPONENT_TYPE(AnimationComponent)

#endif // !___ANIMATION_COMPONENT_H___