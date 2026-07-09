#pragma once
#include <Core/Core.h>

namespace FINALITY
{
	class FAPI Scene
	{
	public:
		~Scene() = default;

		virtual void OnInit() = 0;
		virtual void OnUpdate(float ts) = 0;
		virtual void OnDestroy() = 0;
	};

	class FAPI EmptyScene : public Scene
	{
	public:
		void OnInit() override {}
		void OnUpdate(float ts) override {}
		void OnDestroy() override {}
	};
}