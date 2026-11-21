/**
 * @file
 * @ingroup core
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for Game.h.
 */
#pragma once
#include "Core.h"

namespace FINALITY
{
	/** @todo TODO: Document Game class semantics, ownership and usage. */
	class FAPI Game
	{
	public:
		virtual void Init() = 0;
		virtual void Update(float ts) = 0;
		virtual void Destroy() = 0;
	};
}