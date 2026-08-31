#include "Mesh.h"

#include <Graphics/graphicsSetting.h>


#include "../Asset/ThreeDModelAsset.h"
#include "eVertexArrayFlag.h"
#include "Matrix3x3.h"

UINT64 dooms::graphics::Mesh::BOUND_VERTEX_ARRAY_ID{(UINT64)-1};
UINT64 dooms::graphics::Mesh::BOUND_VERTEX_BUFFER_ID[MAX_VERTEX_BUFFER_LAYOUT_COUNT]{ (UINT64)-1 };
UINT64 dooms::graphics::Mesh::BOUND_INDEX_BUFFER_ID{ (UINT64)-1 };

dooms::graphics::Mesh::Mesh()
	:
	Buffer(),
	mVertexDataBuffer{},
	mVertexArrayObjectID{},
	mElementBufferObjectID{},
	mTargetThreeDModelMesh{nullptr},
	mNumOfVertices{ 0 },
	mNumOfIndices{ 0 },
	mPrimitiveType{ GraphicsAPI::ePrimitiveType::NONE },
	mVertexBufferLayoutCount(),
	mVertexBufferLayouts()
{

}

dooms::graphics::Mesh::Mesh
(
	const UINT64 dataComponentCount,
	const UINT64 vertexCount,
	const void* data, 
	GraphicsAPI::ePrimitiveType primitiveType,
	UINT32 vertexArrayFlag,
	const UINT32* const indices,
	const UINT64 indiceCount,
	const bool dynamicWrite
)
	:
	Buffer(),
	mVertexDataBuffer{},
	mVertexArrayObjectID{},
	mElementBufferObjectID{},
	mTargetThreeDModelMesh{ nullptr },
	mNumOfVertices{ 0 },
	mNumOfIndices{ 0 },
	mPrimitiveType{ GraphicsAPI::ePrimitiveType::NONE },
	mVertexBufferLayoutCount(),
	mVertexBufferLayouts()
{
	CreateBufferObject(dataComponentCount, vertexCount, data, primitiveType, vertexArrayFlag, indices, indiceCount, dynamicWrite);
}


dooms::graphics::Mesh::Mesh(const ThreeDModelMesh& threeDModelMesh)
	:
	Buffer(),
	mVertexDataBuffer{},
	mVertexArrayObjectID{},
	mElementBufferObjectID{},
	mTargetThreeDModelMesh{ nullptr },
	mNumOfVertices{ 0 },
	mNumOfIndices{ 0 },
	mPrimitiveType{ GraphicsAPI::ePrimitiveType::NONE },
	mVertexBufferLayoutCount(),
	mVertexBufferLayouts()
{
	CreateBufferObjectFromModelMesh(threeDModelMesh);
}

dooms::graphics::Mesh::~Mesh()
{
	DeleteBuffers();
	
}

void dooms::graphics::Mesh::OnSetPendingKill()
{
	Buffer::OnSetPendingKill();

	DeleteBuffers();
}

void dooms::graphics::Mesh::BindVertexArrayObject() const
{
	if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
	{
		D_ASSERT(mVertexArrayObjectID.IsValid() == true);
		if (BOUND_VERTEX_ARRAY_ID != mVertexArrayObjectID.GetBufferID())
		{
			BOUND_VERTEX_ARRAY_ID = mVertexArrayObjectID;
			dooms::graphics::GraphicsAPI::BindVertexArrayObject(mVertexArrayObjectID);
		}
	}
}

void dooms::graphics::Mesh::BindVertexBufferObject() const
{
	D_ASSERT(mVertexDataBuffer.IsValid() == true);
	D_ASSERT(mTotalStride > 0);

	if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
	{
		if (BOUND_VERTEX_BUFFER_ID[0] != mVertexArrayObjectID.GetBufferID())
		{
			BOUND_VERTEX_BUFFER_ID[0] = mVertexArrayObjectID;
			dooms::graphics::GraphicsAPI::BindVertexDataBuffer
			(
				mVertexDataBuffer,
				0,
				mTotalStride,
				0
			);
		}
	}
	else if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::D3D11)
	{
		for (UINT32 bufferLayoutIndex = 0; bufferLayoutIndex < mVertexBufferLayoutCount; bufferLayoutIndex++)
		{
			if (BOUND_VERTEX_BUFFER_ID[bufferLayoutIndex] != mVertexDataBuffer.GetBufferID())
			{
				BOUND_VERTEX_BUFFER_ID[bufferLayoutIndex] = mVertexDataBuffer;
				dooms::graphics::GraphicsAPI::BindVertexDataBuffer
				(
					mVertexDataBuffer,
					bufferLayoutIndex,
					mVertexBufferLayouts[bufferLayoutIndex].mStride,
					mVertexBufferLayouts[bufferLayoutIndex].mOffset
				);
			}
		}

	}
	else
	{
		ASSUME_ZERO;
	}

}


void dooms::graphics::Mesh::DeleteBuffers()
{
	if (mVertexDataBuffer.IsValid())
	{
		GraphicsAPI::DestroyBuffer(mVertexDataBuffer);
		mVertexDataBuffer.Reset();
	}

	if (mElementBufferObjectID.IsValid())
	{
		GraphicsAPI::DestroyBuffer( mElementBufferObjectID );
		mElementBufferObjectID.Reset();
	}

	if (mVertexArrayObjectID.IsValid())
	{
		GraphicsAPI::DestroyVertexArrayObject(mVertexArrayObjectID);
		mVertexArrayObjectID.Reset();
	}
}

dooms::graphics::Mesh& dooms::graphics::Mesh::operator=(const ThreeDModelMesh& threeDModelMesh)
{
	DeleteBuffers();
	
	CreateBufferObjectFromModelMesh(threeDModelMesh);
	return *this;
}


const dooms::ThreeDModelMesh* dooms::graphics::Mesh::GetTargetThreeDModelMesh() const
{
	return mTargetThreeDModelMesh;
}

void dooms::graphics::Mesh::CreateBufferObject
(
	const UINT64 dataComponentCount,
	const UINT64 vertexCount,
	const void* data, 
	GraphicsAPI::ePrimitiveType primitiveType,
	UINT32 vertexArrayFlag,
	const UINT32* const indices,
	const UINT64 indiceCount,
	const bool dynamicWrite
) noexcept
{
	D_ASSERT(IsBufferGenerated() == false);
	if (IsBufferGenerated() == false)
	{
		if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
		{
			CreateVertexArrayObjectIfNotExist();
		}

		const UINT32 stride = Mesh::GetStride(vertexArrayFlag);
		mTotalStride = stride;

		BindVertexArrayObject(); // bind vertex array buffer

		D_DEBUG_LOG(eLogType::D_LOG, "%f", sizeof(FLOAT32) * dataComponentCount);
		mVertexDataBuffer = GraphicsAPI::CreateBufferObject
		(
			GraphicsAPI::eBufferTarget::ARRAY_BUFFER, 
			static_cast<unsigned long long>(sizeof(FLOAT32) * dataComponentCount), 
			data,
			dynamicWrite
		);
		BindVertexBufferObject();
		ResetBoundMeshCache();
		
#pragma warning( disable : 4312 )

		D_ASSERT(((vertexArrayFlag & eVertexArrayFlag::VertexVector2)) > 0 != ((vertexArrayFlag & eVertexArrayFlag::VertexVector3) > 0));

		UINT64 offset = 0;
		UINT32 vertexLayoutCount = 0;
		UINT32 layoutIndex = 0;
		if (vertexArrayFlag & eVertexArrayFlag::VertexVector2)
		{
			if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
			{
				//mVertex
				GraphicsAPI::EnableVertexAttributeArrayIndex(layoutIndex);
				GraphicsAPI::DefineVertexAttributeLayout(mVertexDataBuffer, layoutIndex, 2, sizeof(math::Vector2), offset);
			}

			mVertexBufferLayouts[vertexLayoutCount].mOffset = offset;
			mVertexBufferLayouts[vertexLayoutCount].mStride = sizeof(math::Vector2);
			vertexLayoutCount++;
			layoutIndex++;

			offset += vertexCount * sizeof(math::Vector2);
		}

		if (vertexArrayFlag & eVertexArrayFlag::VertexVector3)
		{
			if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
			{
				//mVertex
				GraphicsAPI::EnableVertexAttributeArrayIndex(layoutIndex);
				GraphicsAPI::DefineVertexAttributeLayout(mVertexDataBuffer, layoutIndex, 3, sizeof(math::Vector3), offset);
			}

			mVertexBufferLayouts[vertexLayoutCount].mOffset = offset;
			mVertexBufferLayouts[vertexLayoutCount].mStride = sizeof(math::Vector3);
			vertexLayoutCount++;
			layoutIndex++;

			offset += vertexCount * sizeof(math::Vector3);
		}

		if (vertexArrayFlag & eVertexArrayFlag::TexCoord)
		{
			if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
			{
				//mTexCoord
				GraphicsAPI::EnableVertexAttributeArrayIndex(layoutIndex);
				GraphicsAPI::DefineVertexAttributeLayout(mVertexDataBuffer, layoutIndex, 3, sizeof(math::Vector3), offset);
			}

			mVertexBufferLayouts[vertexLayoutCount].mOffset = offset;
			mVertexBufferLayouts[vertexLayoutCount].mStride = sizeof(math::Vector3);
			vertexLayoutCount++;
			layoutIndex++;

			offset += vertexCount * sizeof(math::Vector3);
		}

		if (vertexArrayFlag & eVertexArrayFlag::mNormal)
		{
			if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
			{
				//mNormal
				GraphicsAPI::EnableVertexAttributeArrayIndex(layoutIndex);
				GraphicsAPI::DefineVertexAttributeLayout(mVertexDataBuffer, layoutIndex, 3, sizeof(math::Vector3), offset);
			}

			mVertexBufferLayouts[vertexLayoutCount].mOffset = offset;
			mVertexBufferLayouts[vertexLayoutCount].mStride = sizeof(math::Vector3);
			vertexLayoutCount++;
			layoutIndex++;

			offset += vertexCount * sizeof(math::Vector3);
		}

		if (vertexArrayFlag & eVertexArrayFlag::mTangent)
		{
			if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
			{
				//mTangent
				GraphicsAPI::EnableVertexAttributeArrayIndex(layoutIndex);
				GraphicsAPI::DefineVertexAttributeLayout(mVertexDataBuffer, layoutIndex, 3, sizeof(math::Vector3), offset);
			}

			mVertexBufferLayouts[vertexLayoutCount].mOffset = offset;
			mVertexBufferLayouts[vertexLayoutCount].mStride = sizeof(math::Vector3);
			vertexLayoutCount++;
			layoutIndex++;

			offset += vertexCount * sizeof(math::Vector3);
		}

		if (vertexArrayFlag & eVertexArrayFlag::mBitangent)
		{
			if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
			{
				//mBitangent
				GraphicsAPI::EnableVertexAttributeArrayIndex(layoutIndex);
				GraphicsAPI::DefineVertexAttributeLayout(mVertexDataBuffer, layoutIndex, 3, sizeof(math::Vector3), offset);
			}

			mVertexBufferLayouts[vertexLayoutCount].mOffset = offset;
			mVertexBufferLayouts[vertexLayoutCount].mStride = sizeof(math::Vector3);
			vertexLayoutCount++;
			layoutIndex++;

			offset += vertexCount * sizeof(math::Vector3);
		}

		if (vertexArrayFlag & eVertexArrayFlag::mTBN)
		{
			for(size_t i = 0 ; i < 3 ; i++)
			{
				//const UINT64 offset = static_cast<unsigned int>((char*)threeDModelMesh.mMeshDatas.mBitangent - threeDModelMesh.mMeshDatas.mData);
				if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
				{
					//mBitangent
					GraphicsAPI::EnableVertexAttributeArrayIndex(layoutIndex);
					GraphicsAPI::DefineVertexAttributeLayout(mVertexDataBuffer, layoutIndex, 3, sizeof(math::Vector3), offset);
				}

				mVertexBufferLayouts[vertexLayoutCount].mOffset = offset;
				mVertexBufferLayouts[vertexLayoutCount].mStride = sizeof(math::Vector3);
				vertexLayoutCount++;
				layoutIndex++;

				offset += vertexCount * sizeof(math::Vector3);
			}
			
		}

		mVertexBufferLayoutCount = vertexLayoutCount;

#pragma warning( disable : 4244 )
		mNumOfVertices = vertexCount;
		mNumOfIndices = indiceCount;
		if (mNumOfIndices > 0)
		{
			mElementBufferObjectID = GraphicsAPI::CreateBufferObject(GraphicsAPI::eBufferTarget::ELEMENT_ARRAY_BUFFER, mNumOfIndices * sizeof(UINT32), NULL, false);
			GraphicsAPI::UpdateDataToBuffer(mElementBufferObjectID, GraphicsAPI::eBufferTarget::ELEMENT_ARRAY_BUFFER, 0, mNumOfIndices * sizeof(UINT32), reinterpret_cast<const void*>(indices));
		}

		mPrimitiveType = primitiveType;
		mVertexArrayFlag = vertexArrayFlag;

	}
}

/*
void dooms::graphics::Mesh::UpdateVertexData
(
	const long long int dataSize,
	const void* data,
	const long long int offsetInByte
) const noexcept
{
	D_ASSERT(mVertexDataBuffer.IsValid());
	D_ASSERT_LOG()
	
	BindVertexBufferObject();
	ResetBoundMeshCache();
	GraphicsAPI::UpdateDataToBuffer(mVertexDataBuffer, GraphicsAPI::ARRAY_BUFFER, offsetInByte, dataSize, data);
}
*/


void dooms::graphics::Mesh::BindIndexBufferObject() const
{
	D_ASSERT(mVertexDataBuffer.IsValid() == true);
	if (BOUND_INDEX_BUFFER_ID != mElementBufferObjectID.GetBufferID())
	{
		INDEX_BIND_COUNT++;
		BOUND_INDEX_BUFFER_ID = mElementBufferObjectID;
		dooms::graphics::GraphicsAPI::BindBuffer(mElementBufferObjectID, 0, graphics::GraphicsAPI::eBufferTarget::ELEMENT_ARRAY_BUFFER, graphics::GraphicsAPI::eGraphicsPipeLineStage::DUMMY);
	}
}

void dooms::graphics::Mesh::BindVertexBufferObject(const UINT32 bindingPosition, const UINT32 stride, const UINT32 offset) const
{
	D_ASSERT(mVertexDataBuffer.IsValid() == true);
	if (BOUND_VERTEX_BUFFER_ID[bindingPosition] != mVertexDataBuffer.GetBufferID())
	{
		BOUND_VERTEX_BUFFER_ID[bindingPosition] = mVertexDataBuffer;
		dooms::graphics::GraphicsAPI::BindVertexDataBuffer
		(
			mVertexDataBuffer,
			bindingPosition,
			stride,
			offset
		);
	}
}

void dooms::graphics::Mesh::CreateVertexArrayObjectIfNotExist()
{
	D_ASSERT(graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL);
	
	if (mVertexArrayObjectID.IsValid() == false)
	{
		mVertexArrayObjectID = GraphicsAPI::CreateVertexArrayObject();
	}
	BindVertexArrayObject(); // bind vertex array buffer first
}

void dooms::graphics::Mesh::CreateBufferObjectFromModelMesh(const ThreeDModelMesh& threeDModelMesh) noexcept
{
	D_ASSERT(IsBufferGenerated() == false);
	if (IsBufferGenerated() == false)
	{
		if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
		{
			CreateVertexArrayObjectIfNotExist();
		}

		mTotalStride = GetStride(mVertexArrayFlag);

		BindVertexArrayObject(); // bind vertex array buffer

		mVertexDataBuffer = GraphicsAPI::CreateBufferObject(GraphicsAPI::eBufferTarget::ARRAY_BUFFER, threeDModelMesh.mMeshDatas.GetAllocatedDataSize(), threeDModelMesh.mMeshDatas.mData, false);
		D_ASSERT(mVertexDataBuffer.IsValid());
		//GraphicsAPI::UpdateDataToBuffer(mVertexDataBuffer, GraphicsAPI::eBufferTarget::ARRAY_BUFFER, 0, threeDModelMesh.mMeshDatas.GetAllocatedDataSize(), reinterpret_cast<const void*>(threeDModelMesh.mMeshDatas.mData));

		UINT32 vertexLayoutCount = 0;
		UINT32 layoutIndex = 0;

		//mVertex
		if (threeDModelMesh.mVertexArrayFlag & eVertexArrayFlag::VertexVector3)
		{
			const UINT32 stride = sizeof(*(threeDModelMesh.mMeshDatas.mVertex));
			const UINT64 offset = static_cast<unsigned int>((char*)threeDModelMesh.mMeshDatas.mVertex - threeDModelMesh.mMeshDatas.mData);

			if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
			{
				GraphicsAPI::EnableVertexAttributeArrayIndex(0);
				GraphicsAPI::EnableVertexAttributeArrayIndex(0); GraphicsAPI::DefineVertexAttributeLayout(mVertexDataBuffer, layoutIndex, 3, stride, offset);
			}

			mVertexBufferLayouts[vertexLayoutCount].mOffset = offset;
			mVertexBufferLayouts[vertexLayoutCount].mStride = stride;
			vertexLayoutCount++;
			layoutIndex++;
		}

		//mTexCoord
		if (threeDModelMesh.mVertexArrayFlag & eVertexArrayFlag::TexCoord)
		{
			const UINT32 stride = sizeof(*(threeDModelMesh.mMeshDatas.mTexCoord));
			const UINT64 offset = static_cast<unsigned int>((char*)threeDModelMesh.mMeshDatas.mTexCoord - threeDModelMesh.mMeshDatas.mData);

			if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
			{
				GraphicsAPI::EnableVertexAttributeArrayIndex(1);
				GraphicsAPI::DefineVertexAttributeLayout(mVertexDataBuffer, layoutIndex, 3, stride, offset);
			}

			mVertexBufferLayouts[vertexLayoutCount].mOffset = offset;
			mVertexBufferLayouts[vertexLayoutCount].mStride = stride;
			vertexLayoutCount++;
			layoutIndex++;
		}

		//mNormal
		if (threeDModelMesh.mVertexArrayFlag & eVertexArrayFlag::mNormal)
		{
			const UINT32 stride = sizeof(*(threeDModelMesh.mMeshDatas.mNormal));
			const UINT64 offset = static_cast<unsigned int>((char*)threeDModelMesh.mMeshDatas.mNormal - threeDModelMesh.mMeshDatas.mData);

			if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
			{
				GraphicsAPI::EnableVertexAttributeArrayIndex(2);
				GraphicsAPI::DefineVertexAttributeLayout(mVertexDataBuffer, layoutIndex, 3, stride, offset);
			}

			mVertexBufferLayouts[vertexLayoutCount].mOffset = offset;
			mVertexBufferLayouts[vertexLayoutCount].mStride = stride;
			vertexLayoutCount++;
			layoutIndex++;
		}

		//mTangent
		if (threeDModelMesh.mVertexArrayFlag & eVertexArrayFlag::mTangent)
		{
			const UINT32 stride = sizeof(*(threeDModelMesh.mMeshDatas.mTangent));
			const UINT64 offset = static_cast<unsigned int>((char*)threeDModelMesh.mMeshDatas.mTangent - threeDModelMesh.mMeshDatas.mData);

			if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
			{
				GraphicsAPI::EnableVertexAttributeArrayIndex(3);
				GraphicsAPI::DefineVertexAttributeLayout(mVertexDataBuffer, layoutIndex, 3, stride, offset);
			}

			mVertexBufferLayouts[vertexLayoutCount].mOffset = offset;
			mVertexBufferLayouts[vertexLayoutCount].mStride = stride;
			vertexLayoutCount++;
			layoutIndex++;
		}

		if (threeDModelMesh.mVertexArrayFlag & eVertexArrayFlag::mBitangent)
		{
			const UINT32 stride = sizeof(*(threeDModelMesh.mMeshDatas.mBitangent));
			const UINT64 offset = static_cast<unsigned int>((char*)threeDModelMesh.mMeshDatas.mBitangent - threeDModelMesh.mMeshDatas.mData);

			if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
			{
				GraphicsAPI::EnableVertexAttributeArrayIndex(4);
				GraphicsAPI::DefineVertexAttributeLayout(mVertexDataBuffer, layoutIndex, 3, stride, offset);
			}

			mVertexBufferLayouts[vertexLayoutCount].mOffset = offset;
			mVertexBufferLayouts[vertexLayoutCount].mStride = stride;
			vertexLayoutCount++;
			layoutIndex++;
		}
		
		mVertexBufferLayoutCount = vertexLayoutCount;
		mNumOfVertices = threeDModelMesh.mMeshDatas.mVerticeCount;

		// only fill the index buffer if the index array is non-empty.
		mNumOfIndices = 0;
		if (threeDModelMesh.bHasIndices == true && threeDModelMesh.mMeshIndices.size() > 0)
		{
			mElementBufferObjectID = GraphicsAPI::CreateBufferObject(GraphicsAPI::eBufferTarget::ELEMENT_ARRAY_BUFFER, threeDModelMesh.mMeshIndices.size() * sizeof(UINT32), NULL, false);
			GraphicsAPI::UpdateDataToBuffer(mElementBufferObjectID, GraphicsAPI::eBufferTarget::ELEMENT_ARRAY_BUFFER, 0, threeDModelMesh.mMeshIndices.size() * sizeof(UINT32), reinterpret_cast<const void*>(threeDModelMesh.mMeshIndices.data()));
			mNumOfIndices = threeDModelMesh.mMeshIndices.size();
		}

		mPrimitiveType = threeDModelMesh.mPrimitiveType;

		mAABB3D = threeDModelMesh.mAABB3D;
		mSphere = threeDModelMesh.mSphere;

		mVertexArrayFlag = threeDModelMesh.mVertexArrayFlag;

		D_ASSERT(mPrimitiveType != GraphicsAPI::ePrimitiveType::NONE);

		mTargetThreeDModelMesh = &threeDModelMesh;
	}
}

// The mesh whose buffers are currently bound, or null when that is not known.
//
// Every bind in this file goes through Mesh, so the pointer is enough to say
// whether the geometry is already in place: nothing else in the engine binds a
// vertex buffer. It is cleared at the start of each draw loop, because ImGui
// and the graphics debugger do change the binding behind us between loops.
namespace
{
	const dooms::graphics::Mesh* BOUND_MESH = nullptr;
}

void dooms::graphics::Mesh::ResetBoundMeshCache()
{
	BOUND_MESH = nullptr;
}

void dooms::graphics::Mesh::BindVertexBufferObjectIfNotBound() const
{
	if ((BOUND_MESH == this) && (graphicsSetting::IsSkipRedundantMeshBindEnabled == true))
	{
		return;
	}

	MESH_BIND_COUNT++;
	BOUND_MESH = this;

	if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
	{
		BindVertexArrayObject();
	}
	else if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::D3D11)
	{
		BindVertexBufferObject();
	}
	else
	{
		ASSUME_ZERO;
	}
}

unsigned int dooms::graphics::Mesh::GetAndResetMeshBindCount()
{
	const unsigned int meshBindCount = MESH_BIND_COUNT;
	MESH_BIND_COUNT = 0;
	return meshBindCount;
}

unsigned int dooms::graphics::Mesh::GetAndResetIndexBindCount()
{
	const unsigned int indexBindCount = INDEX_BIND_COUNT;
	INDEX_BIND_COUNT = 0;
	return indexBindCount;
}

unsigned long long dooms::graphics::Mesh::GetAndResetIndexCount()
{
	const unsigned long long indexCount = INDEX_COUNT;
	INDEX_COUNT = 0;
	return indexCount;
}

void dooms::graphics::Mesh::DrawWithLodBuffers(
	const BufferID& vertexBuffer,
	const unsigned int* const layoutOffsets,
	const BufferID& indexBuffer,
	const unsigned long long indexCount) const
{
	D_ASSERT(mPrimitiveType != GraphicsAPI::ePrimitiveType::NONE);

	if (vertexBuffer.IsValid() == false || indexBuffer.IsValid() == false || indexCount == 0)
	{
		Draw();
		return;
	}

	// Every level of a mesh shares this buffer, so consecutive objects drawing
	// the same mesh at different levels do not rebind it.
	if (BOUND_VERTEX_BUFFER_ID[0] != vertexBuffer.GetBufferID())
	{
		// Not the mesh's own buffer, so the cache can no longer vouch for it and
		// the next full detail draw must rebind.
		ResetBoundMeshCache();
		MESH_BIND_COUNT++;

		for (UINT32 bufferLayoutIndex = 0; bufferLayoutIndex < mVertexBufferLayoutCount; bufferLayoutIndex++)
		{
			BOUND_VERTEX_BUFFER_ID[bufferLayoutIndex] = vertexBuffer;

			GraphicsAPI::BindVertexDataBuffer(
				vertexBuffer,
				bufferLayoutIndex,
				mVertexBufferLayouts[bufferLayoutIndex].mStride,
				layoutOffsets[bufferLayoutIndex]);
		}
	}

	if (BOUND_INDEX_BUFFER_ID != indexBuffer.GetBufferID())
	{
		INDEX_BIND_COUNT++;
		BOUND_INDEX_BUFFER_ID = indexBuffer;
		GraphicsAPI::BindBuffer(indexBuffer, 0, GraphicsAPI::eBufferTarget::ELEMENT_ARRAY_BUFFER, GraphicsAPI::eGraphicsPipeLineStage::DUMMY);
	}

	INDEX_COUNT += indexCount;

	GraphicsAPI::DrawIndexed(mPrimitiveType, indexCount);
}

void dooms::graphics::Mesh::DrawWithIndexBuffer(const BufferID& indexBuffer, const unsigned long long indexCount) const
{
	D_ASSERT(mPrimitiveType != GraphicsAPI::ePrimitiveType::NONE);

	if (indexBuffer.IsValid() == false || indexCount == 0)
	{
		Draw();
		return;
	}

	// The vertices are this mesh's, so the vertex binding is this mesh's, and a
	// following draw of the same mesh can still skip it. Only the index buffer
	// differs, and it has a cache of its own.
	BindVertexBufferObjectIfNotBound();

	if (BOUND_INDEX_BUFFER_ID != indexBuffer.GetBufferID())
	{
		INDEX_BIND_COUNT++;
		BOUND_INDEX_BUFFER_ID = indexBuffer;
		GraphicsAPI::BindBuffer(indexBuffer, 0, GraphicsAPI::eBufferTarget::ELEMENT_ARRAY_BUFFER, GraphicsAPI::eGraphicsPipeLineStage::DUMMY);
	}

	INDEX_COUNT += indexCount;

	GraphicsAPI::DrawIndexed(mPrimitiveType, indexCount);
}

void dooms::graphics::Mesh::Draw() const
{
	D_ASSERT(mPrimitiveType != GraphicsAPI::ePrimitiveType::NONE);

	BindVertexBufferObjectIfNotBound();

	if (IsElementBufferGenerated() == true)
	{// TODO : WHY THIS MAKE ERROR ON RADEON GPU, CHECK THIS https://stackoverflow.com/questions/18299646/gldrawelements-emits-gl-invalid-operation-when-using-amd-driver-on-linux
		// you don't need bind EBO everytime, EBO will be bound automatically when bind VAO
		// Self checking, and checked every time: a detail level drawn in
		// between will have left a different index buffer bound while this
		// mesh's vertices were still the ones in place.
		BindIndexBufferObject();

		INDEX_COUNT += mNumOfIndices;
		GraphicsAPI::DrawIndexed(mPrimitiveType, mNumOfIndices);
	}
	else
	{
		GraphicsAPI::Draw(mPrimitiveType, 0, mNumOfVertices);
	}
}

void dooms::graphics::Mesh::DrawArray(const INT32 startVertexLocation, const UINT32 vertexCount) const
{
	D_ASSERT(mPrimitiveType != GraphicsAPI::ePrimitiveType::NONE);

	if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
	{
		BindVertexArrayObject();
	}
	else if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::D3D11)
	{
		BindVertexBufferObject();
	}
	else
	{
		ASSUME_ZERO;
	}

	GraphicsAPI::Draw(mPrimitiveType, vertexCount, startVertexLocation);

	// This bound a vertex buffer without binding an index buffer, so the state
	// is not one a later Draw could reuse.
	ResetBoundMeshCache();
}

void dooms::graphics::Mesh::DrawArray(const GraphicsAPI::ePrimitiveType primitiveType, const INT32 startVertexLocation,	const INT32 vertexCount) const
{
	D_ASSERT(primitiveType != GraphicsAPI::ePrimitiveType::NONE);

	if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
	{
		BindVertexArrayObject();
	}
	else if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::D3D11)
	{
		BindVertexBufferObject();
	}
	else
	{
		ASSUME_ZERO;
	}

	GraphicsAPI::Draw(primitiveType, vertexCount, startVertexLocation);

	ResetBoundMeshCache();
}

constexpr UINT32 dooms::graphics::Mesh::GetStride(const UINT32 vertexArrayFlag)
{
	UINT32 offset = 0;

	if (vertexArrayFlag & eVertexArrayFlag::VertexVector2)
	{
		//mVertex
		offset += sizeof(math::Vector2);
	}

	if (vertexArrayFlag & eVertexArrayFlag::VertexVector3)
	{
		//mVertex
		offset += sizeof(math::Vector3);
	}

	if (vertexArrayFlag & eVertexArrayFlag::TexCoord)
	{
		//mTexCoord
		offset += sizeof(math::Vector3);
	}

	if (vertexArrayFlag & eVertexArrayFlag::mNormal)
	{
		//mNormal
		offset += sizeof(math::Vector3);
	}

	if (vertexArrayFlag & eVertexArrayFlag::mTangent)
	{
		//mTangent
		offset += sizeof(math::Vector3);
	}

	if (vertexArrayFlag & eVertexArrayFlag::mBitangent)
	{
		//mBitangent
		offset += sizeof(math::Vector3);
	}

	if (vertexArrayFlag & eVertexArrayFlag::mTBN)
	{
		//mBitangent
		offset += sizeof(math::Matrix3x3);
	}

	return offset;
}



bool dooms::graphics::Mesh::IsBufferGenerated() const
{
	if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
	{
		return mVertexDataBuffer.IsValid() && mVertexArrayObjectID.IsValid();
	}
	else if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::D3D11)
	{
		return mVertexDataBuffer.IsValid();
	}
	else
	{
		ASSUME_ZERO;
		return false;
	}
}

/*
void dooms::graphics::Mesh::UpdateElementBuffer(const UINT32* indices, const UINT32 indiceCount)
{
	D_ASSERT(IsBufferGenerated() == true);

	if (graphics::GraphicsAPIManager::GetCurrentAPIType() == graphics::GraphicsAPI::eGraphicsAPIType::OpenGL)
	{
		BindVertexArrayObject();
	}
	BindElementBuffer();

	GraphicsAPI::UpdateDataToBuffer(mElementBufferObjectID, GraphicsAPI::eBufferTarget::ELEMENT_ARRAY_BUFFER, 0, indiceCount * sizeof(UINT32), indices);
	mNumOfIndices = indiceCount;
}
*/

const dooms::physics::AABB3D& dooms::graphics::Mesh::GetBoundingBox() const
{
	return mAABB3D;
}

const dooms::physics::Sphere& dooms::graphics::Mesh::GetBoundingSphere() const
{
	return mSphere;
}

const dooms::graphics::BufferID& dooms::graphics::Mesh::GetVertexArrayObjectID() const
{
	return mVertexArrayObjectID;
}

const dooms::graphics::BufferID& dooms::graphics::Mesh::GetElementBufferObjectID() const
{
	return mElementBufferObjectID;
}

void* dooms::graphics::Mesh::MapVertexDataBuffer
(
	const dooms::graphics::GraphicsAPI::eMapBufferAccessOption mapBufferAccessOption
)
{
	void* bufferAddress = nullptr;
	if (mVertexDataBuffer.IsValid() == true)
	{
		bufferAddress = GraphicsAPI::MapBufferObjectToClientAddress(mVertexDataBuffer, GraphicsAPI::eBufferTarget::ARRAY_BUFFER, mapBufferAccessOption);
	}
	D_ASSERT(bufferAddress != nullptr);

	return bufferAddress;
}

void dooms::graphics::Mesh::UnmapVertexDataBuffer()
{
	if(mVertexDataBuffer.IsValid() == true)
	{
		GraphicsAPI::UnMapBufferObjectMappedToClientAddress(mVertexDataBuffer, GraphicsAPI::eBufferTarget::ARRAY_BUFFER);
	}
}

void* dooms::graphics::Mesh::MapElementBuffer
(
	const dooms::graphics::GraphicsAPI::eMapBufferAccessOption mapBufferAccessOption
)
{
	void* bufferAddress = nullptr;
	if (mVertexArrayObjectID.IsValid() == true)
	{
		bufferAddress = GraphicsAPI::MapBufferObjectToClientAddress(mElementBufferObjectID, GraphicsAPI::eBufferTarget::ELEMENT_ARRAY_BUFFER, mapBufferAccessOption);
	}
	D_ASSERT(bufferAddress != nullptr);

	return bufferAddress;
}

void dooms::graphics::Mesh::UnmapElementBuffer()
{
	if (mElementBufferObjectID.IsValid() == true)
	{
		GraphicsAPI::UnMapBufferObjectMappedToClientAddress(mElementBufferObjectID, GraphicsAPI::eBufferTarget::ELEMENT_ARRAY_BUFFER);
	}
}

UINT64 dooms::graphics::Mesh::GetNumOfIndices() const
{
	return mNumOfIndices;
}

UINT64 dooms::graphics::Mesh::GetNumOfVertices() const
{
	return mNumOfVertices;
}





