#pragma once

#include <vector>

namespace dooms
{
	class ThreeDModelMesh;

	namespace graphics
	{
		class Mesh;

		/// <summary>
		/// Simplified versions of a mesh, coarsest last, level zero being the
		/// original.
		///
		/// The geometry pass in this engine submits about eight triangles for
		/// every pixel on screen, and costs 5.2 ms plus 1.37 ms per million
		/// triangles. A rock covering twenty pixels does not need two thousand
		/// triangles to do it, and the ones it does not need are not merely
		/// invisible: hardware rasterises in 2x2 quads, so a triangle smaller
		/// than a pixel wastes most of the shading it triggers.
		///
		/// Levels share the original vertex buffer and differ only in their
		/// index buffer. Vertex shading is driven by indices, so the vertices a
		/// level does not reference cost nothing to keep.
		/// </summary>
		struct MeshLodChain
		{
			std::vector<Mesh*> mLevels;
			std::vector<unsigned long long> mIndexCounts;
			bool bmIsBuilt{ false };
		};

		/// <summary>
		/// The level chain for a mesh, built on first use.
		/// </summary>
		const MeshLodChain& GetMeshLodChain(const Mesh* const mesh);

		/// <summary>
		/// The coarsest level that still has at least one triangle per pixel the
		/// object covers, which is the point past which more geometry cannot be
		/// seen. Returns the mesh itself when no level is coarse enough, or when
		/// levels are switched off.
		/// </summary>
		const Mesh* SelectMeshLod(const Mesh* const mesh, const float coveredPixelCount);

		/// <summary>
		/// Levels built and the meshes they were built for, for the overlay.
		/// </summary>
		void GetMeshLodStatistics(unsigned int& outMeshCount, unsigned int& outLevelCount);
	}
}
