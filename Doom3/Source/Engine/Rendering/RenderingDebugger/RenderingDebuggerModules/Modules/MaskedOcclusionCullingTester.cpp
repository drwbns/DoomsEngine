#include "maskedOcclusionCullingTester.h"

#include "DebugDrawer.h"
#include <Rendering/Pipeline/GraphicsPipeLine.h>
#include "Graphics/graphicsSetting.h"
#include <Rendering/Culling/EveryCulling/EveryCullingCore.h>
#include <Rendering/Culling/EveryCulling/CullingModule/MaskedSWOcclusionCulling/MaskedSWOcclusionCulling.h>
#include <Rendering/Pipeline/PipeLines/DefaultGraphcisPipeLine.h>
#include <Rendering/Camera.h>

#include <cmath>

#define DEBUGGER_TILE_BOX_PADIDNG_X 0.002f
#define DEBUGGER_TILE_BOX_PADIDNG_Y 0.002f

namespace
{
	// How many steps the heat ramp is quantised to.
	//
	// The debug drawer batches primitives by colour, so this is very nearly the
	// draw call count for an entire heatmap. It matters more than it looks: the
	// per-tile debuggers cover the screen with tens of thousands of quads, and
	// an unquantised ramp gives almost every one of them a unique colour, which
	// means its own draw call and its own uniform update. Sixteen steps are
	// about as many as can be told apart by eye anyway.
	constexpr UINT32 HEATMAP_STEP_COUNT = 16;

	// Cold to hot, the convention every other engine's complexity view uses.
	constexpr UINT32 HEATMAP_STOP_COUNT = 5;
	const math::Vector4 HEATMAP_STOPS[HEATMAP_STOP_COUNT] =
	{
		math::Vector4{ 0.0f, 0.1f, 0.6f, 1.0f },	// blue
		math::Vector4{ 0.0f, 0.7f, 1.0f, 1.0f },	// cyan
		math::Vector4{ 0.1f, 0.9f, 0.1f, 1.0f },	// green
		math::Vector4{ 1.0f, 0.9f, 0.0f, 1.0f },	// yellow
		math::Vector4{ 1.0f, 0.0f, 0.0f, 1.0f }		// red
	};

	/// <summary>
	/// Maps 0..1 onto the heat ramp above.
	///
	/// The value is snapped to a step before the colour is picked, so tiles that
	/// look the same really are the same colour and batch into one draw call.
	/// </summary>
	math::Vector4 HeatmapColor(const float normalisedValue)
	{
		const float clampedValue = (normalisedValue < 0.0f) ? 0.0f : ((normalisedValue > 1.0f) ? 1.0f : normalisedValue);

		const float stepCount = static_cast<float>(HEATMAP_STEP_COUNT - 1);
		const float quantisedValue = std::floor(clampedValue * stepCount + 0.5f) / stepCount;

		const float rampPosition = quantisedValue * static_cast<float>(HEATMAP_STOP_COUNT - 1);
		const UINT32 lowerStop = static_cast<UINT32>(rampPosition);
		const UINT32 upperStop = (lowerStop + 1 < HEATMAP_STOP_COUNT) ? (lowerStop + 1) : (HEATMAP_STOP_COUNT - 1);
		const float blend = rampPosition - static_cast<float>(lowerStop);

		return math::Vector4
		{
			HEATMAP_STOPS[lowerStop].x + (HEATMAP_STOPS[upperStop].x - HEATMAP_STOPS[lowerStop].x) * blend,
			HEATMAP_STOPS[lowerStop].y + (HEATMAP_STOPS[upperStop].y - HEATMAP_STOPS[lowerStop].y) * blend,
			HEATMAP_STOPS[lowerStop].z + (HEATMAP_STOPS[upperStop].z - HEATMAP_STOPS[lowerStop].z) * blend,
			1.0f
		};
	}
}

void dooms::graphics::MaskedOcclusionCullingTester::DebugTileCoverageMask
(
	const culling::SWDepthBuffer* const depthBuffer
)
{
	std::atomic_thread_fence(std::memory_order_acquire);

	const float xScale = 2.0f / (float)(depthBuffer->mResolution.mColumnSubTileCount);
	const float yScale = 2.0f / (float)(depthBuffer->mResolution.mRowSubTileCount);

	const UINT32 screenWidth = depthBuffer->mResolution.mWidth;
	const UINT32 screenHeight = depthBuffer->mResolution.mHeight;

	//const UINT32 space = ((float)screenWidth / (float)screenHeight) * ((float)GetRowSubTileCount() / (float)GetColumnSubTileCount());

	for (INT32 subTileRowIndex = static_cast<INT32>(depthBuffer->mResolution.mRowSubTileCount) - 1; subTileRowIndex >= 0; subTileRowIndex--)
	{
		for (INT32 subTileColIndex = 0; subTileColIndex < static_cast<INT32>(depthBuffer->mResolution.mColumnSubTileCount) ; subTileColIndex++)
		{
			const INT32 tileRowIndex = subTileRowIndex / (EVERYCULLING_TILE_HEIGHT / EVERYCULLING_SUB_TILE_HEIGHT);
			const INT32 tileColIndex = subTileColIndex / (EVERYCULLING_TILE_WIDTH / EVERYCULLING_SUB_TILE_WIDTH);

			const INT32 subTileRowIndexInTile = subTileRowIndex % (EVERYCULLING_TILE_HEIGHT / EVERYCULLING_SUB_TILE_HEIGHT);
			const INT32 subTileColIndexInTile = subTileColIndex % (EVERYCULLING_TILE_WIDTH / EVERYCULLING_SUB_TILE_WIDTH);

			D_ASSERT(subTileRowIndexInTile >= 0 && subTileRowIndexInTile < (EVERYCULLING_TILE_HEIGHT / EVERYCULLING_SUB_TILE_HEIGHT));
			D_ASSERT(subTileColIndexInTile >= 0 && subTileRowIndexInTile < (EVERYCULLING_TILE_WIDTH / EVERYCULLING_SUB_TILE_WIDTH));

			const culling::EVERYCULLING_M256I L1CoverageMask = depthBuffer->GetTile(tileRowIndex, tileColIndex)->mHizDatas.L1CoverageMask;
			const INT32 subTileIndex = subTileColIndexInTile + subTileRowIndexInTile * (EVERYCULLING_TILE_WIDTH / EVERYCULLING_SUB_TILE_WIDTH);
			D_ASSERT(subTileIndex >= 0 && subTileIndex < 8);

			const int coverageMaskValue = reinterpret_cast<const int*>(&L1CoverageMask)[subTileIndex];

			dooms::graphics::DebugDrawer::GetSingleton()->DebugDraw2DBox
			(
				math::Vector3(DEBUGGER_TILE_BOX_PADIDNG_X + -1.0f + xScale * subTileColIndex, DEBUGGER_TILE_BOX_PADIDNG_Y + -1.0f + yScale * subTileRowIndex, 0.0f),
				math::Vector3(-DEBUGGER_TILE_BOX_PADIDNG_X + -1.0f + xScale * (subTileColIndex + 1), -DEBUGGER_TILE_BOX_PADIDNG_Y + -1.0f + yScale * (subTileRowIndex + 1), 0.0f),
				(coverageMaskValue == (int)0xFFFFFFFF) ? eColor::Red : eColor::White
			);
		}
	}
	
}

void dooms::graphics::MaskedOcclusionCullingTester::DebugTileL0MaxDepthValue
(
	const culling::SWDepthBuffer* const depthBuffer
)
{
	std::atomic_thread_fence(std::memory_order_acquire);

	const float xScale = 2.0f / (float)(depthBuffer->mResolution.mColumnSubTileCount);
	const float yScale = 2.0f / (float)(depthBuffer->mResolution.mRowSubTileCount);

	const UINT32 screenWidth = depthBuffer->mResolution.mWidth;
	const UINT32 screenHeight = depthBuffer->mResolution.mHeight;

	//const UINT32 space = ((float)screenWidth / (float)screenHeight) * ((float)GetRowSubTileCount() / (float)GetColumnSubTileCount());

	for (INT32 subTileRowIndex = depthBuffer->mResolution.mRowSubTileCount - 1; subTileRowIndex >= 0; subTileRowIndex--)
	{
		for (INT32 subTileColIndex = 0; subTileColIndex < depthBuffer->mResolution.mColumnSubTileCount ; subTileColIndex++)
		{
			const INT32 tileRowIndex = subTileRowIndex / (EVERYCULLING_TILE_HEIGHT / EVERYCULLING_SUB_TILE_HEIGHT);
			const INT32 tileColIndex = subTileColIndex / (EVERYCULLING_TILE_WIDTH / EVERYCULLING_SUB_TILE_WIDTH);

			const INT32 subTileRowIndexInTile = subTileRowIndex % (EVERYCULLING_TILE_HEIGHT / EVERYCULLING_SUB_TILE_HEIGHT);
			const INT32 subTileColIndexInTile = subTileColIndex % (EVERYCULLING_TILE_WIDTH / EVERYCULLING_SUB_TILE_WIDTH);

			D_ASSERT(subTileRowIndexInTile >= 0 && subTileRowIndexInTile < (EVERYCULLING_TILE_HEIGHT / EVERYCULLING_SUB_TILE_HEIGHT));
			D_ASSERT(subTileColIndexInTile >= 0 && subTileRowIndexInTile < (EVERYCULLING_TILE_WIDTH / EVERYCULLING_SUB_TILE_WIDTH));

			const culling::EVERYCULLING_M256F L0MaxDepthValue = depthBuffer->GetTile(tileRowIndex, tileColIndex)->mHizDatas.L0SubTileMaxDepthValue;
			const INT32 subTileIndex = subTileColIndexInTile + subTileRowIndexInTile * (EVERYCULLING_TILE_WIDTH / EVERYCULLING_SUB_TILE_WIDTH);
			D_ASSERT(subTileIndex >= 0 && subTileIndex < 8);

			float ndcDepthValue = reinterpret_cast<const float*>(&L0MaxDepthValue)[subTileIndex];
			
			ndcDepthValue = (ndcDepthValue + 1.0f) * 0.5f;
			// Cubed to spread out the near end, where perspective crowds almost
			// all of the interesting depth range into a narrow band of values.
			ndcDepthValue = ndcDepthValue * ndcDepthValue * ndcDepthValue;
			//const float linearDepth = (2.0 * Camera::GetMainCamera()->GetClippingPlaneNear() * Camera::GetMainCamera()->GetClippingPlaneFar()) / (Camera::GetMainCamera()->GetClippingPlaneFar() + Camera::GetMainCamera()->GetClippingPlaneNear() - ndcDepthValue * (Camera::GetMainCamera()->GetClippingPlaneFar() - Camera::GetMainCamera()->GetClippingPlaneNear()));

			//const float nonLinearDepthValue = ((1.0f / linearDepth) - (1.0f / Camera::GetMainCamera()->GetClippingPlaneNear())) / ((1.0f / Camera::GetMainCamera()->GetClippingPlaneFar()) - (1.0f / Camera::GetMainCamera()->GetClippingPlaneNear()));

			

			dooms::graphics::DebugDrawer::GetSingleton()->DebugDraw2DBox
			(
				math::Vector3(DEBUGGER_TILE_BOX_PADIDNG_X + -1.0f + xScale * subTileColIndex, DEBUGGER_TILE_BOX_PADIDNG_Y + -1.0f + yScale * subTileRowIndex, 0.0f),
				math::Vector3(-DEBUGGER_TILE_BOX_PADIDNG_X + -1.0f + xScale * (subTileColIndex + 1), -DEBUGGER_TILE_BOX_PADIDNG_Y + -1.0f + yScale * (subTileRowIndex + 1), 0.0f),
				// Inverted so that near reads as hot: the near occluders are the
				// ones doing the culling work worth looking at.
				HeatmapColor(1.0f - ndcDepthValue)
			);
		}
	}
}

void dooms::graphics::MaskedOcclusionCullingTester::DebugOccluderBoundingBox(const culling::OccluderListManager* const OccluderListManager, const unsigned long long InTickCount)
{
	if(EVERYCULLING_WHEN_TO_BIN_TRIANGLE(InTickCount))
	{
		CachedOccluderAABBList.clear();

		const math::Vector3 CameraPos = Camera::GetMainCamera()->GetTransform()->GetPosition();
		for (const culling::OccluderData& OccluderData : OccluderListManager->GetSortedOccluderList(culling::Vec3{ CameraPos.x, CameraPos.y, CameraPos.z }))
		{
			const float MinX = OccluderData.mEntityBlock->mAABBMinWorldPoint[OccluderData.mEntityIndexInEntityBlock].values[0];
			const float MinY = OccluderData.mEntityBlock->mAABBMinWorldPoint[OccluderData.mEntityIndexInEntityBlock].values[1];
			const float MinZ = OccluderData.mEntityBlock->mAABBMinWorldPoint[OccluderData.mEntityIndexInEntityBlock].values[2];

			const float MaxX = OccluderData.mEntityBlock->mAABBMaxWorldPoint[OccluderData.mEntityIndexInEntityBlock].values[0];
			const float MaxY = OccluderData.mEntityBlock->mAABBMaxWorldPoint[OccluderData.mEntityIndexInEntityBlock].values[1];
			const float MaxZ = OccluderData.mEntityBlock->mAABBMaxWorldPoint[OccluderData.mEntityIndexInEntityBlock].values[2];

			CachedOccluderAABBList.emplace_back(math::Vector3{ MinX, MinY, MinZ }, math::Vector3{ MaxX, MaxY, MaxZ });
		}
	}

	for(const dooms::physics::AABB3D& CachedOccluderAABB : CachedOccluderAABBList)
	{
		dooms::graphics::DebugDrawer::GetSingleton()->DebugDraw3DBox(static_cast<math::Vector3>(CachedOccluderAABB.mLowerBound), 
			static_cast<math::Vector3>(CachedOccluderAABB.mUpperBound), eColor::Red);
	}
}

void dooms::graphics::MaskedOcclusionCullingTester::Initialize()
{
}

void dooms::graphics::MaskedOcclusionCullingTester::PreRender()
{
}

void dooms::graphics::MaskedOcclusionCullingTester::Render(dooms::Camera* const targetCamera)
{
	graphics::DefaultGraphcisPipeLine* defaultGraphicsPipeLine = CastTo<graphics::DefaultGraphcisPipeLine*>(dooms::graphics::GraphicsPipeLine::GetSingleton());
	D_ASSERT(IsValid(defaultGraphicsPipeLine));
	if (IsValid(defaultGraphicsPipeLine))
	{
		if (graphicsSetting::IsDrawMaskedOcclusionCullingBinTriangleStageDebugger == true)
		{
			DebugBinnedTriangles(&(defaultGraphicsPipeLine->mRenderingCullingManager.mCullingSystem->mMaskedSWOcclusionCulling->mDepthBuffer));
		}

		if (graphicsSetting::IsDrawMaskedOcclusionCullingTileCoverageMaskDebugger == true)
		{
			DebugTileCoverageMask(&(defaultGraphicsPipeLine->mRenderingCullingManager.mCullingSystem->mMaskedSWOcclusionCulling->mDepthBuffer));
		}

		if (graphicsSetting::IsDrawMaskedOcclusionCullingTileL0MaxDepthValueDebugger == true)
		{
			DebugTileL0MaxDepthValue(&(defaultGraphicsPipeLine->mRenderingCullingManager.mCullingSystem->mMaskedSWOcclusionCulling->mDepthBuffer));
		}

		if (graphicsSetting::IsDrawMaskedOcclusionCullingOcculderBoundingBoxDebugger == true)
		{
			DebugOccluderBoundingBox(&(defaultGraphicsPipeLine->mRenderingCullingManager.mCullingSystem->mMaskedSWOcclusionCulling->mOccluderListManager), defaultGraphicsPipeLine->mRenderingCullingManager.mCullingSystem->GetTickCount());
		}
	}
}

void dooms::graphics::MaskedOcclusionCullingTester::PostRender()
{
}

const char* dooms::graphics::MaskedOcclusionCullingTester::GetRenderingDebuggerModuleName()
{
	return "MaskedOcclusionCullingTester";
}

void dooms::graphics::MaskedOcclusionCullingTester::DebugBinnedTriangles
(
	const culling::SWDepthBuffer* const depthBuffer
)
{
	std::atomic_thread_fence(std::memory_order_acquire);

	const float xScale = 2.0f / (float)(depthBuffer->mResolution.mColumnTileCount);
	const float yScale = 2.0f / (float)(depthBuffer->mResolution.mRowTileCount);

	for (std::uint32_t y = 0; y < depthBuffer->mResolution.mRowTileCount; y++)
	{
		// y = 0 -> bottom
		for (std::uint32_t x = 0; x < depthBuffer->mResolution.mColumnTileCount; x++)
		{
			const size_t binnedTriangleCount = depthBuffer->GetTile(y, x)->mBinnedTriangleCount;

			// Tiles nothing was binned into are skipped rather than filled in.
			//
			// They used to be drawn black, which is opaque, so the debugger
			// blacked out the whole frame apart from thin gaps between the tile
			// boxes and you could not tell what the heat was sitting on. Leaving
			// them out puts the heat over the scene instead, which is the point.
			if (binnedTriangleCount == 0)
			{
				continue;
			}

			const math::Vector4 tileColor = HeatmapColor((float)binnedTriangleCount / (float)BIN_TRIANGLE_CAPACITY_PER_TILE);

			//draw -1 ~ 1
			dooms::graphics::DebugDrawer::GetSingleton()->DebugDraw2DBox
			(
				math::Vector3(DEBUGGER_TILE_BOX_PADIDNG_X + -1.0f + xScale * x, DEBUGGER_TILE_BOX_PADIDNG_Y + -1.0f + yScale * y, 0.0f),
				math::Vector3(-DEBUGGER_TILE_BOX_PADIDNG_X + -1.0f + xScale * (x + 1), -DEBUGGER_TILE_BOX_PADIDNG_Y + -1.0f + yScale * (y + 1), 0.0f),
				tileColor
			);
		}
	}

}
