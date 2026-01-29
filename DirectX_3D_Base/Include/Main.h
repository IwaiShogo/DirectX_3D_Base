/*****************************************************************//**
 * @file	Main.h
 * @brief	
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/15	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/
#ifndef ___MAIN_H___
#define ___MAIN_H___

// ===== 定数・マクロ定義 =====
#define NOMINMAX

/* リソースパス */
#define ASSET(path)	"Assets/"path

/* 3D空間定義 */
#define CMETER(value) (value * 0.01f)
#define METER(value) (value * 1.0f)
#define MSEC(value) (value / fFPS)
#define CMSEC(value) MSEC(CMETER(value))
static const float GRAVITY = 5.98f;
 
// @brief	画面サイズ
static const int SCREEN_WIDTH	= 1280;
static const int SCREEN_HEIGHT	= 720;

// @brief	タイトル
static const char* APP_TITLE = "記憶のカイトウ";

// @brief	グリッドサイズ（デバッグ用）
static const int	DEBUG_GRID_NUM = 10;				// グリッド中心から端までの線の本数
static const float	DEBUG_GRID_MARGIN = METER(1.0f);	// グリッド配置幅

// @brief	FPS制御
static const int FPS = 60;
static const float fFPS = static_cast<float>(FPS);

#endif // !___MAIN_H___