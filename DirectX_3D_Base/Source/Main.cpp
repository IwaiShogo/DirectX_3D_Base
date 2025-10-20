/*****************************************************************//**
 * @file	Main.cpp
 * @brief	�v���O�����̃G���g���[�|�C���g
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/15	����쐬��
 * 			��Ɠ��e�F	- �ǉ��FECS�̃R�A��`�iEntityID, ComponentTypeID, Signature�j�����ID�W�F�l���[�^���`���� `ECS.h` �t�@�C�����쐬�B
 *                      - �ǉ��FSystem���ۊ��N���X���`�B
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/

// ===== �C���N���[�h =====
// Windows API �̊�{�@�\
#include <windows.h>

// ����w�b�_�t�@�C��
#include "Main.h"
#include "Systems/DirectX/DirectX.h"
#include "Systems/DirectX/ShaderList.h"
#include "Systems/Geometory.h"
#include "Systems/Sprite.h"
#include "Systems/Input.h"


#include <stdio.h>
#include <iostream>
#include <crtdbg.h>

// timeGetTime����̎g�p
#pragma comment(lib, "winmm.lib")

// ===== �v���g�^�C�v�錾 =====
int Init(HINSTANCE hInstance, int nCmdShow);	// ������
void Uninit();									// �I��
void Update();									// �X�V
void Draw();									// �`��
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/**
 * [int  - WinMain]
 * @brief	�v���O�����̃G���g���[�|�C���g
 * 
 * @param	[in] hInstance 
 * @param	[in] hPrevInstance 
 * @param	[in] lpCmdLine 
 * @param	[in] nCmdShow 
 * @return	int - 0.���� 1.���s
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	/* ���������[�N�`�F�b�N */
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// ********************************************** //
	//          �� Console��ʁi�f�o�b�O�p�j          //
	// ********************************************** //
#ifdef _DEBUG
	if (AllocConsole()) {
		FILE* fp = nullptr;
		// �W���o��(stdout)��V�����R���\�[���Ɍ�����
		freopen_s(&fp, "CONOUT$", "w", stdout);
		// �W���G���[�o��(stderr)�����l
		freopen_s(&fp, "CONOUT$", "w", stderr);
		// �W������(stdin)�����l
		freopen_s(&fp, "CONIN$", "r", stdin);
		// C++��cout��C��stdout�𓯊�������
		std::cout.sync_with_stdio();
	}
#endif

	// ******************************* //
	//          �� ����������          //
	// ******************************* //
	if (FAILED(Init(hInstance, nCmdShow))) {
		Uninit();
		return 0;
	}
	
	// ********************************* //
	//          �� �Q�[�����[�v          //
	// ********************************* //
	/* FPS���� */
	timeBeginPeriod(1);
	DWORD countStartTime = timeGetTime();
	DWORD preExecTime = countStartTime;

	/* ���b�Z�[�W���[�v */
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
				//          �� �X�V����          //
				// ***************************** //
				Update();

				// ***************************** //
				//          �� �`�揈��          //
				// ***************************** //
				Draw();
				preExecTime = nowTime;
			}
		}
	}

	// ***************************** //
	//          �� �I������          //
	// ***************************** //
	timeEndPeriod(1);
	Uninit();

	return 0;
}

int Init(HINSTANCE hInstance, int nCmdShow)
{
	/* �E�B���h�E�N���X���̍쐬 */
	WNDCLASSEX wcex;
	// �w�肳�ꂽ�A�h���X�̕ϐ���0�ŏ���������֐�
	ZeroMemory(&wcex, sizeof(wcex));
	/* �E�B���h�E�N���X���̐ݒ� */
	wcex.hInstance = hInstance;									// �A�v���P�[�V�����̎��ʔԍ�
	wcex.lpszClassName = "Class Name";
	wcex.lpfnWndProc = WndProc;									// �E�B���h�E�v���V�[�W���̐ݒ�i�֐��|�C���^�j
	wcex.style = CS_CLASSDC | CS_DBLCLKS;						// �E�B���h�E�̋���
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);				// �A�v���̃A�C�R���ݒ�
	wcex.hIconSm = wcex.hIcon;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);					// �}�E�X�̃A�C�R���ݒ�
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);	// �w�i�̐F

	/* �E�B���h�E�N���X���̓o�^ */
	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL, "Failed to RegisterClassEx", "Error", MB_OK);
		return 0;
	}

	/* �E�B���h�E�̍쐬 */
	RECT rect = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
	DWORD style = WS_CAPTION | WS_SYSMENU;
	DWORD exStyle = WS_EX_OVERLAPPEDWINDOW;
	AdjustWindowRectEx(&rect, style, false, exStyle);	// �E�B���h�E�T�C�Y�̎Z�o
	HWND hWnd = CreateWindowEx(
		exStyle,						// �E�B���h�E�̌����ڂP
		wcex.lpszClassName, APP_TITLE,	// �^�C�g���o�[�ɕ\�����镶��
		style,							// �E�B���h�E�̌����ڂQ
		CW_USEDEFAULT, CW_USEDEFAULT,	// �E�B���h�E�̕\���ʒu
		rect.right - rect.left,			// �E�B���h�E�̑傫��
		rect.bottom - rect.top,
		HWND_DESKTOP, NULL, hInstance, NULL
	);
	if (hWnd == NULL)
	{
		MessageBox(NULL, "�E�B���h�E�̍쐬�Ɏ��s", "Error", MB_OK);
		return 0;
	}

	/* �E�B���h�E�̕\�� */
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	/* DirectX�̏����� */
	if (FAILED(InitDirectX(hWnd, SCREEN_WIDTH, SCREEN_HEIGHT, false))) {
		MessageBox(hWnd, "DirectX�̏������Ɏ��s", "�G���[", MB_OK);
		return 0;
	}

	/* ���@�\������ */
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
	

	// �����̕\��
#ifdef _DEBUG
	// �O���b�h
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
	// ��
	Geometory::AddLine(DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3(size, 0, 0), DirectX::XMFLOAT4(1, 0, 0, 1));
	Geometory::AddLine(DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3(0, size, 0), DirectX::XMFLOAT4(0, 1, 0, 1));
	Geometory::AddLine(DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3(0, 0, size), DirectX::XMFLOAT4(0, 0, 1, 1));
	Geometory::AddLine(DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3(-size, 0, 0), DirectX::XMFLOAT4(0, 0, 0, 1));
	Geometory::AddLine(DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3(0, 0, -size), DirectX::XMFLOAT4(0, 0, 0, 1));

	Geometory::DrawLines();

	// �J�����̒l
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

	// �W�I���g���p�J����������
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

	Geometory::DrawBox();

	EndDrawDirectX();
}

/**
 * [LRESULT - WndProc]
 * @brief	�E�B���h�E�v���V�[�W��
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
		if (IDNO == MessageBox(hWnd, "�Q�[�����I�����܂����H", "�m�F", MB_YESNO)) {
			return 0;
		}

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}