#ifndef __INPUT_H__
#define __INPUT_H__

// ===== インクルード =====
#define WIN32_LEAN_AND_MEAN // 必要なAPIのみを含め、コンパイル時間を短縮
#include <Windows.h> 
#include <DirectXMath.h> // XMFLOAT2, XMFLOAT3を使用するため

#include <Xinput.h>
#pragma comment(lib, "xinput.lib")

#undef max
#undef min

// コントローラーボタン定数（XINPUT_GAMEPAD_XXXに対応）
enum GamePadButton
{
	BUTTON_A = XINPUT_GAMEPAD_A,
	BUTTON_B = XINPUT_GAMEPAD_B,
	BUTTON_X = XINPUT_GAMEPAD_X,
	BUTTON_Y = XINPUT_GAMEPAD_Y,
	BUTTON_UP = XINPUT_GAMEPAD_DPAD_UP,
	BUTTON_DOWN = XINPUT_GAMEPAD_DPAD_DOWN,
	BUTTON_LEFT = XINPUT_GAMEPAD_DPAD_LEFT,
	BUTTON_RIGHT = XINPUT_GAMEPAD_DPAD_RIGHT,
	BUTTON_BACK = XINPUT_GAMEPAD_BACK,
	BUTTON_START = XINPUT_GAMEPAD_START,
	BUTTON_LS = XINPUT_GAMEPAD_LEFT_SHOULDER,
	BUTTON_RS = XINPUT_GAMEPAD_RIGHT_SHOULDER,
};

HRESULT InitInput();
void UninitInput();
void UpdateInput();

bool IsKeyPress(BYTE key);
bool IsKeyTrigger(BYTE key);
bool IsKeyRelease(BYTE key);
bool IsKeyRepeat(BYTE key);

// マウス入力処理
DirectX::XMFLOAT2 GetMouseDelta();
bool IsMousePress(int mouseButton);

// コントローラーの状態を取得する関数
DirectX::XMFLOAT2 GetLeftStick();
DirectX::XMFLOAT2 GetRightStick();
bool IsButtonPress(GamePadButton button);
bool IsButtonTriggered(GamePadButton button);

#endif // __INPUT_H__