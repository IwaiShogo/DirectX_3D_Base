/*****************************************************************//**
 * @file	Main.cpp
 * @brief	プログラムのエントリーポイント
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
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

#include <stdio.h>
#include <iostream>
#include <crtdbg.h>
#include <DirectXMath.h>

// timeGetTime周りの使用
#pragma comment(lib, "winmm.lib")

// ===== プロトタイプ宣言 =====
int Init(HINSTANCE hInstance, int nCmdShow);	// 初期化
void Uninit();									// 終了
void Update();									// 更新
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
				Update();

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

	return 0;
}

void Uninit()
{
	ShaderList::Uninit();
	UninitInput();
	Sprite::Uninit();
	Geometory::Uninit();
	UninitDirectX();
}

void Update()
{
	UpdateInput();
}

void Draw()
{
	BeginDrawDirectX();
	

	// 軸線の表示
#ifdef _DEBUG
	// グリッド
	DirectX::XMFLOAT4 lineColor(0.5f, 0.5f, 0.5f, 1.0f);
	float size = DEBUG_GRID_NUM * DEBUG_GRID_MARGIN;
	for (int i = 1; i <= DEBUG_GRID_NUM; ++i)
	{
		float grid = i * DEBUG_GRID_MARGIN;
		DirectX::XMFLOAT3 pos[2] = {
			DirectX::XMFLOAT3(grid, 0.0f, size),
			DirectX::XMFLOAT3(grid, 0.0f,-size),
		};
		Geometory::AddLine(pos[0], pos[1], lineColor);
		pos[0].x = pos[1].x = -grid;
		Geometory::AddLine(pos[0], pos[1], lineColor);
		pos[0].x = size;
		pos[1].x = -size;
		pos[0].z = pos[1].z = grid;
		Geometory::AddLine(pos[0], pos[1], lineColor);
		pos[0].z = pos[1].z = -grid;
		Geometory::AddLine(pos[0], pos[1], lineColor);
	}
	// 軸
	Geometory::AddLine(DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3(size, 0, 0), DirectX::XMFLOAT4(1, 0, 0, 1));
	Geometory::AddLine(DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3(0, size, 0), DirectX::XMFLOAT4(0, 1, 0, 1));
	Geometory::AddLine(DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3(0, 0, size), DirectX::XMFLOAT4(0, 0, 1, 1));
	Geometory::AddLine(DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3(-size, 0, 0), DirectX::XMFLOAT4(0, 0, 0, 1));
	Geometory::AddLine(DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3(0, 0, -size), DirectX::XMFLOAT4(0, 0, 0, 1));

	Geometory::DrawLines();

	// カメラの値
	static bool camAutoSwitch = false;
	static bool camUpDownSwitch = true;
	static float camAutoRotate = 1.0f;
	if (IsKeyTrigger(VK_RETURN)) {
		camAutoSwitch ^= true;
	}
	if (IsKeyTrigger(VK_SPACE)) {
		camUpDownSwitch ^= true;
	}

	DirectX::XMVECTOR camPos;
	if (camAutoSwitch) {
		camAutoRotate += 0.01f;
	}
	camPos = DirectX::XMVectorSet(
		cosf(camAutoRotate) * 5.0f,
		3.5f * (camUpDownSwitch ? 1.0f : -1.0f),
		sinf(camAutoRotate) * 5.0f,
		0.0f);

	// ジオメトリ用カメラ初期化
	DirectX::XMFLOAT4X4 mat[2];
	DirectX::XMStoreFloat4x4(&mat[0], DirectX::XMMatrixTranspose(
		DirectX::XMMatrixLookAtLH(
			camPos,
			DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
			DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
		)));
	DirectX::XMStoreFloat4x4(&mat[1], DirectX::XMMatrixTranspose(
		DirectX::XMMatrixPerspectiveFovLH(
			DirectX::XMConvertToRadians(60.0f), (float)SCREEN_WIDTH / SCREEN_HEIGHT, 0.1f, 1000.0f)
	));
	Geometory::SetView(mat[0]);
	Geometory::SetProjection(mat[1]);
#endif


	// ***** 後々ゲームシーンに移動 *****
	//--- １つ目の地面 
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(0.0f, -0.1f, 0.0f);   // 天面がグリッドよりも下に来るように移動 
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(10.0f, 0.2f, 10.0f); // 地面となるように、前後左右に広く、上下に狭くする 
	DirectX::XMMATRIX mat1 = S * T;
	mat1 = DirectX::XMMatrixTranspose(mat1);
	DirectX::XMFLOAT4X4 fMat; // 行列の格納先 
	DirectX::XMStoreFloat4x4(&fMat, mat1);
	Geometory::SetWorld(fMat); // ボックスに変換行列を設定 
	Geometory::DrawBox();

	//--- 2つ目の地面 
	T = DirectX::XMMatrixTranslation(0.0f, 1.0f, 0.0f);
	S = DirectX::XMMatrixScaling(0.2f, 3.0f, 0.2f);
	mat1 = S * T;
	mat1 = DirectX::XMMatrixTranspose(mat1);
	DirectX::XMStoreFloat4x4(&fMat, mat1);
	Geometory::SetWorld(fMat);
	Geometory::DrawBox();

	static float rad = 0.0f;
	static float newY;
	DirectX::XMMATRIX Rx = DirectX::XMMatrixRotationX(0.0f);
	DirectX::XMMATRIX Ry = DirectX::XMMatrixRotationY(rad);
	DirectX::XMMATRIX Rz = DirectX::XMMatrixRotationZ(0.0f);
	T = DirectX::XMMatrixTranslation(1.0f, newY, 0.0f);   // ゴールの場所へ移動 
	mat1 = Rx * Ry * Rz * T; // 移動➡回転？ 回転➡移動？ 
	mat1 = DirectX::XMMatrixTranspose(mat1);
	DirectX::XMStoreFloat4x4(&fMat, mat1);
	Geometory::SetWorld(fMat); // ボックスに変換行列を設定 
	// rad += 回転角の更新(速度はお任せ); の部分
	const float ROTATION_INCREMENT = 1.01f; // 1フレームあたり0.01ラジアン回転
	rad += ROTATION_INCREMENT;

	static float time = 0.0f; // 実行時間を模倣した変数
	const float CENTER_Y = 1.5f; // 振動の中心となるY座標 (元のTのY座標)
	const float AMPLITUDE = 0.5f; // 振動の振幅 (上下にどれだけ動くか)
	const float FREQUENCY = 2.0f; // 振動の周波数 (速度。値が大きいほど速い)
	const float TIME_INCREMENT = 0.05f; // 毎フレームの時間の進み具合
	time += TIME_INCREMENT;
	// Y軸座標の計算
	newY = CENTER_Y + AMPLITUDE * sin(time * FREQUENCY);

	// ラジアン値が360度（2π）を超えたらリセット
	if (rad > DirectX::XM_2PI)
	{
		rad -= DirectX::XM_2PI;
	}

	Geometory::DrawBox();

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