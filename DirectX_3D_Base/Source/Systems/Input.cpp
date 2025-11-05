// ===== インクルード =====
#include "Systems/Input.h" 
#include <cmath> 
#include <algorithm> // std::min を使用するため

using namespace DirectX;

// ===== 内部変数・定数 =====
namespace
{
	const int MAX_CONTROLLERS = 1; // 1Pコントローラーのみをサポート
	// XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE の推奨値
	const SHORT STICK_DEAD_ZONE = 7849;
	const float MAX_STICK_VALUE = 32767.0f;

	// XInputの状態を保持
	XINPUT_STATE g_controllerState[MAX_CONTROLLERS];
	XINPUT_STATE g_prevControllerState[MAX_CONTROLLERS];
}

//--- グローバル変数
BYTE g_keyTable[256];
BYTE g_oldTable[256];

HRESULT InitInput()
{
	// 一番最初の入力
	GetKeyboardState(g_keyTable);
	return S_OK;
}
void UninitInput()
{
}
void UpdateInput()
{
	// ---------------------------------
	// 1. キーボード状態の更新 (既存ロジック)
	// ---------------------------------
	// 古い入力を更新
	memcpy_s(g_oldTable, sizeof(g_oldTable), g_keyTable, sizeof(g_keyTable));
	// 現在の入力を取得
	GetKeyboardState(g_keyTable);

	// ---------------------------------
	// 【追加】2. コントローラー状態の更新
	// ---------------------------------
	for (int i = 0; i < MAX_CONTROLLERS; ++i)
	{
		// 前フレームの状態を保存
		g_prevControllerState[i] = g_controllerState[i];

		// 現在のコントローラーの状態を取得（1Pのみサポートと仮定）
		// XInputGetStateが失敗した場合（コントローラーが切断されているなど）でも、
		// g_controllerState[i] は前の状態（またはゼロ）を保持します。
		// ※ 戻り値チェックは不要
		XInputGetState(i, &g_controllerState[i]);
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
//    スティック入力処理（デッドゾーン＆正規化）
// ===========================================

XMFLOAT2 ApplyDeadZoneAndNormalize(SHORT x, SHORT y)
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
	return ApplyDeadZoneAndNormalize(
		g_controllerState[0].Gamepad.sThumbLX,
		g_controllerState[0].Gamepad.sThumbLY
	);
}

XMFLOAT2 GetRightStick()
{
	return ApplyDeadZoneAndNormalize(
		g_controllerState[0].Gamepad.sThumbRX,
		g_controllerState[0].Gamepad.sThumbRY
	);
}

// ===========================================
//    ボタン入力処理
// ===========================================

bool IsButtonPress(GamePadButton button)
{
	return (g_controllerState[0].Gamepad.wButtons & button) != 0;
}

bool IsButtonTriggered(GamePadButton button)
{
	WORD currentButtons = g_controllerState[0].Gamepad.wButtons;
	WORD prevButtons = g_prevControllerState[0].Gamepad.wButtons;

	// 現在押されていて、前フレームで押されていなかった場合
	return (currentButtons & button) && !(prevButtons & button);
}