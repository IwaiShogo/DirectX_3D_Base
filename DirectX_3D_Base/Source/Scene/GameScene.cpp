/*****************************************************************//**
 * @file	GameScene.cpp
 * @brief	ã‚²ãƒ¼ãƒ ã®ãƒ¡ã‚¤ãƒ³ãƒ­ã‚¸ãƒƒã‚¯ã‚’å«ã‚€ã‚·ãƒ¼ãƒ³ã‚¯ãƒ©ã‚¹ã®å®Ÿè£…ã€‚
 * 
 * @details	
 * ECSã®åˆæœŸåŒ–ã¨å®Ÿè¡Œã€ãƒ‡ãƒ¢Entityã®ä½œæˆãƒ­ã‚¸ãƒƒã‚¯ã‚’å†…åŒ…ã™ã‚‹ã€‚
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/10/27	åˆå›ä½œæˆæ—¥
 * 			ä½œæ¥­å†…å®¹ï¼š	- è¿½åŠ ï¼šECSã®ãƒ©ã‚¤ãƒ•ã‚µã‚¤ã‚¯ãƒ«ã¨ãƒ‡ãƒ¢ãƒ­ã‚¸ãƒƒã‚¯ã‚’ç®¡ç†ã™ã‚‹ `GameScene` ã‚¯ãƒ©ã‚¹ã®å®Ÿè£…ã€‚
 * 
 * @update	2025/xx/xx	æœ€çµ‚æ›´æ–°æ—¥
 * 			ä½œæ¥­å†…å®¹ï¼š	- è­¦å‚™å“¡AIã®è¿½åŠ ï¼š
 * 
 * @note	ï¼ˆçœç•¥å¯ï¼‰
 *********************************************************************/

// ===== ã‚¤ãƒ³ã‚¯ãƒ«ãƒ¼ãƒ‰ =====
#include "Scene/GameScene.h"

#include"Scene/StageinformationScene.h"
#include "Ecs/Components/ScoreManager.h"
#include "ECS/ECS.h"
#include "ECS/ECSInitializer.h"
#include "ECS/EntityFactory.h"
#include "ECS/Systems/Gameplay/CollectionSystem.h"
#include "ECS/Components/Core/GameStateComponent.h"
#include "Systems/Input.h"
#include "ECS/Systems/Core/GameSceneSystem.h"
#include "ECS/Components/ScoreManager.h"

#include <DirectXMath.h>
#include <iostream>
<<<<<<< HEAD
#include <typeindex> // SystemManagerã‹ã‚‰ã®RenderSystemå–å¾—ã«ä½¿ç”¨
 
// ===== é™çš„ãƒ¡ãƒ³ãƒãƒ¼å¤‰æ•°ã®å®šç¾© =====
// ä»–ã®ã‚·ã‚¹ãƒ†ãƒ ã‹ã‚‰ECSã«ã‚¢ã‚¯ã‚»ã‚¹ã™ã‚‹ãŸã‚ã®é™çš„ãƒã‚¤ãƒ³ã‚¿
ECS::Coordinator* GameScene::s_coordinator = nullptr;

using namespace DirectX;

// ===== GameScene ãƒ¡ãƒ³ãƒãƒ¼é–¢æ•°ã®å®Ÿè£… =====
=======
#include <typeindex> // SystemManager‚©‚ç‚ÌRenderSystemæ“¾‚Ég—p
#include <sstream> 

// ===== Ã“Iƒƒ“ƒo[•Ï”‚Ì’è‹` =====u
// ‘¼‚ÌƒVƒXƒeƒ€‚©‚çECS‚ÉƒAƒNƒZƒX‚·‚é‚½‚ß‚ÌÃ“Iƒ|ƒCƒ“ƒ^
ECS::Coordinator* GameScene::s_coordinator = nullptr;

using namespace DirectX;
int GameScene::s_StageNo = 1;
// ===== GameScene ƒƒ“ƒo[ŠÖ”‚ÌÀ‘• =====
>>>>>>> bc6b0b4fabf53592a5dfcb219b9ca5372cee877a

void GameScene::Init()
{
	// --- 1. ECS Coordinatorã®åˆæœŸåŒ– ---
	m_coordinator = std::make_shared<ECS::Coordinator>();

	// é™çš„ãƒã‚¤ãƒ³ã‚¿ã«ç¾åœ¨ã®Coordinatorã‚’è¨­å®š
	s_coordinator = m_coordinator.get();

	ECS::ECSInitializer::InitECS(m_coordinator);

    {
        auto system = m_coordinator->RegisterSystem<GameSceneSystem>();
        ECS::Signature signature;
        signature.set(m_coordinator->GetComponentTypeID<GameSceneComponent>());
        m_coordinator->SetSystemSignature<GameSceneSystem>(signature);
        system->Init(m_coordinator.get());
    }
	// --- 2. ƒXƒe[ƒWID‚Ìì¬ ---
	// —á: s_StageNo ‚ª 1 ‚È‚ç "ST_001"A 6 ‚È‚ç "ST_006" ‚Æ‚¢‚¤•¶š—ñ‚ğì‚é
	std::stringstream ss;
	ss << "ST_" << std::setfill('0') << std::setw(3) << s_StageNo;
	std::string stageID = ss.str();

<<<<<<< HEAD
	// --- 4. ãƒ‡ãƒ¢ç”¨Entityã®ä½œæˆ ---
=======
	std::cout << "Starting Stage No: " << s_StageNo << " (ID: " << stageID << ")" << std::endl;

	// --- 3. JSONƒRƒ“ƒtƒBƒO‚ğg‚Á‚ÄˆêŒ‚¶¬I ---
	// ‚ ‚È‚½‚ªì‚Á‚½uˆêŒ‚ŠÖ”v‚ÉAID‚ÆCoordinator‚ğ“n‚µ‚Ü‚·
	// ¦ŠÖ”–¼‚ÍÀÛ‚ÌƒR[ƒh‚É‡‚í‚¹‚Ä‘‚«Š·‚¦‚Ä‚­‚¾‚³‚¢
	ECS::EntityFactory::GenerateStageFromConfig(m_coordinator.get(), stageID);

	// --- 4. ‚»‚Ì‘¼‚Ì‹¤’ÊEntity‚Ìì¬ ---
>>>>>>> bc6b0b4fabf53592a5dfcb219b9ca5372cee877a
	ECS::EntityFactory::CreateAllDemoEntities(m_coordinator.get());
	ECS::EntityFactory::CreateGameSceneEntity(m_coordinator.get());
	}

void GameScene::Uninit()
{
	// 1. ECS Systemã®é™çš„ãƒªã‚½ãƒ¼ã‚¹ã‚’è§£æ”¾
	ECS::ECSInitializer::UninitECS();

	// Coordinatorã®ç ´æ£„ï¼ˆunique_ptrãŒè‡ªå‹•çš„ã«deleteã‚’å®Ÿè¡Œï¼‰
	m_coordinator.reset();

	// é™çš„ãƒã‚¤ãƒ³ã‚¿ã‚’ã‚¯ãƒªã‚¢
	s_coordinator = nullptr;

	std::cout << "GameScene::Uninit() - ECS Destroyed." << std::endl;
}

void GameScene::Update(float deltaTime)
{

	m_elapsedTime += deltaTime;

	// 2. ƒS[ƒ‹”»’èi‚Æ‚è‚ ‚¦‚¸ƒfƒoƒbƒO—p‚É 'G' ƒL[‚ÅƒS[ƒ‹ˆµ‚¢‚É‚µ‚Ü‚·j
	// ¦Œã‚ÅuƒvƒŒƒCƒ„[‚ªƒS[ƒ‹‚É“–‚½‚Á‚½‚ç truev‚É‚È‚é‚æ‚¤‚É‘‚«Š·‚¦‚Ä‚­‚¾‚³‚¢
	bool isGoal = IsKeyTrigger('G');

	// 3. ƒS[ƒ‹‚µ‚½‚Ìˆ—
	if (isGoal)
	{
		std::cout << "GOAL! Time: " << m_elapsedTime << std::endl;

		// ƒxƒXƒgƒ^ƒCƒ€‚ğ•Û‘¶ (Œ»İ‚ÌƒXƒe[ƒW”Ô†‚ÆAƒNƒŠƒAƒ^ƒCƒ€)
		ScoreManager::SaveBestTime(s_StageNo, m_elapsedTime);

		// ƒŠƒUƒ‹ƒg‰æ–ÊiStageinformationScenej‚Ö‘JˆÚ
		SceneManager::ChangeScene<StageinformationScene>();
		return;
	}

	if (IsKeyTrigger('Q') || IsButtonTriggered(BUTTON_A))
	{
		SceneManager::ChangeScene<GameScene>();
	}

	// ECSã®æ›´æ–°
	m_coordinator->UpdateSystems(deltaTime);

	if (IsKeyTrigger(VK_SPACE))
	{
		ECS::EntityFactory::CreateOneShotSoundEntity(m_coordinator.get(), "SE_TEST");
	}

}

void GameScene::Draw()
{
	
	// ã‚¨ãƒ³ãƒ†ã‚£ãƒ†ã‚£ã®æç”»
	if (auto system = ECS::ECSInitializer::GetSystem<RenderSystem>())
	{
		system->DrawSetup();
		system->DrawEntities();
	}

	// UIã®æç”»
	if (auto system = ECS::ECSInitializer::GetSystem<UIRenderSystem>())
	{
		system->Render();
	}
}