/*****************************************************************//**
 * @file	Main.cpp
 * @brief	プログラムのエントリーポイント
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo / Oda Kaito
 * ------------------------------------------------------------
 * 
 * @date	2025/10/15	初回作成日
 * 			作業内容：	- 追加：ECSのコア定義（EntityID, ComponentTypeID, Signature）およびIDジェネレータを定義する `ECS.h` ファイルを作成。
 *                      - 追加：System抽象基底クラスを定義。
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
// Windows API の基本機能
#include <windows.h>

// 自作ヘッダファイル
#include "Main.h"
#include "Systems/DirectX/DirectX.h"
#include "Systems/DirectX/ShaderList.h"
#include "Systems/Geometory.h"
#include "Systems/Sprite.h"
#include "Systems/Input.h"

// Scene
#include "Scene/SceneManager.h"

#include <stdio.h>
#include <iostream>
#include <crtdbg.h>
#include <DirectXMath.h>

// timeGetTime周りの使用
#pragma comment(lib, "winmm.lib")

// ===== プロトタイプ宣言 =====
int Init(HINSTANCE hInstance, int nCmdShow);	// 初期化
void Uninit();									// 終了
void Update(float deltaTime);					// 更新
void Draw();									// 描画
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/**
 * [int  - WinMain]
 * @brief	プログラムのエントリーポイント
 * 
 * @param	[in] hInstance 
 * @param	[in] hPrevInstance 
 * @param	[in] lpCmdLine 
 * @param	[in] nCmdShow 
 * @return	int - 0.成功 1.失敗
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	/* メモリリークチェック */
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// ********************************************** //
	//          ▼ Console画面（デバッグ用）          //
	// ********************************************** //
#ifdef _DEBUG
	if (AllocConsole()) {
		FILE* fp = nullptr;
		// 標準出力(stdout)を新しいコンソールに向ける
		freopen_s(&fp, "CONOUT$", "w", stdout);
		// 標準エラー出力(stderr)も同様
		freopen_s(&fp, "CONOUT$", "w", stderr);
		// 標準入力(stdin)も同様
		freopen_s(&fp, "CONIN$", "r", stdin);
		// C++のcoutとCのstdoutを同期させる
		std::cout.sync_with_stdio();
	}
#endif

	// ******************************* //
	//          ▼ 初期化処理          //
	// ******************************* //
	if (FAILED(Init(hInstance, nCmdShow))) {
		Uninit();
		return 0;
	}
	
	// ********************************* //
	//          ▼ ゲームループ          //
	// ********************************* //
	/* FPS制御 */
	timeBeginPeriod(1);
	DWORD countStartTime = timeGetTime();
	DWORD preExecTime = countStartTime;

	/* メッセージループ */
	MSG message;
	while (1)
	{
		if (PeekMessage(&message, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage(&message, NULL, 0, 0))
			{
				break;
			}
			else
			{
				TranslateMessage(&message);
				DispatchMessage(&message);
			}
		}
		else
		{
			DWORD nowTime = timeGetTime();
			float diff = static_cast<float>(nowTime - preExecTime);
			if (diff >= 1000.0f / fFPS)
			{
				// ***************************** //
				//          ▼ 更新処理          //
				// ***************************** //
				Update(1000.0f / fFPS);

				// ***************************** //
				//          ▼ 描画処理          //
				// ***************************** //
				Draw();
				preExecTime = nowTime;
			}
		}
	}

	// ***************************** //
	//          ▼ 終了処理          //
	// ***************************** //
	timeEndPeriod(1);
	Uninit();

	return 0;
}

int Init(HINSTANCE hInstance, int nCmdShow)
{
	/* ウィンドウクラス情報の作成 */
	WNDCLASSEX wcex;
	// 指定されたアドレスの変数を0で初期化する関数
	ZeroMemory(&wcex, sizeof(wcex));
	/* ウィンドウクラス情報の設定 */
	wcex.hInstance = hInstance;									// アプリケーションの識別番号
	wcex.lpszClassName = "Class Name";
	wcex.lpfnWndProc = WndProc;									// ウィンドウプロシージャの設定（関数ポインタ）
	wcex.style = CS_CLASSDC | CS_DBLCLKS;						// ウィンドウの挙動
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);				// アプリのアイコン設定
	wcex.hIconSm = wcex.hIcon;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);					// マウスのアイコン設定
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);	// 背景の色

	/* ウィンドウクラス情報の登録 */
	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL, "Failed to RegisterClassEx", "Error", MB_OK);
		return 0;
	}

	/* ウィンドウの作成 */
	RECT rect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
	DWORD style = WS_CAPTION | WS_SYSMENU;
	DWORD exStyle = WS_EX_OVERLAPPEDWINDOW;
	AdjustWindowRectEx(&rect, style, false, exStyle);	// ウィンドウサイズの算出
	HWND hWnd = CreateWindowEx(
		exStyle,						// ウィンドウの見た目１
		wcex.lpszClassName, APP_TITLE,	// タイトルバーに表示する文字
		style,							// ウィンドウの見た目２
		CW_USEDEFAULT, CW_USEDEFAULT,	// ウィンドウの表示位置
		rect.right - rect.left,			// ウィンドウの大きさ
		rect.bottom - rect.top,
		HWND_DESKTOP, NULL, hInstance, NULL
	);
	if (hWnd == NULL)
	{
		MessageBox(NULL, "ウィンドウの作成に失敗", "Error", MB_OK);
		return 0;
	}

	/* ウィンドウの表示 */
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	/* DirectXの初期化 */
	if (FAILED(InitDirectX(hWnd, SCREEN_WIDTH, SCREEN_HEIGHT, false))) {
		MessageBox(hWnd, "DirectXの初期化に失敗", "エラー", MB_OK);
		return 0;
	}

	/* 多機能初期化 */
	Geometory::Init();	// Geometory
	Sprite::Init();		// Sprite
	InitInput();		// Input
	ShaderList::Init();	// ShaderList

	// シーンマネージャー
	SceneManager::Init();
	SceneManager::RegisterScene<TitleScene>();
	SceneManager::RegisterScene<GameScene>();
	SceneManager::RegisterScene<ResultScene>();
	SceneManager::ChangeScene<TitleScene>();

	return 0;
}

void Uninit()
{
	// シーンマネージャー
	SceneManager::Uninit();

	ShaderList::Uninit();
	UninitInput();
	Sprite::Uninit();
	Geometory::Uninit();
	UninitDirectX();
}

void Update(float deltaTime)
{
	UpdateInput();

	// シーンマネージャー
	SceneManager::Update(deltaTime);
}

void Draw()
{
	BeginDrawDirectX();
	
	SceneManager::Draw();

	EndDrawDirectX();
}

/**
 * [LRESULT - WndProc]
 * @brief	ウィンドウプロシージャ
 * 
 * @param	[in] hWnd 
 * @param	[in] message 
 * @param	[in] wParam 
 * @param	[in] lParam 
 * @return	
 */
LRESULT WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
		if (IDNO == MessageBox(hWnd, "ゲームを終了しますか？", "確認", MB_YESNO)) {
			return 0;
		}

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}