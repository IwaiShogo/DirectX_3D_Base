#pragma once
#include <string>
#include <DirectXMath.h>
#include "ECS/ComponentRegistry.h"
#include "ECS/ECS.h" // EntityID / INVALID_ENTITY_ID

struct ProximitySparkleEffectComponent
{
    // 再生するEFKのアセットID（CSVに入れたやつ）
    std::string assetID = "EFK_ITEMGET";

    // 開始条件：この距離以内
    float triggerDistance = 2.0f;

    // キラキラ生成間隔（秒）
    float interval = 0.25f;

    // 1回のエフェクト寿命（秒）: interval と同じにすると「常に1個だけ」運用になって停止がきれい
    float oneShotDuration = 0.25f;

    // スケール
    float effectScale = 1.0f;

    // お宝からのオフセット（少し上で光らせる）
    DirectX::XMFLOAT3 offset = { 0.0f, 0.6f, 0.0f };

    // 実行時用
    float timer = 0.0f;
    bool wasInRange = false;

    // 「最後に生成したエフェクトEntity」= 獲得時に消す用
    ECS::EntityID lastEffectEntity = ECS::INVALID_ENTITY_ID;

    bool enabled = true;
};

REGISTER_COMPONENT_TYPE(ProximitySparkleEffectComponent)
