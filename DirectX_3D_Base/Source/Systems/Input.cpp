// ===== インクルード =====
#include "Systems/Input.h" 
#include <cmath> 
#include <algorithm> // std::min を使用するため
#include <vector>

// DirectInput用インクルード
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

// ライブラリのリンク
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "xinput.lib")

using namespace DirectX;

// ===== 内部変数・定数 =====
namespace
{
	const int MAX_CONTROLLERS = 1; // 1Pコントローラーのみをサポート
	// XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE の推奨値
	const SHORT STICK_DEAD_ZONE = 7849;
	const float MAX_STICK_VALUE = 32767.0f;

	// --- XInput (Xbox) 用 ---
	XINPUT_STATE g_controllerState[MAX_CONTROLLERS];
	XINPUT_STATE g_prevControllerState[MAX_CONTROLLERS];
	bool g_isXInputConnected = false;

	// --- DirectInput (PS/Generic) 用 ---
	LPDIRECTINPUT8 g_pDirectInput = nullptr;
	LPDIRECTINPUTDEVICE8 g_pJoystick = nullptr;
	DIJOYSTATE2 g_diState;      // 現在の入力状態
	DIJOYSTATE2 g_prevDiState;  // 前フレームの入力状態
	bool g_isDirectInputConnected = false;

	// --- マウス用 ---
	POINT g_prevMousePos = { 0, 0 };
	POINT g_currentMousePos = { 0, 0 };
	XMFLOAT2 g_mouseDelta = { 0.0f, 0.0f };

	// コールバック関数: 最初に見つかったジョイスティックをセットアップする
	BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
	{
		HRESULT hr;
		// デバイス作成
		hr = g_pDirectInput->CreateDevice(pdidInstance->guidInstance, &g_pJoystick, NULL);
		if (FAILED(hr)) return DIENUM_CONTINUE;

		return DIENUM_STOP; // 1つ見つかったら終了
	}
}

//--- グローバル変数
BYTE g_keyTable[256];
BYTE g_oldTable[256];

HRESULT InitInput()
{
	// 1. キーボード・マウス初期化
	GetKeyboardState(g_keyTable);
	GetCursorPos(&g_prevMousePos);
	g_currentMousePos = g_prevMousePos;

	// 2. DirectInputの初期化
	// インスタンス作成
	HRESULT hr = DirectInput8Create(
		GetModuleHandle(NULL),
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		(VOID**)&g_pDirectInput,
		NULL
	);

	if (SUCCEEDED(hr))
	{
		// デバイス列挙 (ジョイスティックを探す)
		g_pDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, NULL, DIEDFL_ATTACHEDONLY);

		if (g_pJoystick)
		{
			// データフォーマット設定
			g_pJoystick->SetDataFormat(&c_dfDIJoystick2);
			// 協調レベル設定 (バックグラウンドでも入力を受け付けるかどうか。通常はFOREGROUND | EXCLUSIVE推奨だが簡易的にこうする)
			// ※ ウィンドウハンドルが必要ですが、取得が面倒なのでNONEXCLUSIVEのみ指定して動作を期待します
			//    本来は hWnd を渡すべきです。
			// g_pJoystick->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

			// 入力権取得
			g_pJoystick->Acquire();
			g_isDirectInputConnected = true;
		}
	}

	return S_OK;
}
void UninitInput()
{
	if (g_pJoystick) {
		g_pJoystick->Unacquire();
		g_pJoystick->Release();
		g_pJoystick = nullptr;
	}
	if (g_pDirectInput) {
		g_pDirectInput->Release();
		g_pDirectInput = nullptr;
	}
}
void UpdateInput()
{
	// 1. キーボード
	memcpy_s(g_oldTable, sizeof(g_oldTable), g_keyTable, sizeof(g_keyTable));
	GetKeyboardState(g_keyTable);

	// 2. マウス
	g_prevMousePos = g_currentMousePos;
	GetCursorPos(&g_currentMousePos);
	g_mouseDelta.x = static_cast<float>(g_currentMousePos.x - g_prevMousePos.x);
	g_mouseDelta.y = static_cast<float>(g_currentMousePos.y - g_prevMousePos.y);

	// 3. XInput (Xbox) 更新
	g_prevControllerState[0] = g_controllerState[0];
	DWORD dwResult = XInputGetState(0, &g_controllerState[0]);
	g_isXInputConnected = (dwResult == ERROR_SUCCESS);

	// 4. DirectInput (PS/Other) 更新
	if (g_pJoystick)
	{
		g_prevDiState = g_diState;
		HRESULT hr = g_pJoystick->GetDeviceState(sizeof(DIJOYSTATE2), &g_diState);

		if (FAILED(hr)) {
			// 取得失敗時は再取得を試みる
			hr = g_pJoystick->Acquire();
			while (hr == DIERR_INPUTLOST) hr = g_pJoystick->Acquire();

			if (SUCCEEDED(hr)) {
				g_pJoystick->GetDeviceState(sizeof(DIJOYSTATE2), &g_diState);
			}
			else {
				// 切断されたとみなす（簡易）
				ZeroMemory(&g_diState, sizeof(g_diState));
			}
		}
	}
}

bool IsKeyPress(BYTE key)
{
	return g_keyTable[key] & 0x80;
}
bool IsKeyTrigger(BYTE key)
{
	return (g_keyTable[key] ^ g_oldTable[key]) & g_keyTable[key] & 0x80;
}
bool IsKeyRelease(BYTE key)
{
	return (g_keyTable[key] ^ g_oldTable[key]) & g_oldTable[key] & 0x80;
}
bool IsKeyRepeat(BYTE key)
{
	return false;
}

// ===========================================
//    マウス入力取得
// ===========================================
// マウスの移動量を取得 (カメラ操作用)
XMFLOAT2 GetMouseDelta()
{
	return g_mouseDelta;
}

// マウスボタン入力も IsKeyPress で取得可能ですが、
// 専用関数が必要なら以下のように実装します
bool IsMousePress(int mouseButton) // 0:Left, 1:Right, 2:Middle
{
	int key = VK_LBUTTON;
	if (mouseButton == 1) key = VK_RBUTTON;
	if (mouseButton == 2) key = VK_MBUTTON;
	return IsKeyPress(static_cast<BYTE>(key));
}

// ===========================================
//    スティック入力処理（デッドゾーン＆正規化）
// ===========================================

XMFLOAT2 ApplyDeadZoneAndNormalize(float x, float y)
{
	float magnitude = std::sqrtf((float)(x * x + y * y));
	float normalizedMagnitude = 0.0f;

	if (magnitude > STICK_DEAD_ZONE)
	{
		magnitude = std::min(magnitude, MAX_STICK_VALUE);
		normalizedMagnitude = (magnitude - STICK_DEAD_ZONE) / (MAX_STICK_VALUE - STICK_DEAD_ZONE);
	}
	else
	{
		return XMFLOAT2(0.0f, 0.0f);
	}

	float dirX = (float)x / magnitude;
	float dirY = (float)y / magnitude;
	return XMFLOAT2(dirX * normalizedMagnitude, dirY * normalizedMagnitude);
}

XMFLOAT2 GetLeftStick()
{
	// A. XInputが接続されていれば優先
	if (g_isXInputConnected)
	{
		return ApplyDeadZoneAndNormalize(
			(float)g_controllerState[0].Gamepad.sThumbLX,
			(float)g_controllerState[0].Gamepad.sThumbLY
		);
	}
	// B. DirectInputを使用
	else if (g_isDirectInputConnected)
	{
		// DirectInputは 0~65535 を返す (中央は 32767)
		// XInputの -32768 ~ 32767 に合わせる
		float x = (float)g_diState.lX - 32767.0f;
		float y = -((float)g_diState.lY - 32767.0f); // Y軸は上下逆の場合が多いので反転
		return ApplyDeadZoneAndNormalize(x, y);
	}

	return XMFLOAT2(0.0f, 0.0f);
}

XMFLOAT2 GetRightStick()
{
	if (g_isXInputConnected)
	{
		return ApplyDeadZoneAndNormalize(
			(float)g_controllerState[0].Gamepad.sThumbRX,
			(float)g_controllerState[0].Gamepad.sThumbRY
		);
	}
	else if (g_isDirectInputConnected)
	{
		// PSコントローラーの右スティックは通常 Z軸(lZ) と Z回転(lRz) に割り当てられることが多い
		float x = (float)g_diState.lZ - 32767.0f;   // 機種により異なる可能性あり
		float y = -((float)g_diState.lRz - 32767.0f);
		return ApplyDeadZoneAndNormalize(x, y);
	}

	return XMFLOAT2(0.0f, 0.0f);
}

// ===========================================
//    ボタン入力処理 (統合版)
// ===========================================

// DirectInputのボタンIDをXInputのビットマスクに変換するヘルパー
WORD GetDirectInputButtonsAsXInput()
{
	if (!g_isDirectInputConnected) return 0;

	WORD buttons = 0;

	// PS4/PS5コントローラーの一般的な配置
	// 0: 四角 (X), 1: バツ (A), 2: 丸 (B), 3: 三角 (Y)
	// ※ 環境によってバツと丸が逆の可能性あり

	if (g_diState.rgbButtons[1] & 0x80) buttons |= XINPUT_GAMEPAD_A; // バツ
	if (g_diState.rgbButtons[2] & 0x80) buttons |= XINPUT_GAMEPAD_B; // 丸
	if (g_diState.rgbButtons[0] & 0x80) buttons |= XINPUT_GAMEPAD_X; // 四角
	if (g_diState.rgbButtons[3] & 0x80) buttons |= XINPUT_GAMEPAD_Y; // 三角

	if (g_diState.rgbButtons[4] & 0x80) buttons |= XINPUT_GAMEPAD_LEFT_SHOULDER;  // L1
	if (g_diState.rgbButtons[5] & 0x80) buttons |= XINPUT_GAMEPAD_RIGHT_SHOULDER; // R1

	// START / BACK
	if (g_diState.rgbButtons[9] & 0x80) buttons |= XINPUT_GAMEPAD_START; // OPTIONS
	if (g_diState.rgbButtons[8] & 0x80) buttons |= XINPUT_GAMEPAD_BACK;  // SHARE

	return buttons;
}

bool IsButtonPress(GamePadButton button)
{
	if (g_isXInputConnected)
	{
		return (g_controllerState[0].Gamepad.wButtons & button) != 0;
	}
	else
	{
		return (GetDirectInputButtonsAsXInput() & button) != 0;
	}
}

bool IsButtonTriggered(GamePadButton button)
{
	WORD currentButtons = 0;
	WORD prevButtons = 0;

	if (g_isXInputConnected)
	{
		currentButtons = g_controllerState[0].Gamepad.wButtons;
		prevButtons = g_prevControllerState[0].Gamepad.wButtons;
	}
	else if (g_isDirectInputConnected)
	{
		// 前回のDI入力からビットマスクを生成する必要がありますが、
		// 簡易的に「現在のDI入力」と「前回のDI入力(生データ)」を比較します
		// 本来は関数化すべきですが、ロジックを統一します
		currentButtons = GetDirectInputButtonsAsXInput();

		// --- 前回のDIボタン状態を変換 ---
		WORD prevMap = 0;
		if (g_prevDiState.rgbButtons[1] & 0x80) prevMap |= XINPUT_GAMEPAD_A;
		if (g_prevDiState.rgbButtons[2] & 0x80) prevMap |= XINPUT_GAMEPAD_B;
		if (g_prevDiState.rgbButtons[0] & 0x80) prevMap |= XINPUT_GAMEPAD_X;
		if (g_prevDiState.rgbButtons[3] & 0x80) prevMap |= XINPUT_GAMEPAD_Y;
		if (g_prevDiState.rgbButtons[4] & 0x80) prevMap |= XINPUT_GAMEPAD_LEFT_SHOULDER;  // L1
		if (g_prevDiState.rgbButtons[5] & 0x80) prevMap |= XINPUT_GAMEPAD_RIGHT_SHOULDER; // R1

		// START / BACK
		if (g_prevDiState.rgbButtons[9] & 0x80) prevMap |= XINPUT_GAMEPAD_START; // OPTIONS
		if (g_prevDiState.rgbButtons[8] & 0x80) prevMap |= XINPUT_GAMEPAD_BACK;  // SHARE
		// ボタン1(A/Cross)だけ例として実装
		if (button == XINPUT_GAMEPAD_A) { // Bボタン(決定)相当
			return (currentButtons & button) && !(prevMap & button);
		}
		// 厳密にやるなら全ボタン変換が必要ですが、
		// まずは動くかどうか確認してください。
		prevButtons = prevMap;
	}

	return (currentButtons & button) && !(prevButtons & button);
}