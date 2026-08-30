#include "DeferredRenderingPipeLine.h"

#include <atomic>

#include <Rendering/Graphics_Server.h>
#include "Graphics/GraphicsAPI/graphicsAPISetting.h"
#include <Rendering/Camera.h>
#include <Rendering/Renderer/Renderer.h>
#include "DeferredRenderingPipeLineCamera.h"
#include <Graphics/graphicsSetting.h>
#include <Rendering/RenderingDebugger/RenderingDebuggerModules/Modules/OverDrawVisualization.h>
#include <PictureInPicture/PicktureInPickture.h>
#include <Rendering/Material/Material.h>
#include <Rendering/Texture/TextureView.h>
#include <Asset/AssetManager/AssetManager.h>
#include <Asset/ShaderAsset.h>
#include <Asset/TextureAsset.h>
#include <Rendering/Buffer/MeshHelper.h>
#include <Rendering/Buffer/UniformBufferObject/UniformBufferObjectView.h>
#include <Rendering/Culling/EveryCulling/DataType/EntityBlock.h>
#include <Rendering/Culling/EveryCulling/CullingModule/MaskedSWOcclusionCulling/MaskedSWOcclusionCulling.h>
#include <cstring>
#include <chrono>
#include <array>
#include <Misc/AccelerationContainer/BVH/BVH.h>
#include <Rendering/Renderer/RendererStaticIterator.h>

dooms::graphics::DeferredRenderingPipeLine::DeferredRenderingPipeLine
(
	dooms::graphics::Graphics_Server& graphicsServer
)
	:
	dooms::graphics::DefaultGraphcisPipeLine(graphicsServer),
	mDeferredRenderingDrawer()
{
}

void dooms::graphics::DeferredRenderingPipeLine::Initialize()
{
	DefaultGraphcisPipeLine::Initialize();

	mDeferredRenderingDrawer.Initialize();

}

void dooms::graphics::DeferredRenderingPipeLine::LateInitialize()
{
	DefaultGraphcisPipeLine::LateInitialize();
}

void dooms::graphics::DeferredRenderingPipeLine::PreRender()
{
	DefaultGraphcisPipeLine::PreRender();

	
}

void dooms::graphics::DeferredRenderingPipeLine::Render()
{
	DefaultGraphcisPipeLine::Render();

	
	
	

}

void dooms::graphics::DeferredRenderingPipeLine::PostRender()
{
	DefaultGraphcisPipeLine::PostRender();

}

dooms::graphics::eGraphicsPipeLineType dooms::graphics::DeferredRenderingPipeLine::GetGraphicsPipeLineType() const
{
	return eGraphicsPipeLineType::DeferredRendering;
}

dooms::graphics::GraphicsPipeLineCamera* dooms::graphics::DeferredRenderingPipeLine::CreateGraphicsPipeLineCamera() const
{
	return dooms::CreateDObject<DeferredRenderingPipeLineCamera>();
}



void dooms::graphics::DeferredRenderingPipeLine::DrawHiZQuad()
{
	if (mHiZQuadMesh == nullptr)
	{
		mHiZQuadMesh = meshHelper::GetQuadMesh(
			math::Vector2(-1.0f, -1.0f),
			math::Vector2(1.0f, 1.0f),
			meshHelper::GetFlipOptionBasedOnCurrentGraphicsAPI());
	}

	if (IsValid(mHiZQuadMesh))
	{
		mHiZQuadMesh->Draw();
	}
}

void dooms::graphics::DeferredRenderingPipeLine::BeginHiZGpuTimer()
{
	if (GraphicsAPI::CreateQuery == nullptr || GraphicsAPI::BeginQuery == nullptr ||
		GraphicsAPI::EndQuery == nullptr || GraphicsAPI::GetQueryResult == nullptr)
	{
		return;
	}

	if (bmAreGpuTimersCreated == false)
	{
		for (UINT32 frameIndex = 0; frameIndex < GPU_TIMER_FRAME_COUNT; frameIndex++)
		{
			mHiZGpuTimers[frameIndex].mDisjointQuery = GraphicsAPI::CreateQuery(GraphicsAPI::QUERY_TIMESTAMP_DISJOINT);
			mHiZGpuTimers[frameIndex].mStartQuery = GraphicsAPI::CreateQuery(GraphicsAPI::QUERY_TIMESTAMP);
			mHiZGpuTimers[frameIndex].mEndQuery = GraphicsAPI::CreateQuery(GraphicsAPI::QUERY_TIMESTAMP);
		}

		bmAreGpuTimersCreated = true;
	}

	// Collect the oldest one first. Going round the ring means this is the
	// frame furthest from the one about to be issued, so it has had the most
	// time to finish.
	GpuTimerFrame& oldestTimer = mHiZGpuTimers[(mHiZGpuTimerIndex + 1) % GPU_TIMER_FRAME_COUNT];

	if (oldestTimer.bmIsPending && oldestTimer.mDisjointQuery != 0)
	{
		unsigned long long frequency = 0;
		unsigned int bIsDisjoint = 0;

		if (GraphicsAPI::GetQueryResult(oldestTimer.mDisjointQuery, GraphicsAPI::QUERY_TIMESTAMP_DISJOINT, &frequency, &bIsDisjoint) != 0)
		{
			unsigned long long startTicks = 0;
			unsigned long long endTicks = 0;

			const bool bHasBothTimestamps =
				(GraphicsAPI::GetQueryResult(oldestTimer.mStartQuery, GraphicsAPI::QUERY_TIMESTAMP, &startTicks, nullptr) != 0) &&
				(GraphicsAPI::GetQueryResult(oldestTimer.mEndQuery, GraphicsAPI::QUERY_TIMESTAMP, &endTicks, nullptr) != 0);

			// Disjoint means the gpu clock was interrupted during the span, so
			// the tick counts either side of it cannot be compared. The only
			// correct thing to do is drop the measurement.
			if (bHasBothTimestamps && (bIsDisjoint == 0) && (frequency > 0) && (endTicks >= startTicks))
			{
				graphicsSetting::GpuStatHiZBuildMilliseconds =
					static_cast<FLOAT32>(static_cast<double>(endTicks - startTicks) * 1000.0 / static_cast<double>(frequency));
			}

			oldestTimer.bmIsPending = false;
		}
	}

	GpuTimerFrame& currentTimer = mHiZGpuTimers[mHiZGpuTimerIndex];

	if (currentTimer.mDisjointQuery != 0 && currentTimer.bmIsPending == false)
	{
		GraphicsAPI::BeginQuery(currentTimer.mDisjointQuery);
		GraphicsAPI::EndQuery(currentTimer.mStartQuery);
	}
}

void dooms::graphics::DeferredRenderingPipeLine::EndHiZGpuTimer()
{
	if (GraphicsAPI::EndQuery == nullptr || bmAreGpuTimersCreated == false)
	{
		return;
	}

	GpuTimerFrame& currentTimer = mHiZGpuTimers[mHiZGpuTimerIndex];

	if (currentTimer.mDisjointQuery != 0 && currentTimer.bmIsPending == false)
	{
		GraphicsAPI::EndQuery(currentTimer.mEndQuery);
		GraphicsAPI::EndQuery(currentTimer.mDisjointQuery);

		currentTimer.bmIsPending = true;
	}

	mHiZGpuTimerIndex = (mHiZGpuTimerIndex + 1) % GPU_TIMER_FRAME_COUNT;
}

void dooms::graphics::DeferredRenderingPipeLine::BuildHiZPyramid(dooms::Camera* const targetCamera)
{
	dooms::graphics::DeferredRenderingPipeLineCamera* const deferredRenderingPipeLineCamera
		= CastTo<graphics::DeferredRenderingPipeLineCamera*>(targetCamera->GetGraphicsPipeLineCamera());

	if (IsValid(deferredRenderingPipeLineCamera) == false)
	{
		return;
	}

	DefferedRenderingFrameBuffer& gBuffer = deferredRenderingPipeLineCamera->mDeferredRenderingFrameBuffer;

	if (IsValid(mHiZTexture) == false)
	{
		const UINT32 width = gBuffer.GetDefaultWidth();
		const UINT32 height = gBuffer.GetDefaultHeight();

		if (width == 0 || height == 0)
		{
			return;
		}

		// Levels down to a single texel, so the coarsest one summarises the
		// whole screen and a test against a large object need only read it.
		mHiZLevelCount = 1;
		for (UINT32 size = (width > height) ? width : height; size > 1; size >>= 1)
		{
			mHiZLevelCount++;
		}

		mHiZTexture = dooms::CreateDObject<dooms::asset::TextureAsset>
		(
			GraphicsAPI::eTargetTexture::TARGET_TEXTURE_TEXTURE_2D,
			GraphicsAPI::eTextureInternalFormat::TEXTURE_INTERNAL_FORMAT_R32F,
			GraphicsAPI::eTextureCompressedInternalFormat::TEXTURE_COMPRESSED_INTERNAL_FORMAT_NONE,
			width,
			height,
			GraphicsAPI::eTextureComponentFormat::TEXTURE_COMPONENT_RED,
			GraphicsAPI::eDataType::FLOAT,
			(GraphicsAPI::eBindFlag)(GraphicsAPI::eBindFlag::BIND_RENDER_TARGET | GraphicsAPI::eBindFlag::BIND_SHADER_RESOURCE),
			GraphicsAPI::eTextureBindTarget::TEXTURE_2D,
			nullptr,
			0,
			mHiZLevelCount
		);
		mHiZTexture->AddToRootObjectList();

		mHiZFrameBuffers.reserve(mHiZLevelCount);
		for (UINT32 levelIndex = 0; levelIndex < mHiZLevelCount; levelIndex++)
		{
			const UINT32 levelWidth = mHiZTexture->GetTextureWidth(levelIndex);
			const UINT32 levelHeight = mHiZTexture->GetTextureHeight(levelIndex);

			// Sized, because binding a frame buffer takes the viewport from
			// these and a zero one silently discards every draw.
			FrameBuffer* const levelFrameBuffer = dooms::CreateDObject<FrameBuffer>(levelWidth, levelHeight);
			levelFrameBuffer->AttachExistingColorTextureToFrameBuffer(0, mHiZTexture, levelIndex);
			levelFrameBuffer->AddToRootObjectList();

			mHiZFrameBuffers.push_back(levelFrameBuffer);
		}

		dooms::asset::ShaderAsset* const copyShader
			= dooms::assetImporter::AssetManager::GetSingleton()->GetAsset<dooms::asset::eAssetType::SHADER>("HiZCopyShader.glsl");
		dooms::asset::ShaderAsset* const downsampleShader
			= dooms::assetImporter::AssetManager::GetSingleton()->GetAsset<dooms::asset::eAssetType::SHADER>("HiZDownsampleShader.glsl");

		D_ASSERT(IsValid(copyShader) && IsValid(downsampleShader));
		if (IsValid(copyShader) == false || IsValid(downsampleShader) == false)
		{
			return;
		}

		mHiZCopyMaterial = copyShader->CreateMatrialWithThisShaderAsset();
		mHiZCopyMaterial->AddToRootObjectList();

		mHiZDownsampleMaterial = downsampleShader->CreateMatrialWithThisShaderAsset();
		mHiZDownsampleMaterial->AddToRootObjectList();

		D_RELEASE_LOG(eLogType::D_LOG, "HiZ : pyramid built, %u x %u, %u levels", width, height, mHiZLevelCount);
	}

	if (IsValid(mHiZCopyMaterial) == false || IsValid(mHiZDownsampleMaterial) == false)
	{
		return;
	}

	BeginHiZGpuTimer();

	// The pyramid is written, never blended or depth tested.
	GraphicsAPI::SetIsBlendEnabled(false);
	GraphicsAPI::SetIsDepthTestEnabled(false);
	GraphicsAPI::SetDepthMask(false);

	TextureView* const depthTextureView
		= gBuffer.GetDepthTextureView(0, GraphicsAPI::eGraphicsPipeLineStage::PIXEL_SHADER);

	if (IsValid(depthTextureView))
	{
		mHiZFrameBuffers[0]->BindFrameBuffer();
		mHiZCopyMaterial->BindMaterial();
		depthTextureView->BindTexture(0, GraphicsAPI::eGraphicsPipeLineStage::PIXEL_SHADER);

		DrawHiZQuad();

		depthTextureView->UnBindTexture(0, GraphicsAPI::eGraphicsPipeLineStage::PIXEL_SHADER);
	}

	// One view per source level, built once and kept.
	//
	// A view spanning the whole chain cannot be used here: it would include the
	// level being rendered into, and D3D11 answers that by unbinding the view
	// rather than failing the call, which showed up as every level below the
	// top reading as zero. Each view covers exactly the level being read.
	if (mHiZSourceViews.empty())
	{
		mHiZSourceViews.reserve(mHiZLevelCount);

		for (UINT32 levelIndex = 0; levelIndex < mHiZLevelCount; levelIndex++)
		{
			TextureView* const levelView = dooms::CreateDObject<TextureView>(
				mHiZTexture, 0, GraphicsAPI::eGraphicsPipeLineStage::PIXEL_SHADER, levelIndex, 1);
			levelView->AddToRootObjectList();

			mHiZSourceViews.push_back(levelView);
		}
	}

	UniformBufferObjectView* const hiZDataView = mHiZDownsampleMaterial->GetUniformBufferObjectViewFromUBOName("HiZData");

	for (UINT32 levelIndex = 1; levelIndex < mHiZLevelCount; levelIndex++)
	{
		const UINT32 sourceLevel = levelIndex - 1;

		TextureView* const sourceView = mHiZSourceViews[sourceLevel];
		if (IsValid(sourceView) == false)
		{
			continue;
		}

		mHiZFrameBuffers[levelIndex]->BindFrameBuffer();
		mHiZDownsampleMaterial->BindMaterial();

		if (hiZDataView != nullptr)
		{
			// Texel size of the level being read. The level index is not passed
			// because the view covers one level, so to a sampler it is level
			// zero whichever level of the chain it actually is.
			hiZDataView->SetVector4((UINT64)0, math::Vector4(
				0.0f,
				1.0f / static_cast<FLOAT32>(mHiZTexture->GetTextureWidth(sourceLevel)),
				1.0f / static_cast<FLOAT32>(mHiZTexture->GetTextureHeight(sourceLevel)),
				0.0f));
		}

		sourceView->BindTexture(0, GraphicsAPI::eGraphicsPipeLineStage::PIXEL_SHADER);

		DrawHiZQuad();

		sourceView->UnBindTexture(0, GraphicsAPI::eGraphicsPipeLineStage::PIXEL_SHADER);
	}

	GraphicsAPI::SetIsDepthTestEnabled(true);
	GraphicsAPI::SetDepthMask(true);
	FrameBuffer::StaticBindBackFrameBuffer();

	EndHiZGpuTimer();

	ReadBackHiZLevel();
}

void dooms::graphics::DeferredRenderingPipeLine::ReadBackHiZLevel()
{
	if (GraphicsAPI::CreateStagingTexture2D == nullptr ||
		GraphicsAPI::CopyTexture2DToStagingTexture == nullptr ||
		GraphicsAPI::MapTexture2DForRead == nullptr ||
		GraphicsAPI::UnMapTexture2D == nullptr)
	{
		return;
	}

	if (IsValid(mHiZTexture) == false)
	{
		return;
	}

	if (mHiZReadbackTexture == 0)
	{
		// A coarse level, so the copy is small and the map is cheap. Sixteen
		// texels across is enough to see that real values are coming through
		// while staying far away from the cost of reading back a whole screen.
		// Aimed at roughly sixty texels across: fine enough that an object's
		// screen rectangle covers a useful number of cells, coarse enough that
		// the copy and the scan over it stay cheap.
		mHiZReadbackLevel = 0;
		while ((mHiZReadbackLevel + 1 < mHiZLevelCount) && (mHiZTexture->GetTextureWidth(mHiZReadbackLevel) > 64))
		{
			mHiZReadbackLevel++;
		}

		mHiZReadbackWidth = mHiZTexture->GetTextureWidth(mHiZReadbackLevel);
		mHiZReadbackHeight = mHiZTexture->GetTextureHeight(mHiZReadbackLevel);

		mHiZReadbackTexture = GraphicsAPI::CreateStagingTexture2D(
			mHiZReadbackWidth,
			mHiZReadbackHeight,
			GraphicsAPI::eTextureInternalFormat::TEXTURE_INTERNAL_FORMAT_R32F);

		if (mHiZReadbackTexture == 0)
		{
			return;
		}
	}

	// Last frame's copy, if the gpu has finished with it. Never waited on: a
	// frame or two of staleness is the price of not stalling.
	if (bmIsHiZReadbackPending)
	{
		unsigned int rowPitchInBytes = 0;
		const void* const mappedData = GraphicsAPI::MapTexture2DForRead(mHiZReadbackTexture, 0u, &rowPitchInBytes);

		if (mappedData != nullptr)
		{
			FLOAT32 nearestDepth = 1.0f;
			FLOAT32 farthestDepth = 0.0f;

			for (UINT32 rowIndex = 0; rowIndex < mHiZReadbackHeight; rowIndex++)
			{
				const FLOAT32* const row = reinterpret_cast<const FLOAT32*>(
					reinterpret_cast<const char*>(mappedData) + static_cast<size_t>(rowIndex) * rowPitchInBytes);

				for (UINT32 columnIndex = 0; columnIndex < mHiZReadbackWidth; columnIndex++)
				{
					const FLOAT32 storedDepth = row[columnIndex];

					nearestDepth = (storedDepth < nearestDepth) ? storedDepth : nearestDepth;
					farthestDepth = (storedDepth > farthestDepth) ? storedDepth : farthestDepth;
				}
			}

			// Kept, so the occlusion test can read it without mapping again.
			mHiZReadbackData.resize(static_cast<size_t>(mHiZReadbackWidth) * mHiZReadbackHeight);

			for (UINT32 rowIndex = 0; rowIndex < mHiZReadbackHeight; rowIndex++)
			{
				const FLOAT32* const row = reinterpret_cast<const FLOAT32*>(
					reinterpret_cast<const char*>(mappedData) + static_cast<size_t>(rowIndex) * rowPitchInBytes);

				std::memcpy(
					mHiZReadbackData.data() + static_cast<size_t>(rowIndex) * mHiZReadbackWidth,
					row,
					static_cast<size_t>(mHiZReadbackWidth) * sizeof(FLOAT32));
			}

			bmIsHiZReadbackDataValid = true;

			GraphicsAPI::UnMapTexture2D(mHiZReadbackTexture);
			bmIsHiZReadbackPending = false;

			// Throttled, because this runs every frame once it is working.
			if ((mHiZFrameCounter % 300) == 0)
			{
				D_RELEASE_LOG(eLogType::D_LOG, "HiZ readback : level %u, %u x %u, nearest %f, farthest %f",
					mHiZReadbackLevel, mHiZReadbackWidth, mHiZReadbackHeight, nearestDepth, farthestDepth);
			}
		}
	}

	if (bmIsHiZReadbackPending == false)
	{
		GraphicsAPI::CopyTexture2DToStagingTexture(
			mHiZReadbackTexture,
			mHiZTexture->GetTextureResourceObject(),
			mHiZReadbackLevel);

		bmIsHiZReadbackPending = true;
	}

	mHiZFrameCounter++;
}

void dooms::graphics::DeferredRenderingPipeLine::ApplyHiZOcclusionCulling(const size_t cameraIndex)
{
	if (bmIsHiZReadbackDataValid == false || mHiZReadbackData.empty())
	{
		return;
	}

	culling::EveryCulling* const cullingSystem = mRenderingCullingManager.mCullingSystem.get();
	if (cullingSystem == nullptr)
	{
		return;
	}

	// The screen space bounds PreCulling writes are in the culling system's
	// resolution, which is the window. The pyramid is the g-buffer, which is
	// not. Everything is taken to normalised coordinates rather than assuming
	// the two agree.
	const culling::SWDepthBuffer& swDepthBuffer = cullingSystem->mMaskedSWOcclusionCulling->mDepthBuffer;
	const FLOAT32 cullingWidth = static_cast<FLOAT32>(swDepthBuffer.mResolution.mWidth);
	const FLOAT32 cullingHeight = static_cast<FLOAT32>(swDepthBuffer.mResolution.mHeight);

	if (cullingWidth <= 0.0f || cullingHeight <= 0.0f)
	{
		return;
	}

	const std::chrono::steady_clock::time_point testStartTime = std::chrono::steady_clock::now();

	UINT32 testedCount = 0;
	UINT32 culledAsRawCount = 0;

	FLOAT32 observedMinNDCZ = 1000.0f;
	FLOAT32 observedMaxNDCZ = -1000.0f;

	for (culling::EntityBlock* const entityBlock : cullingSystem->GetActiveEntityBlockList())
	{
		if (entityBlock == nullptr)
		{
			continue;
		}

		for (size_t entityIndex = 0; entityIndex < entityBlock->mCurrentEntityCount; entityIndex++)
		{
			if (entityBlock->GetIsObjectEnabled(entityIndex) == false)
			{
				continue;
			}

			// Behind the camera, so the screen bounds mean nothing.
			if (entityBlock->mIsAllAABBClipPointWPositive[entityIndex] == false)
			{
				continue;
			}

			// Already gone, by the frustum or by distance. Testing it again
			// cannot change the outcome, and this skips most of the scene:
			// frustum culling alone accounts for around forty percent of it.
			if (entityBlock->GetIsCulled(entityIndex, cameraIndex) == true)
			{
				continue;
			}

			const FLOAT32 objectNDCZ = entityBlock->mAABBMinNDCZ[entityIndex];

			observedMinNDCZ = (objectNDCZ < observedMinNDCZ) ? objectNDCZ : observedMinNDCZ;
			observedMaxNDCZ = (objectNDCZ > observedMaxNDCZ) ? objectNDCZ : observedMaxNDCZ;

			const FLOAT32 minU = entityBlock->mAABBMinScreenSpacePointX[entityIndex] / cullingWidth;
			const FLOAT32 maxU = entityBlock->mAABBMaxScreenSpacePointX[entityIndex] / cullingWidth;
			// Flipped, and the ends swapped with it.
			//
			// PreCulling produces screen Y as (ndc y + 1) * half height, and ndc
			// y is positive upwards, so its origin is the bottom of the screen.
			// The pyramid is a render target, whose first row is the top. Without
			// this every object is tested against the mirror image of where it
			// actually is, which culls things in empty sky because the ground
			// below them is full of rock.
			const FLOAT32 minV = 1.0f - (entityBlock->mAABBMaxScreenSpacePointY[entityIndex] / cullingHeight);
			const FLOAT32 maxV = 1.0f - (entityBlock->mAABBMinScreenSpacePointY[entityIndex] / cullingHeight);

			// Widened by a cell on every side. The depth being tested against is
			// a frame or two old, so an object may have moved, or the camera may
			// have. Testing a larger rectangle can only bring in more cells,
			// which can only raise the farthest depth found, which can only make
			// the test less willing to cull. The error goes towards drawing
			// something needlessly rather than dropping something visible.
			const INT32 startX = static_cast<INT32>(math::Max(0.0f, minU) * mHiZReadbackWidth) - 1;
			const INT32 endX = static_cast<INT32>(math::Min(1.0f, maxU) * mHiZReadbackWidth) + 1;
			const INT32 startY = static_cast<INT32>(math::Max(0.0f, minV) * mHiZReadbackHeight) - 1;
			const INT32 endY = static_cast<INT32>(math::Min(1.0f, maxV) * mHiZReadbackHeight) + 1;

			if (startX > endX || startY > endY)
			{
				continue;
			}

			testedCount++;

			// Occluded only if the object's nearest point is behind everything
			// drawn in every cell it covers, so one cell that something is in
			// front of settles it. Stopping there rather than scanning the whole
			// rectangle matters: most objects are visible, and a visible one
			// usually proves it on the first cell or two.
			//
			// A rectangle touching empty background can never be culled, because
			// background reads as the far value and nothing is behind that.
			//
			// The comparison is direct, in one shared space: D3D11 clips z to
			// zero and one, so a projection producing any other range could not
			// render a correct frame, and this one does.
			bool bIsOccluded = true;

			for (INT32 y = math::Max(0, startY); bIsOccluded && y <= endY && y < static_cast<INT32>(mHiZReadbackHeight); y++)
			{
				const FLOAT32* const row = mHiZReadbackData.data() + static_cast<size_t>(y) * mHiZReadbackWidth;

				for (INT32 x = math::Max(0, startX); x <= endX && x < static_cast<INT32>(mHiZReadbackWidth); x++)
				{
					if (objectNDCZ < row[x])
					{
						bIsOccluded = false;
						break;
					}
				}
			}

			if (bIsOccluded)
			{
				culledAsRawCount++;

				if (dooms::graphics::graphicsSetting::IsHiZOcclusionCullingEnabled)
				{
					entityBlock->SetCulled(entityIndex, cameraIndex);
				}
			}
		}
	}

	graphicsSetting::CpuStatHiZTestMilliseconds = static_cast<FLOAT32>(
		std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
			std::chrono::steady_clock::now() - testStartTime).count());

	if ((mHiZFrameCounter % 300) == 0 && testedCount > 0)
	{
		D_RELEASE_LOG(eLogType::D_LOG,
			"HiZ occlusion : tested %u, occluded %u, culling %s, object ndc z range %f .. %f",
			testedCount, culledAsRawCount,
			dooms::graphics::graphicsSetting::IsHiZOcclusionCullingEnabled ? "on" : "off",
			observedMinNDCZ, observedMaxNDCZ);
	}
}

namespace
{
	// Rejected against every plane using the box corner furthest along that
	// plane's normal. If even that corner is behind the plane, nothing in the
	// box is in front of it.
	//
	// Shared by the traversal and by the audit below so that a disagreement
	// between them can only come from the bounds, never from two subtly
	// different implementations of the same test.
	bool IsAABBOutsideFrustum(
		const std::array<math::Vector4, 6>& frustumPlanes,
		const math::Vector3& lowerBound,
		const math::Vector3& upperBound)
	{
		for (size_t planeIndex = 0; planeIndex < frustumPlanes.size(); planeIndex++)
		{
			const math::Vector4& plane = frustumPlanes[planeIndex];

			const math::Vector3 furthestCorner(
				(plane.x >= 0.0f) ? upperBound.x : lowerBound.x,
				(plane.y >= 0.0f) ? upperBound.y : lowerBound.y,
				(plane.z >= 0.0f) ? upperBound.z : lowerBound.z);

			if ((plane.x * furthestCorner.x + plane.y * furthestCorner.y + plane.z * furthestCorner.z + plane.w) < 0.0f)
			{
				return true;
			}
		}

		return false;
	}
}

void dooms::graphics::DeferredRenderingPipeLine::ApplyBVHFrustumCulling(dooms::Camera* const targetCamera, const size_t cameraIndex)
{
	if (dooms::graphics::graphicsSetting::IsBVHFrustumCullingEnabled == false)
	{
		return;
	}

	const std::chrono::steady_clock::time_point cullStartTime = std::chrono::steady_clock::now();

	dooms::BVHAABB3D& rendererBVH = mGraphicsServer.mRendererColliderBVH;

	const INT32 rootNodeIndex = rendererBVH.GetRootNodeIndex();
	if (rendererBVH.GetIsNodeValid(rootNodeIndex) == false)
	{
		return;
	}

	const std::array<math::Vector4, 6> frustumPlanes = targetCamera->CalculateFrustumPlane();

	mBVHVisibleNodes.assign(rendererBVH.GetNodeCapacity(), false);
	mBVHTraversalStack.clear();
	mBVHTraversalStack.push_back(rootNodeIndex);

	while (mBVHTraversalStack.empty() == false)
	{
		const INT32 nodeIndex = mBVHTraversalStack.back();
		mBVHTraversalStack.pop_back();

		const dooms::BVHAABB3D::node_type* const node = rendererBVH.GetNode(nodeIndex);
		if (node == nullptr)
		{
			continue;
		}

		const math::Vector3 lowerBound = static_cast<math::Vector3>(node->mBoundingCollider.mLowerBound);
		const math::Vector3 upperBound = static_cast<math::Vector3>(node->mBoundingCollider.mUpperBound);

		// Rejected against every plane using the box corner furthest along that
		// plane's normal. If even that corner is behind the plane, nothing in the
		// box is in front of it, and nothing in the subtree can be visible.
		if (IsAABBOutsideFrustum(frustumPlanes, lowerBound, upperBound))
		{
			// The whole subtree goes with it. This is the saving: one test
			// instead of one per object underneath.
			continue;
		}

		mBVHVisibleNodes[static_cast<size_t>(nodeIndex)] = true;

		if (node->mIsLeaf == false)
		{
			if (rendererBVH.GetIsNodeValid(node->mLeftNode))
			{
				mBVHTraversalStack.push_back(node->mLeftNode);
			}

			if (rendererBVH.GetIsNodeValid(node->mRightNode))
			{
				mBVHTraversalStack.push_back(node->mRightNode);
			}
		}
	}

	// Renderers know their own node, so the result is read that way round. The
	// tree has no way back to a renderer: leaves are inserted with a null
	// collider, and nothing else identifies the owner.
	const std::vector<Renderer*>& renderers = RendererComponentStaticIterator::GetSingleton()->GetSortedRendererInLayer();

	UINT32 disagreementCount = 0;

	for (Renderer* const renderer : renderers)
	{
		if (IsValid(renderer) == false || renderer->GetIsComponentEnabled() == false)
		{
			continue;
		}

		// Through the node rather than the view's index, which is private.
		const dooms::BVH_Node_View<dooms::physics::AABB3D>& nodeView = renderer->BVH_AABB3D_Node_Object::mBVH_Node_View;
		if (nodeView.IsValid() == false)
		{
			continue;
		}

		const dooms::BVHAABB3D::node_type* const rendererNode = nodeView.GetNode();
		if (rendererNode == nullptr)
		{
			continue;
		}

		const INT32 nodeIndex = rendererNode->mIndex;

		if (nodeIndex < 0 || static_cast<size_t>(nodeIndex) >= mBVHVisibleNodes.size())
		{
			continue;
		}

		if (mBVHVisibleNodes[static_cast<size_t>(nodeIndex)] == false)
		{
			culling::EntityBlockViewer& viewer = renderer->mCullingEntityBlockViewer;

			if (viewer.IsValid())
			{
				// Audited against the entity block's world bounds rather than
				// against the other modules' verdict, because enabling the tree
				// switches the per object frustum module off, so there is no
				// second frustum opinion left to compare with.
				//
				// These bounds are rewritten every frame from the renderer's
				// current transform, so they are the one description of where
				// the object is now that the tree cannot have corrupted. Same
				// planes, same test, current bounds: anything the tree rejects
				// that this accepts is the tree culling a visible object.
				const culling::EntityBlock* const entityBlock = viewer.GetTargetEntityBlock();
				const UINT32 entityIndex = viewer.GetEntityIndexInBlock();

				const culling::Vec4& minWorldPoint = entityBlock->mAABBMinWorldPoint[entityIndex];
				const culling::Vec4& maxWorldPoint = entityBlock->mAABBMaxWorldPoint[entityIndex];

				if (IsAABBOutsideFrustum(
						frustumPlanes,
						math::Vector3(minWorldPoint[0], minWorldPoint[1], minWorldPoint[2]),
						math::Vector3(maxWorldPoint[0], maxWorldPoint[1], maxWorldPoint[2])) == false)
				{
					disagreementCount++;
				}

				viewer.GetTargetEntityBlock()->SetCulled(viewer.GetEntityIndexInBlock(), cameraIndex);
			}
		}
	}

	// Cleared here rather than where it is set, because every renderer's
	// PreRender has run by this point in the frame, so all of them have had the
	// chance to act on it exactly once.
	graphicsSetting::IsBVHFullRefreshRequested = false;

	graphicsSetting::CullStatBVHDisagreementCount = disagreementCount;

	graphicsSetting::CpuStatBVHCullMilliseconds = static_cast<FLOAT32>(
		std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
			std::chrono::steady_clock::now() - cullStartTime).count());
}

void dooms::graphics::DeferredRenderingPipeLine::UpdateCullStatistics(const size_t cameraIndex)
{
	culling::EveryCulling* const cullingSystem = mRenderingCullingManager.mCullingSystem.get();
	if (cullingSystem == nullptr)
	{
		return;
	}

	UINT32 entityCount = 0;
	UINT32 culledCount = 0;

	for (culling::EntityBlock* const entityBlock : cullingSystem->GetActiveEntityBlockList())
	{
		if (entityBlock == nullptr)
		{
			continue;
		}

		for (size_t entityIndex = 0; entityIndex < entityBlock->mCurrentEntityCount; entityIndex++)
		{
			if (entityBlock->GetIsObjectEnabled(entityIndex) == false)
			{
				continue;
			}

			entityCount++;

			if (entityBlock->GetIsCulled(entityIndex, cameraIndex) == true)
			{
				culledCount++;
			}
		}
	}

	dooms::graphics::graphicsSetting::CullStatEntityCount = entityCount;
	dooms::graphics::graphicsSetting::CullStatCulledCount = culledCount;
}

void dooms::graphics::DeferredRenderingPipeLine::UpdateHiZVisualization()
{
	if (dooms::graphics::graphicsSetting::IsHiZVisualizationEnabled == false)
	{
		if (IsValid(mHiZPIP))
		{
			mHiZPIP->bmIsDrawOnScreen = false;
		}
		return;
	}

	if (IsValid(mHiZPIP) == false)
	{
		if (IsValid(mHiZTexture) == false)
		{
			return;
		}

		mHiZPIP = CreateFullscreenPIP(mHiZTexture->GenerateTextureView(0, GraphicsAPI::eGraphicsPipeLineStage::PIXEL_SHADER));

		if (IsValid(mHiZPIP) == false)
		{
			return;
		}

		// The stock material samples without naming a level and shows raw depth,
		// which came out as a flat red field: the values all sit against one and
		// the hardware picks the level. This one takes the level explicitly and
		// puts it through the same ramp as the depth view, so a level of the
		// pyramid can be compared against the buffer it came from.
		dooms::asset::ShaderAsset* const presentShader
			= dooms::assetImporter::AssetManager::GetSingleton()->GetAsset<dooms::asset::eAssetType::SHADER>("HiZPresentShader.glsl");

		D_ASSERT(IsValid(presentShader));
		if (IsValid(presentShader))
		{
			mHiZPresentMaterial = presentShader->CreateMatrialWithThisShaderAsset();
			mHiZPresentMaterial->AddToRootObjectList();
			mHiZPIP->SetMaterial(mHiZPresentMaterial);
		}
	}

	// Set every frame, because the level being inspected can change.
	if (IsValid(mHiZPresentMaterial))
	{
		UniformBufferObjectView* const presentDataView
			= mHiZPresentMaterial->GetUniformBufferObjectViewFromUBOName("HiZPresentData");

		if (presentDataView != nullptr)
		{
			presentDataView->SetVector4((UINT64)0, math::Vector4(
				static_cast<FLOAT32>(graphicsSetting::HiZVisualizationLevel), 0.0f, 0.0f, 0.0f));
		}
	}

	mHiZPIP->bmIsDrawOnScreen = true;
}

dooms::graphics::PicktureInPickture* dooms::graphics::DeferredRenderingPipeLine::CreateFullscreenPIP(TextureView* const textureView)
{
	if (IsValid(textureView) == false)
	{
		return nullptr;
	}

	PicktureInPickture* const pip = mGraphicsServer.mPIPManager.AddNewPIP(
		math::Vector2(-1.0f, -1.0f),
		math::Vector2(1.0f, 1.0f),
		textureView);

	if (IsValid(pip))
	{
		pip->AddToRootObjectList();
	}

	return pip;
}

void dooms::graphics::DeferredRenderingPipeLine::UpdateAlbedoVisualization(dooms::Camera* const targetCamera)
{
	if (dooms::graphics::graphicsSetting::RenderMode != dooms::graphics::graphicsSetting::eRenderMode::Textured)
	{
		if (IsValid(mAlbedoPIP))
		{
			mAlbedoPIP->bmIsDrawOnScreen = false;
		}
		return;
	}

	if (IsValid(mAlbedoPIP) == false)
	{
		dooms::graphics::DeferredRenderingPipeLineCamera* const deferredRenderingPipeLineCamera
			= CastTo<graphics::DeferredRenderingPipeLineCamera*>(targetCamera->GetGraphicsPipeLineCamera());

		if (IsValid(deferredRenderingPipeLineCamera) == false)
		{
			return;
		}

		// Albedo is colour attachment 2 of the g-buffer, after position and
		// normal. Shown with the stock material, since it is already a plain
		// colour texture and wants no interpretation.
		mAlbedoPIP = CreateFullscreenPIP(
			deferredRenderingPipeLineCamera->mDeferredRenderingFrameBuffer.GetColorTextureView(2, GraphicsAPI::eGraphicsPipeLineStage::PIXEL_SHADER));

		if (IsValid(mAlbedoPIP) == false)
		{
			return;
		}
	}

	mAlbedoPIP->bmIsDrawOnScreen = true;
}

void dooms::graphics::DeferredRenderingPipeLine::UpdateDepthBufferVisualization(dooms::Camera* const targetCamera)
{
	if (dooms::graphics::graphicsSetting::IsDepthBufferVisualizationEnabled == false)
	{
		if (IsValid(mDepthBufferPIP))
		{
			mDepthBufferPIP->bmIsDrawOnScreen = false;
		}
		return;
	}

	if (IsValid(mDepthBufferPIP) == false)
	{
		dooms::graphics::DeferredRenderingPipeLineCamera* const deferredRenderingPipeLineCamera
			= CastTo<graphics::DeferredRenderingPipeLineCamera*>(targetCamera->GetGraphicsPipeLineCamera());

		if (IsValid(deferredRenderingPipeLineCamera) == false)
		{
			return;
		}

		TextureView* const depthTextureView
			= deferredRenderingPipeLineCamera->mDeferredRenderingFrameBuffer.GetDepthTextureView(0, GraphicsAPI::eGraphicsPipeLineStage::PIXEL_SHADER);

		if (IsValid(depthTextureView) == false)
		{
			return;
		}

		mDepthBufferPIP = CreateFullscreenPIP(depthTextureView);

		if (IsValid(mDepthBufferPIP) == false)
		{
			return;
		}

		// The stock picture-in-picture material would show raw depth, which is
		// so non linear that everything past the near plane reads as flat white.
		// This one linearises against the camera planes first.
		dooms::asset::ShaderAsset* const depthShader
			= dooms::assetImporter::AssetManager::GetSingleton()->GetAsset<dooms::asset::eAssetType::SHADER>("DepthBufferTextureShader.glsl");

		D_ASSERT(IsValid(depthShader));
		if (IsValid(depthShader))
		{
			mDepthBufferPresentMaterial = depthShader->CreateMatrialWithThisShaderAsset();
			mDepthBufferPresentMaterial->AddToRootObjectList();
			mDepthBufferPIP->SetMaterial(mDepthBufferPresentMaterial);
		}
	}

	mDepthBufferPIP->bmIsDrawOnScreen = true;
}

void dooms::graphics::DeferredRenderingPipeLine::CameraRender(dooms::Camera* const targetCamera, const size_t cameraIndex)
{
	D_ASSERT(IsValid(targetCamera));

	if (targetCamera->GetIsCullJobEnabled() == true)
	{
		mRenderingCullingManager.CameraCullJob(targetCamera); // do this first
	}

	// Applied here, after the cull job and before anything is drawn.
	//
	// PreCullJob resets every entity to visible at the start of each frame, so
	// marking one culled any later than this is wiped before it can matter. The
	// data it reads is from an earlier frame, which is the point: waiting for
	// this frame's pyramid would stall the cpu on the gpu.
	// Before the Hi-Z test, so it can skip whatever this already removed.
	ApplyBVHFrustumCulling(targetCamera, cameraIndex);
	ApplyHiZOcclusionCulling(cameraIndex);
	UpdateCullStatistics(cameraIndex);

	if (dooms::graphics::graphicsSetting::IsSortObjectFrontToBack == true)
	{
		FrontToBackSort(targetCamera->GetTransform()->GetPosition(), cameraIndex);
	}

	FrameBuffer::StaticBindBackFrameBuffer();
	GraphicsAPI::ClearBackFrameBufferColorBuffer(targetCamera->mClearColor[0], targetCamera->mClearColor[1], targetCamera->mClearColor[2], targetCamera->mClearColor[3]);
	GraphicsAPI::ClearBackFrameBufferDepthBuffer(GraphicsAPI::DEFAULT_MAX_DEPTH_VALUE);
	
	dooms::graphics::DeferredRenderingPipeLineCamera* const deferredRenderingPipeLineCamera = CastTo<graphics::DeferredRenderingPipeLineCamera*>(dooms::Camera::GetMainCamera()->GetGraphicsPipeLineCamera());
	D_ASSERT(IsValid(deferredRenderingPipeLineCamera));
	if (IsValid(deferredRenderingPipeLineCamera))
	{
		deferredRenderingPipeLineCamera->mDeferredRenderingFrameBuffer.ClearFrameBuffer(targetCamera);
		deferredRenderingPipeLineCamera->mDeferredRenderingFrameBuffer.BindFrameBuffer();
	}

	targetCamera->UpdateUniformBufferObject();


	if
	(
		dooms::graphics::graphicsAPISetting::DepthPrePassType == dooms::graphics::eDepthPrePassType::AllOpaque //||
		//dooms::graphics::graphicsAPISetting::DepthPrePassType == dooms::graphics::eDepthPrePassType::ConsiderBound
	)
	{
		DrawRenderersWithDepthOnly(targetCamera, cameraIndex);
	}
	

	{
		D_START_PROFILING(RenderObject, dooms::profiler::eProfileLayers::Rendering);
		GraphicsAPI::SetIsDepthTestEnabled(true);
		GraphicsAPI::SetDepthMask(true);
		GraphicsAPI::SetDepthFunc(GraphicsAPI::eTestFuncType::LEQUAL);

		// Wireframe is rasterizer state, so it wraps the geometry pass only and
		// is put back afterwards. Leaving it on would draw the deferred lighting
		// quad and every debug overlay as wireframe too.
		//
		// Null checked because the entry point is resolved from the graphics DLL
		// and an older one will not have it.
		const bool bIsWireframe =
			(dooms::graphics::graphicsSetting::RenderMode == dooms::graphics::graphicsSetting::eRenderMode::Wireframe) &&
			(GraphicsAPI::SetFillMode != nullptr);

		if (bIsWireframe)
		{
			GraphicsAPI::SetFillMode(GraphicsAPI::eFillMode::FILLMODE_WIREFRAME);
		}

		DrawBatchedRenderers();
		DrawRenderers(targetCamera, cameraIndex);

		if (bIsWireframe)
		{
			GraphicsAPI::SetFillMode(GraphicsAPI::eFillMode::FILLMODE_SOLID);
		}

		D_END_PROFILING(RenderObject);
	}

	// Overdraw gets its own pass over the scene. It forces every renderer onto a
	// material that adds a fixed amount per fragment, which cannot be shared
	// with the pass that fills the g-buffer, and it needs the depth test off so
	// that hidden layers still count.
	if (targetCamera->IsMainCamera() == true)
	{
		dooms::graphics::OverDrawVisualization* const overDrawVisualization = dooms::graphics::OverDrawVisualization::GetSingleton();
		if (overDrawVisualization != nullptr)
		{
			if (dooms::graphics::graphicsSetting::IsOverDrawVisualizationEnabled == true)
			{
				D_START_PROFILING(OverDrawVisualization, dooms::profiler::eProfileLayers::Rendering);
				overDrawVisualization->BeginOverDrawPass();
				DrawBatchedRenderers();
				DrawRenderers(targetCamera, cameraIndex);
				overDrawVisualization->EndOverDrawPass();
				D_END_PROFILING(OverDrawVisualization);
			}
			else
			{
				overDrawVisualization->HideOverDrawVisualization();
			}
		}
	}

	// After the geometry pass, so the depth buffer it reads is complete, and
	// before the lighting resolve, so anything that comes to depend on the
	// pyramid can have it within the same frame.
	if (targetCamera->IsMainCamera() == true)
	{
		BuildHiZPyramid(targetCamera);
	}

	FrameBuffer::StaticBindBackFrameBuffer();

	if (targetCamera->IsMainCamera() == true)
	{
		//Only Main Camera can draw to screen buffer
		dooms::graphics::DeferredRenderingPipeLineCamera* const deferredRenderingPipeLineCamera = CastTo<graphics::DeferredRenderingPipeLineCamera*>(targetCamera->GetGraphicsPipeLineCamera());
		D_ASSERT(IsValid(deferredRenderingPipeLineCamera));
		if (IsValid(deferredRenderingPipeLineCamera))
		{
			deferredRenderingPipeLineCamera->mDeferredRenderingFrameBuffer.BindGBufferTextures();
			mDeferredRenderingDrawer.DrawDeferredRenderingQuadDrawer();
			deferredRenderingPipeLineCamera->mDeferredRenderingFrameBuffer.UnBindGBufferTextures();
		}

		// Updated here because the depth attachment is only safe to sample once
		// the g-buffer is unbound, which the back buffer bind above has done.
		UpdateDepthBufferVisualization(targetCamera);
		UpdateAlbedoVisualization(targetCamera);
		UpdateHiZVisualization();

		// After the lighting resolve, not before it. The resolve covers the
		// whole back buffer, so anything drawn ahead of it was painted over --
		// which meant a picture-in-picture could never actually be seen.
		mGraphicsServer.mPIPManager.DrawPIPs();

		mRenderingDebugger.CameraRender(targetCamera);
	}
}


