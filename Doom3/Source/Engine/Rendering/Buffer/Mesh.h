#pragma once
#include "Buffer.h"

#include <array>

#include <Physics/Collider/AABB.h>
#include <Physics/Collider/Sphere.h>
#include <Graphics/GraphicsAPI/GraphicsAPI.h>
#include <Graphics/GraphicsAPI/Manager/GraphicsAPIManager.h>

#include "Mesh.reflection.h"
namespace dooms
{

	class ThreeDModelMesh;

	namespace graphics
	{
		class Graphics_Server;
		class DOOM_API D_CLASS Mesh : public Buffer
		{
			GENERATE_BODY()

		private:

			struct D_STRUCT VertexBufferLayout : public DObject
			{
				GENERATE_BODY_VertexBufferLayout()

				D_PROPERTY()
				UINT32 mStride;

				D_PROPERTY()
				UINT32 mOffset;
			};
			
		private:

			D_PROPERTY()
			const ThreeDModelMesh* mTargetThreeDModelMesh;

			/// <summary>
			/// DX11 bind this buffer
			/// </summary>
			D_PROPERTY()
			BufferID mVertexDataBuffer;

			D_PROPERTY()
			BufferID mElementBufferObjectID;	

			/// <summary>
			///	DX11 doesn't have this concept. it just bind VertexDataBuffer
			///
			/// OpenGL bind this buffer
			/// </summary>
			D_PROPERTY()
			BufferID mVertexArrayObjectID;
		
			//UINT32 mVertexBufferObject; <- Use Buffer::data

			//const ThreeDModelMesh* mThreeDModelMesh; don't save ModelMeshAssetData
			D_PROPERTY()
			UINT64 mNumOfIndices;
			D_PROPERTY()
			UINT64 mNumOfVertices;
			D_PROPERTY()
			GraphicsAPI::ePrimitiveType mPrimitiveType;

			D_PROPERTY()
			UINT32 mVertexArrayFlag;

			D_PROPERTY()
			UINT32 mTotalStride;

			D_PROPERTY()
			UINT32 mVertexBufferLayoutCount;

			D_PROPERTY()
			std::array<VertexBufferLayout, 10> mVertexBufferLayouts;

			static UINT64 BOUND_VERTEX_ARRAY_ID; // only used for OPENGL
			inline static const UINT32 MAX_VERTEX_BUFFER_LAYOUT_COUNT = 32;
			static UINT64 BOUND_VERTEX_BUFFER_ID[MAX_VERTEX_BUFFER_LAYOUT_COUNT]; // for OPENGL, Only first element is used
			static UINT64 BOUND_INDEX_BUFFER_ID; // INDEX ( ELEMENT BUFFER )

			/*
			/// <summary>
			/// bind buffer array object
			/// </summary>
			/// <returns></returns>
			D_FUNCTION()
			FORCE_INLINE void BindBuffer() const noexcept final
			{
				D_ASSERT(mVertexArrayObjectID.IsValid());

				if (D_OVERLAP_BIND_CHECK_CHECK_IS_NOT_BOUND_AND_BIND_ID(VERTEX_ARRAY_TAG, mVertexArrayObjectID))
				{
					GraphicsAPI::BindVertexArrayObject(mVertexArrayObjectID);
				}
			}

			D_FUNCTION()
			FORCE_INLINE void BindElementBuffer() const noexcept
			{
				D_ASSERT(mElementBufferObjectID.IsValid());

				if (D_OVERLAP_BIND_CHECK_CHECK_IS_NOT_BOUND_AND_BIND_ID(INDEX_BUFFER_TAG, mElementBufferObjectID))
				{
					GraphicsAPI::BindIndexBufferObject(mElementBufferObjectID);
				}
			}
			*/

			/// <summary>
			/// this is local coordinate, you should map to your world coordinate
			/// </summary>
			D_PROPERTY()
			physics::AABB3D mAABB3D{math::Vector4{-1.0f}, math::Vector4{1.0f}};
			D_PROPERTY()
			physics::Sphere mSphere{math::Vector3{0.0f}, 1.0f};

			void OnSetPendingKill() override;

			void BindVertexArrayObject() const;

			void BindVertexBufferObject() const;
			void BindIndexBufferObject() const;
			void BindVertexBufferObject
			(
				const UINT32 bindingPosition,
				const UINT32 stride,
				const UINT32 offset
			) const;

			void CreateVertexArrayObjectIfNotExist();

	
		public:

			

			Mesh();
			virtual ~Mesh();
			
			Mesh
			(
				const UINT64 dataComponentCount,
				const UINT64 vertexCount,
				const void* data, 
				GraphicsAPI::ePrimitiveType primitiveType,
				UINT32 vertexArrayFlag,
				const UINT32* const indices,
				const UINT64 indiceCount,
				const bool dynamicWrite
			);
			Mesh(const ThreeDModelMesh& threeDModelMesh);
			Mesh& operator=(const ThreeDModelMesh& threeDModelMesh);

			Mesh(const Mesh&) = delete;
			Mesh& operator=(const Mesh&) = delete;

			Mesh(Mesh&&) noexcept = default;
			Mesh& operator=(Mesh&&) noexcept = default;

			const ThreeDModelMesh* GetTargetThreeDModelMesh() const;
			void DeleteBuffers() final;

			
			void CreateBufferObject
			(
				const UINT64 dataComponentCount,
				const UINT64 vertexCount,
				const void* data, 
				GraphicsAPI::ePrimitiveType primitiveType,
				UINT32 vertexArrayFlag,
				const UINT32* const indices,
				const UINT64 indiceCount,
				const bool dynamicWrite
			) noexcept;
			void CreateBufferObjectFromModelMesh(const ThreeDModelMesh& threeDModelMesh) noexcept;

			/* You can't update buffer partially in D3D11. Use map, unmap function.
			void UpdateVertexData(const long long int dataSize, const void* data, const long long int offsetInByte) const noexcept;
			*/


			inline static unsigned int MESH_BIND_COUNT = 0;
		inline static unsigned int INDEX_BIND_COUNT = 0;
		inline static unsigned long long INDEX_COUNT = 0;

		void Draw() const;

		/// <summary>
		/// One draw carrying instanceCount copies of this mesh.
		///
		/// The per instance model matrices come from a stream the caller has
		/// already bound; startInstanceLocation says where in it this run
		/// begins, so one buffer holding the whole frame can be drawn from in
		/// pieces without rebinding it.
		/// </summary>
		void DrawInstanced(const unsigned int instanceCount, const unsigned int startInstanceLocation) const;

		/// <summary>
		/// The instanced draw for a run that shares a detail level as well as
		/// a mesh and a material. Instancing and level of detail otherwise
		/// fight: one draw carries one index buffer, so the level has to be
		/// part of what defines a run rather than something chosen per object.
		/// </summary>
		void DrawInstancedWithLodBuffers(
			const BufferID& vertexBuffer,
			const unsigned int* const layoutOffsets,
			const BufferID& indexBuffer,
			const unsigned long long indexCount,
			const unsigned int instanceCount,
			const unsigned int startInstanceLocation) const;

		/// <summary>
		/// Forget which mesh is bound.
		///
		/// Call at the start of a draw loop. Anything outside this class that
		/// touches the graphics state -- ImGui, a debugger, a capture tool --
		/// leaves the cached answer wrong, and the cost of being wrong is
		/// drawing one mesh's indices against another's vertices.
		/// </summary>
		static void ResetBoundMeshCache();

		void BindVertexBufferObjectIfNotBound() const;

		/// <summary>
		/// Mesh binds issued since this was last called.
		/// </summary>
		static unsigned int GetAndResetMeshBindCount();
		static unsigned int GetAndResetIndexBindCount();

		/// <summary>
		/// Draw this mesh's vertices through a different index buffer.
		///
		/// This is how detail levels are drawn. Vertex shading is driven by
		/// indices, so a level that references fewer vertices costs less without
		/// needing a vertex buffer of its own, and every level of a mesh shares
		/// the one the mesh already uploaded.
		/// </summary>
		void DrawWithIndexBuffer(const BufferID& indexBuffer, const unsigned long long indexCount) const;

		/// <summary>
		/// Draw a detail level: its own vertices, its own indices, and the
		/// attribute offsets that belong to a buffer of that many vertices.
		///
		/// The offsets cannot be the mesh's own. Vertex data is five contiguous
		/// arrays, so where the texture coordinates begin depends on how many
		/// vertices come before them.
		/// </summary>
		void DrawWithLodBuffers(
			const BufferID& vertexBuffer,
			const unsigned int* const layoutOffsets,
			const BufferID& indexBuffer,
			const unsigned long long indexCount) const;

		/// <summary>
		/// Indices submitted since this was last called.
		/// </summary>
		static unsigned long long GetAndResetIndexCount();
			void DrawArray(const INT32 startVertexLocation, const UINT32 vertexCount) const;
			void DrawArray(const GraphicsAPI::ePrimitiveType primitiveType, const INT32 startVertexLocation, const INT32 vertexCount) const;

			static constexpr UINT32 GetStride(const UINT32 vertexArrayFlag);

			

			D_FUNCTION()
			virtual bool IsBufferGenerated() const final;

			D_FUNCTION()
			FORCE_INLINE bool IsElementBufferGenerated() const 
			{
				return mElementBufferObjectID.IsValid();
			}

			/*
			D_FUNCTION()
			void UpdateElementBuffer(const UINT32* indices, const UINT32 indiceCount);
			*/


			D_FUNCTION()
			const physics::AABB3D& GetBoundingBox() const;
			D_FUNCTION()
			const physics::Sphere& GetBoundingSphere() const;

			D_FUNCTION()
			const BufferID& GetVertexArrayObjectID() const;
			D_FUNCTION()
			const BufferID& GetElementBufferObjectID() const;

			/**
			 * \brief You should unmap mapped buffer object before use it 
			 * \param mapBufferAccessOption 
			 * \return 
			 */
			void* MapVertexDataBuffer(const dooms::graphics::GraphicsAPI::eMapBufferAccessOption mapBufferAccessOption);
			void UnmapVertexDataBuffer();
			void* MapElementBuffer(const dooms::graphics::GraphicsAPI::eMapBufferAccessOption mapBufferAccessOption);
			void UnmapElementBuffer();

			UINT64 GetNumOfIndices() const;
			UINT64 GetNumOfVertices() const;
		};
	}
}