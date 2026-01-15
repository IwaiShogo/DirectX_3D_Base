#ifndef ___OPENING_CONTROL_SYSTEM_H___
#define ___OPENING_CONTROL_SYSTEM_H___

#include "ECS/ECS.h"

class OpeningControlSystem : public ECS::System
{
private:
    ECS::Coordinator* m_coordinator = nullptr;
    bool m_active = false;   // Åö OpeningíÜÇÃÇ› true

    ECS::EntityID s_backgroundEntity = ECS::INVALID_ENTITY_ID;
    ECS::EntityID s_UIbackgroundEntity = ECS::INVALID_ENTITY_ID;
    ECS::EntityID s_fadeEntity = ECS::INVALID_ENTITY_ID;

public:
    void Init(ECS::Coordinator* coordinator) override
    {
        m_coordinator = coordinator;
    }

    // OpeningäJéní ím
    void StartOpening();
    void ChangeBackground(int blockIndex);
    void CreateFadeEntity(float alpha);
    void CreateNextPromptImages();
    ;
    void Update(float deltaTime) override;
};

#endif // ___OPENING_CONTROL_SYSTEM_H___