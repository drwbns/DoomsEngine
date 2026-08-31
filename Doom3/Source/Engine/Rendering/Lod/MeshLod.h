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

			/// <summary>
			/// The level's own vertices, compacted.
			///
			/// Sharing the mesh's vertex buffer was tried first and measured: it
			/// costs no memory and it is slower than drawing the full mesh.
			/// Clustering leaves representatives scattered across the original
			/// buffer, so a coarse level fetches three vertices per triangle from
			/// random places in a buffer of a thousand, loses sequential
			/// prefetch, and loses the reuse that had each vertex serving six
			/// triangles. Cutting triangles by 73% made the pass 32% slower.
			/// </summary>
			unsigned long long mVertexCount{ 0 };
		};

		/// <summary>
		/// The levels for one mesh, finest first. Empty when the mesh cannot be
		/// simplified usefully, in which case it is always drawn in full.
		/// </summary>
		struct MeshLodChain
		{
			std::vector<MeshLodLevel> mLevels;

			/// <summary>
			/// One vertex buffer for every level of the mesh, laid out per
			/// attribute across all of them: all positions, then all texture
			/// coordinates, and so on, with each level occupying a contiguous
			/// run inside each attribute.
			///
			/// That arrangement is the whole point. Attribute offsets depend
			/// only on the total vertex count, so they are identical for every
			/// level, which means switching level does not rebind the vertex
			/// buffer. A buffer per level was tried first and cost more in binds
			/// than the triangles it saved: 1035 mesh binds became 2672, and
			/// each of those rebinds five attribute slots. Each level's indices
			/// are written already offset into its own run, so only the index
			/// buffer changes between levels.
			/// </summary>
			BufferID mSharedVertexBuffer{};
			unsigned int mLayoutOffsets[5]{ 0, 0, 0, 0, 0 };

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
		const MeshLodLevel* SelectMeshLod(const Mesh* const mesh, const float coveredPixelCount, const MeshLodChain** outChain);

		/// <summary>
		/// Meshes with levels, and levels built, for the overlay.
		/// </summary>
		void GetMeshLodStatistics(unsigned int& outMeshCount, unsigned int& outLevelCount);
	}
}
