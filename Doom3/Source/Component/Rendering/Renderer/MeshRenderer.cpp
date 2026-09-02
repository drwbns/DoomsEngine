#include "MeshRenderer.h"

#include <Rendering/Lod/MeshLod.h>
#include <Graphics/graphicsSetting.h>

#include "Asset/ThreeDModelAsset.h"


void dooms::MeshRenderer::InitComponent()
{
	Renderer::InitComponent();
}

void dooms::MeshRenderer::UpdateComponent()
{
	Renderer::UpdateComponent();
}

void dooms::MeshRenderer::OnEndOfFrame_Component()
{
	Renderer::OnEndOfFrame_Component();
}

void dooms::MeshRenderer::OnDestroy()
{
	Base::OnDestroy();

}

void dooms::MeshRenderer::UpdateCullingEntityBlockViewer()
{
	Renderer::UpdateCullingEntityBlockViewer();

	if(IsValid(mTargetMesh) == true)
	{
		const ThreeDModelMesh* const threeDModelMesh = mTargetMesh->GetTargetThreeDModelMesh();

		// TODO : Pass high level LOD mesh to culling system. ( to decrease occluder rasterizing cost )
		mCullingEntityBlockViewer.SetMeshVertexData
		(
			reinterpret_cast<const culling::Vec3*>(threeDModelMesh->mMeshDatas.mVertex),
			threeDModelMesh->mMeshDatas.mVerticeCount,
			threeDModelMesh->mMeshIndices.data(),
			threeDModelMesh->mMeshIndices.size(),
			threeDModelMesh->mVerticeStride
		);
	}
	
}

dooms::graphics::eBatchRenderingType dooms::MeshRenderer::GetCapableBatchRenderingType() const
{
	return dooms::graphics::eBatchRenderingType::StaticMeshBatch;
}

bool dooms::MeshRenderer::IsBatchable() const
{
	bool isBatchable = false;

	if(IsValid(mTargetMaterial) && IsValid(mTargetMesh))
	{
		isBatchable = true;
	}

	return isBatchable;
}

dooms::MeshRenderer::MeshRenderer() : Renderer(), mTargetMesh{ nullptr }
{

}

dooms::MeshRenderer::~MeshRenderer()
{
}

void dooms::MeshRenderer::Draw()
{
	BindMaterial();

	D_ASSERT(mTargetMaterial);
	if (IsValid(mTargetMaterial) && (graphics::graphicsSetting::IsSkipPerDrawUboWriteEnabled == false))
	{
		// Null when the bound shader takes its model matrix per instance
		// instead of from a uniform block, in which case the block is unused,
		// the compiler drops it and the reflection never mentions it. The
		// depth only and batched paths still have one, so this cannot simply
		// go away.
		graphics::UniformBufferObjectView* const modelDataView =
			GetMaterial()->GetUniformBufferObjectViewFromUBOName("ModelData");

		if (modelDataView != nullptr)
		{
			modelDataView->SetMat4x4(graphics::eUniformLocation::ModelMatrix, GetTransform()->GetModelMatrix());
		}
	}
	D_ASSERT(IsValid(mTargetMesh));
	if (IsValid(mTargetMesh))
	{
		// The bounds PreCulling already produced, so choosing a level costs a
		// subtraction and a multiply rather than anything of its own.
		const graphics::MeshLodLevel* lodLevel = nullptr;
		const graphics::MeshLodChain* lodChain = nullptr;

		if (graphics::graphicsSetting::IsMeshLodEnabled && mCullingEntityBlockViewer.IsValid())
		{
			const culling::EntityBlock* const entityBlock = mCullingEntityBlockViewer.GetTargetEntityBlock();
			const UINT32 entityIndex = mCullingEntityBlockViewer.GetEntityIndexInBlock();

			const FLOAT32 screenWidth =
				entityBlock->mAABBMaxScreenSpacePointX[entityIndex] - entityBlock->mAABBMinScreenSpacePointX[entityIndex];
			const FLOAT32 screenHeight =
				entityBlock->mAABBMaxScreenSpacePointY[entityIndex] - entityBlock->mAABBMinScreenSpacePointY[entityIndex];

			const FLOAT32 coveredPixelCount =
				((screenWidth > 0.0f) ? screenWidth : 0.0f) * ((screenHeight > 0.0f) ? screenHeight : 0.0f);

			lodLevel = graphics::SelectMeshLod(mTargetMesh, coveredPixelCount, &lodChain);
		}

		if (lodLevel != nullptr && lodChain != nullptr)
		{
			mTargetMesh->DrawWithLodBuffers(
				lodChain->mSharedVertexBuffer,
				lodChain->mLayoutOffsets,
				lodLevel->mIndexBuffer,
				lodLevel->mIndexCount);
		}
		else
		{
			mTargetMesh->Draw();
		}
	}
}

void dooms::MeshRenderer::SetMesh(const graphics::Mesh* const mesh)
{
	mTargetMesh = mesh;
	if (mTargetMesh != nullptr)
	{
		AddRendererToCullingSystem();
		
		/// <summary>
		/// MeshRenderer is required to UpdateLocalBVhColliderCache only when Mesh is changed
		/// </summary>
		/// <param name="mesh"></param>
		BVH_AABB3D_Node_Object::UpdateLocalColliderCache(mTargetMesh->GetBoundingBox());
		//BVH_Sphere_Node_Object::UpdateBVH_Node();

		//BVH_AABB3D_Node_Object::UpdateLocalBVhColliderCache(mTargetMesh->GetBoundingBox());
		//BVH_AABB3D_Node_Object::UpdateBVH_Node();
		//mIsBoundingSphereDirty.SetDirty(true);

		//SetBoundingSphereRadiusForCulling(boudingSphere.mRadius);
		//SetBoundingSphereRadiusForCulling(0);

		//TODO : when model matrix is changed, should update SetBoundingSphereRadiusForCulling

		UpdateCullingEntityBlockViewer();

		RemoveFromBatchRendering();
		AddToBatchRendering();
	}
	else
	{
		RemoveRendererFromCullingSystem();
		RemoveFromBatchRendering();
	}
	
}

const dooms::graphics::Mesh* dooms::MeshRenderer::GetMesh() const
{
	return mTargetMesh;
}

dooms::physics::AABB3D dooms::MeshRenderer::GetLocalAABBBound() const
{
	if (mTargetMesh != nullptr)
	{
		return mTargetMesh->GetBoundingBox();
	}
	else
	{
		return dooms::physics::AABB3D(nullptr);
	}
}
