#pragma once

#include <vector>

#include <Vector3.h>

namespace dooms
{
	namespace graphics
	{
		class Mesh;

		/// <summary>
		/// The convex hull of a mesh, in model space, cached per mesh.
		///
		/// Used as the occludee shape for occlusion culling instead of an axis
		/// aligned box. A box is a poor stand in for a rock: its projection is
		/// noticeably wider than the rock's outline, so it overlaps background
		/// the rock does not, and its nearest corner sits about 0.7 radii in
		/// front of the surface, so a rock tucked closely behind an occluder
		/// still pokes through it. Neither is fixed by a finer depth pyramid.
		///
		/// The hull is the tightest convex shape that still contains the mesh,
		/// which is the property the test depends on: a proxy that does not
		/// contain the object would cull things that are visible.
		/// </summary>
		struct OccludeeHull
		{
			std::vector<math::Vector3> mVertices;
			bool bmIsBuilt{ false };
		};

		/// <summary>
		/// The hull for a mesh, built on first use and kept.
		///
		/// Returns an empty hull when the mesh has no readable vertex data, and
		/// callers are expected to fall back to the bounding box for those
		/// rather than treating an empty hull as covering nothing.
		/// </summary>
		const OccludeeHull& GetOccludeeHull(const Mesh* const mesh);

		/// <summary>
		/// Total hull vertices across every mesh built so far, and the number of
		/// meshes. Per object cost is paid per hull vertex every frame, so this
		/// is the number that says whether the technique can be afforded.
		/// </summary>
		void GetOccludeeHullStatistics(unsigned int& outMeshCount, unsigned int& outTotalVertexCount);
	}
}
