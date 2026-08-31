#pragma once

#include <vector>

#include <Graphics/GraphicsAPI/GraphicsAPI.h>
#include <Rendering/Buffer/BufferID.h>

namespace dooms
{
	class ThreeDModelMesh;

	namespace graphics
	{
		class Mesh;

		/// <summary>
		/// One simplified version of a mesh, as an index buffer over the
		/// vertices the mesh already uploaded.
		///
		/// The geometry pass in this engine submits about eight triangles for
		/// every pixel on screen, and costs 5.2 ms plus 1.37 ms per million
		/// triangles. A rock covering twenty pixels does not need two thousand
		/// triangles to do it, and the ones it does not need are not merely
		/// invisible: hardware rasterises in 2x2 quads, so a triangle smaller
		/// than a pixel wastes most of the shading it triggers.
		///
		/// Levels hold no vertices of their own. Vertex shading is driven by
		/// indices, so the vertices a coarse level stops referencing stop being
		/// paid for, and every level of a mesh shares one vertex buffer.
		/// </summary>
		struct MeshLodLevel
		{
			BufferID mIndexBuffer{};
			unsigned long long mIndexCount{ 0 };
		};

		/// <summary>
		/// The levels for one mesh, finest first. Empty when the mesh cannot be
		/// simplified usefully, in which case it is always drawn in full.
		/// </summary>
		struct MeshLodChain
		{
			std::vector<MeshLodLevel> mLevels;
			bool bmIsBuilt{ false };
		};

		/// <summary>
		/// The level chain for a mesh, built on first use.
		/// </summary>
		const MeshLodChain& GetMeshLodChain(const Mesh* const mesh);

		/// <summary>
		/// The coarsest level that still has at least one triangle per pixel the
		/// object covers, which is the point past which more geometry cannot be
		/// resolved. Returns null when the mesh should be drawn at full detail.
		/// </summary>
		const MeshLodLevel* SelectMeshLod(const Mesh* const mesh, const float coveredPixelCount);

		/// <summary>
		/// Meshes with levels, and levels built, for the overlay.
		/// </summary>
		void GetMeshLodStatistics(unsigned int& outMeshCount, unsigned int& outLevelCount);
	}
}
