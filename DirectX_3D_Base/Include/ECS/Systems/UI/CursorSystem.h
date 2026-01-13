#ifndef __Cursor_System__
#define __Cursor_System__

#include "ECS/SystemManager.h"

class CursorSystem : public ECS::System
{
public:
    void Init(ECS::Coordinator* coordinator)
    {
        m_coordinator = coordinator;
    }

    void Update(float deltaTime);
private:
    ECS::Coordinator* m_coordinator = nullptr;
};
#endif // __Cursor_System__