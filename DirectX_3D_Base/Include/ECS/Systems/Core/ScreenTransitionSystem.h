#pragma once

#include "ECS/ECS.h"
#include "ECS/Components/Core/ScreenTransitionComponent.h"

#include <cmath>
#include <functional>
#include <unordered_map>

namespace ECS
{
	class ScreenTransitionSystem : public System
	{
	public:
		void Init(Coordinator* coordinator);
		void Update(float dt);
		void Uninit();

		static float ComputeCoverScaleMul(float angleDeg, float screenW, float screenH, float safety = 1.02f);

		// ★追加：コールバック管理（コンポーネントに std::function を持たせない）
		static bool IsBusy(Coordinator* coordinator, EntityID overlay);
		static void SetCallbacks(EntityID overlay, std::function<void()> onBlack, std::function<void()> onFinished);
		static void ClearCallbacks(EntityID overlay);


	private:
		struct Callbacks
		{
			std::function<void()> onBlack;
			std::function<void()> onFinished;
		};

		static std::unordered_map<EntityID, Callbacks> s_callbacks;

		Coordinator* m_coordinator = nullptr;

		static float Clamp01(float x);
		static float SmoothStep01(float t);
		void UpdateOne(EntityID e, float dt);
	};
}
