#pragma once
#include "Core.h"

namespace FINALITY
{
	class FAPI Game
	{
	public:
		virtual void Init() = 0;
		virtual void Update(float ts) = 0;
		virtual void Destroy() = 0;
	};
}