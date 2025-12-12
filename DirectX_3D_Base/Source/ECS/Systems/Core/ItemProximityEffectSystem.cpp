#include "ECS/Systems/Core/ItemProximityEffectSystem.h"
#include "ECS/ECS.h"
#include "ECS/EntityFactory.h"


void ItemProximityEffectSystem::Update(float deltaTime)
{
    if (!m_coordinator) return;

    // プレイヤーを1体取る（プレイヤーはTag"player"で作られてる）:contentReference[oaicite:5]{index=5}
    ECS::EntityID playerID = ECS::FindFirstEntityWithComponent<PlayerControlComponent>(m_coordinator);
    if (playerID == ECS::INVALID_ENTITY_ID) return;

    const auto& playerTr = m_coordinator->GetComponent<TransformComponent>(playerID);

    for (auto const& itemEntity : m_entities)
    {
        // お宝以外が混ざる可能性があるなら CollectableComponent でガード
        if (!m_coordinator->HasComponent<CollectableComponent>(itemEntity)) continue;

        auto& itemTr = m_coordinator->GetComponent<TransformComponent>(itemEntity);
        auto& sparkle = m_coordinator->GetComponent<ProximitySparkleEffectComponent>(itemEntity);
        if (!sparkle.enabled) continue;

        const float dx = playerTr.position.x - itemTr.position.x;
        const float dz = playerTr.position.z - itemTr.position.z; // Y無視（必要ならdyも足す）
        const float distSq = dx * dx + dz * dz;
        const float triggerSq = sparkle.triggerDistance * sparkle.triggerDistance;

        const bool inRange = (distSq <= triggerSq);

        if (inRange)
        {
            // 入った瞬間に1回出す（体感が良い）
            if (!sparkle.wasInRange)
            {
                DirectX::XMFLOAT3 pos = {
                    itemTr.position.x + sparkle.offset.x,
                    itemTr.position.y + sparkle.offset.y,
                    itemTr.position.z + sparkle.offset.z
                };
                sparkle.lastEffectEntity = ECS::EntityFactory::CreateOneShotEffect(
                    m_coordinator, sparkle.assetID, pos, sparkle.oneShotDuration, sparkle.effectScale
                );
                sparkle.timer = 0.0f;
            }
            else
            {
                sparkle.timer += deltaTime;
                if (sparkle.timer >= sparkle.interval)
                {
                    sparkle.timer = 0.0f;

                    DirectX::XMFLOAT3 pos = {
                        itemTr.position.x + sparkle.offset.x,
                        itemTr.position.y + sparkle.offset.y,
                        itemTr.position.z + sparkle.offset.z
                    };

                    sparkle.lastEffectEntity = ECS::EntityFactory::CreateOneShotEffect(
                        m_coordinator, sparkle.assetID, pos, sparkle.oneShotDuration, sparkle.effectScale
                    );
                }
            }
        }
        else
        {
            sparkle.timer = 0.0f;
        }

        sparkle.wasInRange = inRange;
    }
}
