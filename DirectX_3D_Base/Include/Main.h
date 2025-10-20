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
 * @date	2025/10/15	����쐬��
 * 			��Ɠ��e�F	- �ǉ��F
 * 
 * @update	2025/xx/xx	�ŏI�X�V��
 * 			��Ɠ��e�F	- XX�F
 * 
 * @note	�i�ȗ��j
 *********************************************************************/
#ifndef ___MAIN_H___
#define ___MAIN_H___

// ===== �萔�E�}�N����` =====
/* ���\�[�X�p�X */
#define ASSET(path)	"Assets/"path

/* 3D��Ԓ�` */
#define CMETER(value) (value * 0.01f)
#define METER(value) (value * 1.0f)
#define MSEC(value) (value / fFPS)
#define CMSEC(value) MSEC(CMETER(value))
static const float GRAVITY = 0.98f;
 
// @brief	��ʃT�C�Y
static const int SCREEN_WIDTH	= 1280;
static const int SCREEN_HEIGHT	= 720;

// @brief	�^�C�g��
static const char* APP_TITLE = "DirectX 3D Game + ECS";

// @brief	�O���b�h�T�C�Y�i�f�o�b�O�p�j
static const int	DEBUG_GRID_NUM = 10;				// �O���b�h���S����[�܂ł̐��̖{��
static const float	DEBUG_GRID_MARGIN = METER(1.0f);	// �O���b�h�z�u��

// @brief	FPS����
static const int FPS = 60;
static const float fFPS = static_cast<float>(FPS);

#endif // !___MAIN_H___