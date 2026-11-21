/**
 * @file
 * @ingroup render
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for Frustum.h.
 */
#pragma once

#include <glm/glm.hpp>

namespace FINALITY
{
	/** @todo TODO: Document Frustum class semantics, ownership and usage. */
	class Frustum
	{
	public:
		Frustum() = default;

		static Frustum FromViewProjection(const glm::mat4& viewProjection);

		bool IntersectsSphere(const glm::vec3& center, float radius) const;
		bool IntersectsAABB(const glm::vec3& min, const glm::vec3& max) const;
	private:
		glm::vec4 m_Planes[6] = {};
	};
}