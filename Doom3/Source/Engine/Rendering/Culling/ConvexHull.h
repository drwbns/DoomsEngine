#pragma once

#include <vector>

#include <Vector3.h>

namespace dooms
{
	namespace graphics
	{
		/// <summary>
		/// The exact convex hull of a point set, by incremental insertion.
		///
		/// Exposed for the unit tests, which check the property everything that
		/// consumes a hull leans on: the result contains every input point.
		/// Produces an empty output when the points do not span three
		/// dimensions. Nothing here touches engine state, which is why it lives
		/// apart from OccludeeHull, whose cache pulls the mesh system with it.
		/// </summary>
		void BuildConvexHullVertices(const std::vector<math::Vector3>& points, std::vector<math::Vector3>& outVertices);

		/// <summary>
		/// Reduces hull vertices to a budget, expanding the kept set so every
		/// dropped vertex stays contained.
		///
		/// Exposed for the unit tests, because containment is the property the
		/// occlusion test depends on and the expansion step is the only thing
		/// keeping it.
		/// </summary>
		void DecimateHullConservatively(std::vector<math::Vector3>& hullVertices, const unsigned int vertexBudget);

		/// <summary>
		/// Whether a point lies inside the convex shape a vertex list defines.
		///
		/// Faces are recovered by testing every vertex triple for a supporting
		/// plane, which is cubic in the vertex count -- fine for the handful of
		/// vertices a test builds, unaffordable for a hull at runtime. This is
		/// test support, not a culling primitive.
		/// </summary>
		bool IsPointInsideHullVertices(const std::vector<math::Vector3>& hullVertices, const math::Vector3& point, const float epsilon);
	}
}
