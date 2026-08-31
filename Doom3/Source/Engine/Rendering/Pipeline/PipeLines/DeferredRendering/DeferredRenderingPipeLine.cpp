#include "DeferredRenderingPipeLine.h"

#include <Rendering/Culling/OccludeeHull.h>
#include <Rendering/Renderer/MeshRenderer.h>

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

namespace
{
	// The 2D convex hull of the projected hull vertices, by monotone chain.
	//
	// For a convex body this is exactly the outline of its projection, which is
	// why the shape being projected has to be a hull in the first place: the
	// outline of a projected non convex mesh is not the hull of its projected
	// vertices, and testing against it would cull visible objects.
	void BuildConvexOutline(
		std::vector<std::pair<FLOAT32, FLOAT32>> points,
		std::vector<std::pair<FLOAT32, FLOAT32>>& outOutline)
	{
		outOutline.clear();

		if (points.size() < 3)
		{
			return;
		}

		std::sort(points.begin(), points.end());

		const auto Cross = [](const std::pair<FLOAT32, FLOAT32>& o,
			const std::pair<FLOAT32, FLOAT32>& a,
			const std::pair<FLOAT32, FLOAT32>& b)
		{
			return (a.first - o.first) * (b.second - o.second) - (a.second - o.second) * (b.first - o.first);
		};

		outOutline.resize(points.size() * 2);

		size_t outlineSize = 0;

		for (size_t pointIndex = 0; pointIndex < points.size(); pointIndex++)
		{
			while (outlineSize >= 2 &&
				Cross(outOutline[outlineSize - 2], outOutline[outlineSize - 1], points[pointIndex]) <= 0.0f)
			{
				outlineSize--;
			}

			outOutline[outlineSize++] = points[pointIndex];
		}

		const size_t lowerSize = outlineSize + 1;

		for (size_t pointIndex = points.size() - 1; pointIndex > 0; pointIndex--)
		{
			while (outlineSize >= lowerSize &&
				Cross(outOutline[outlineSize - 2], outOutline[outlineSize - 1], points[pointIndex - 1]) <= 0.0f)
			{
				outlineSize--;
			}

			outOutline[outlineSize++] = points[pointIndex - 1];
		}

		outOutline.resize((outlineSize > 0) ? (outlineSize - 1) : 0);
	}

	// The horizontal extent of the outline anywhere within one row of cells.
	//
	// Taken over the whole band rather than at a single scanline, because a cell
	// is covered if the object touches any part of it. Returns false when the
	// outline does not reach the row at all.
	bool GetOutlineSpanForRow(
		const std::vector<std::pair<FLOAT32, FLOAT32>>& outline,
		const FLOAT32 rowTop,
		const FLOAT32 rowBottom,
		FLOAT32& outMinX,
		FLOAT32& outMaxX)
	{
		bool bHasSpan = false;

		outMinX = 0.0f;
		outMaxX = 0.0f;

		const auto Accumulate = [&](const FLOAT32 x)
		{
			if (bHasSpan == false)
			{
				outMinX = x;
				outMaxX = x;
				bHasSpan = true;
			}
			else
			{
				outMinX = math::Min(outMinX, x);
				outMaxX = math::Max(outMaxX, x);
			}
		};

		for (size_t edgeIndex = 0; edgeIndex < outline.size(); edgeIndex++)
		{
			const std::pair<FLOAT32, FLOAT32>& start = outline[edgeIndex];
			const std::pair<FLOAT32, FLOAT32>& end = outline[(edgeIndex + 1) % outline.size()];

			// A vertex sitting inside the band contributes its own x.
			if (start.second >= rowTop && start.second <= rowBottom)
			{
				Accumulate(start.first);
			}

			const FLOAT32 edgeMinY = math::Min(start.second, end.second);
			const FLOAT32 edgeMaxY = math::Max(start.second, end.second);

			if (edgeMaxY < rowTop || edgeMinY > rowBottom)
			{
				continue;
			}

			const FLOAT32 deltaY = end.second - start.second;

			if (std::fabs(deltaY) < 1e-6f)
			{
				// Horizontal edge lying in the band: both ends count.
				Accumulate(start.first);
				Accumulate(end.first);
				continue;
			}

			// Where the edge crosses the top and bottom of the band, clamped to
			// the edge itself so an edge ending inside the band is not run past.
			for (const FLOAT32 bandY : { rowTop, rowBottom })
			{
				const FLOAT32 t = (bandY - start.second) / deltaY;

				if (t >= 0.0f && t <= 1.0f)
				{
					Accumulate(start.first + (end.first - start.first) * t);
				}
			}
		}

		return bHasSpan;
	}
}

void dooms::graphics::DeferredRenderingPipeLine::ApplyHiZHullOcclusionCulling(dooms::Camera* const targetCamera, const size_t cameraIndex)
{
	if (graphicsSetting::IsHiZHullOccludeeEnabled == false ||
		bmIsHiZReadbackDataValid == false ||
		mHiZReadbackData.empty())
	{
		return;
	}

	const std::chrono::steady_clock::time_point testStartTime = std::chrono::steady_clock::now();

	for (unsigned int& bucketCount : graphicsSetting::CullStatHullCullsBySize)
	{
		bucketCount = 0;
	}

	graphicsSetting::CullStatHullTestedCount = 0;
	graphicsSetting::CullStatHullSkippedCount = 0;

	// The same normalisation the box path uses, since the screen bounds are in
	// the culling system's resolution rather than the pyramid's.
	culling::EveryCulling* const cullingSystemForBounds = mRenderingCullingManager.mCullingSystem.get();
	if (cullingSystemForBounds == nullptr)
	{
		return;
	}

	const culling::SWDepthBuffer& hullDepthBuffer = cullingSystemForBounds->mMaskedSWOcclusionCulling->mDepthBuffer;
	const FLOAT32 cullingWidth = static_cast<FLOAT32>(hullDepthBuffer.mResolution.mWidth);
	const FLOAT32 cullingHeight = static_cast<FLOAT32>(hullDepthBuffer.mResolution.mHeight);

	if (cullingWidth <= 0.0f || cullingHeight <= 0.0f)
	{
		return;
	}

	const math::Matrix4x4 viewProjectionMatrix = targetCamera->GetViewProjectionMatrix();

	const std::vector<Renderer*>& renderers = RendererComponentStaticIterator::GetSingleton()->GetSortedRendererInLayer();

	for (Renderer* const renderer : renderers)
	{
		if (IsValid(renderer) == false || renderer->GetIsComponentEnabled() == false)
		{
			continue;
		}

		culling::EntityBlockViewer& viewer = renderer->mCullingEntityBlockViewer;
		if (viewer.IsValid() == false)
		{
			continue;
		}

		culling::EntityBlock* const entityBlock = viewer.GetTargetEntityBlock();
		const UINT32 entityIndex = viewer.GetEntityIndexInBlock();

		// Already gone by the frustum, by distance, or by the box test. This
		// only pays for the ones those could not decide.
		if (entityBlock->GetIsCulled(entityIndex, cameraIndex) == true)
		{
			continue;
		}

		const dooms::MeshRenderer* const meshRenderer = dooms::CastTo<const dooms::MeshRenderer*>(renderer);
		if (IsValid(meshRenderer) == false)
		{
			continue;
		}

		// Screen coverage from the bounds PreCulling already produced, so the
		// decision not to project a hull costs nothing to make.
		//
		// A rock covering a couple of cells is quantised into the same cells
		// whether it is boxed or outlined, and the staleness margin is wider
		// than the difference. The measurement agrees: nothing under eight cells
		// was ever culled by the hull that the box had not already kept.
		const FLOAT32 screenCellWidth =
			((entityBlock->mAABBMaxScreenSpacePointX[entityIndex] - entityBlock->mAABBMinScreenSpacePointX[entityIndex]) / cullingWidth)
			* mHiZReadbackWidth;
		const FLOAT32 screenCellHeight =
			((entityBlock->mAABBMaxScreenSpacePointY[entityIndex] - entityBlock->mAABBMinScreenSpacePointY[entityIndex]) / cullingHeight)
			* mHiZReadbackHeight;

		if ((screenCellWidth * screenCellHeight) < static_cast<FLOAT32>(graphicsSetting::HiZHullMinCellCount))
		{
			graphicsSetting::CullStatHullSkippedCount++;
			continue;
		}

		const OccludeeHull& hull = GetOccludeeHull(meshRenderer->GetMesh());
		if (hull.mVertices.empty())
		{
			continue;
		}

		graphicsSetting::CullStatHullTestedCount++;

		const math::Matrix4x4 modelViewProjection = viewProjectionMatrix * renderer->GetModelMatrix();

		FLOAT32 minU = 1.0f;
		FLOAT32 maxU = 0.0f;
		FLOAT32 minV = 1.0f;
		FLOAT32 maxV = 0.0f;
		FLOAT32 nearestNDCZ = 1.0f;

		// Kept so the projected outline can be rasterised rather than boxed.
		// Static because this runs for thousands of objects a frame and the size
		// is bounded by the hull vertex budget.
		static std::vector<std::pair<FLOAT32, FLOAT32>> projectedPoints;
		projectedPoints.clear();

		bool bIsTestable = true;

		for (const math::Vector3& hullVertex : hull.mVertices)
		{
			const math::Vector4 clipPosition = modelViewProjection * math::Vector4(hullVertex.x, hullVertex.y, hullVertex.z, 1.0f);

			// Straddling the near plane, where the projection is not usable and
			// a wrong answer would remove something in front of the camera.
			if (clipPosition.w <= 0.0001f)
			{
				bIsTestable = false;
				break;
			}

			const FLOAT32 inverseW = 1.0f / clipPosition.w;
			const FLOAT32 ndcX = clipPosition.x * inverseW;
			const FLOAT32 ndcY = clipPosition.y * inverseW;
			const FLOAT32 ndcZ = clipPosition.z * inverseW;

			// Same mapping the box path uses, taken straight from ndc: u runs
			// left to right, v is flipped because the pyramid's first row is the
			// top of the screen while ndc y is positive upwards.
			const FLOAT32 u = (ndcX + 1.0f) * 0.5f;
			const FLOAT32 v = (1.0f - ndcY) * 0.5f;

			minU = math::Min(minU, u);
			maxU = math::Max(maxU, u);
			minV = math::Min(minV, v);
			maxV = math::Max(maxV, v);

			nearestNDCZ = math::Min(nearestNDCZ, ndcZ);

			projectedPoints.emplace_back(u * mHiZReadbackWidth, v * mHiZReadbackHeight);
		}

		if (bIsTestable == false)
		{
			continue;
		}

		// The hull contains the mesh, so its nearest vertex is the nearest the
		// object can be. That is the whole point: the box's nearest corner sits
		// well in front of a rounded rock and made it untestable against
		// anything it was tucked behind.
		const INT32 startX = static_cast<INT32>(math::Max(0.0f, minU) * mHiZReadbackWidth) - 1;
		const INT32 endX = static_cast<INT32>(math::Min(1.0f, maxU) * mHiZReadbackWidth) + 1;
		const INT32 startY = static_cast<INT32>(math::Max(0.0f, minV) * mHiZReadbackHeight) - 1;
		const INT32 endY = static_cast<INT32>(math::Min(1.0f, maxV) * mHiZReadbackHeight) + 1;

		if (startX > endX || startY > endY)
		{
			continue;
		}

		const INT32 coveredCellCount = (endX - startX + 1) * (endY - startY + 1);

		// The outline itself, but only where the rectangle around it is loose
		// enough to be worth the scanline. For an object covering a handful of
		// cells the two differ by less than the margin already added.
		const bool bShouldRasterisePolygon =
			(graphicsSetting::IsHiZHullPolygonEnabled == true) &&
			(coveredCellCount >= static_cast<INT32>(graphicsSetting::HiZHullPolygonMinCellCount)) &&
			(projectedPoints.size() >= 3);

		static std::vector<std::pair<FLOAT32, FLOAT32>> outline;

		if (bShouldRasterisePolygon)
		{
			BuildConvexOutline(projectedPoints, outline);
		}

		bool bIsOccluded = true;

		for (INT32 y = math::Max(0, startY); bIsOccluded && y <= endY && y < static_cast<INT32>(mHiZReadbackHeight); y++)
		{
			INT32 rowStartX = startX;
			INT32 rowEndX = endX;

			if (bShouldRasterisePolygon && outline.size() >= 3)
			{
				FLOAT32 rowMinX = 0.0f;
				FLOAT32 rowMaxX = 0.0f;

				// Nothing of the outline crosses this row, so there is nothing
				// of the object here to be occluded by anything.
				if (GetOutlineSpanForRow(outline, static_cast<FLOAT32>(y), static_cast<FLOAT32>(y + 1), rowMinX, rowMaxX) == false)
				{
					continue;
				}

				// Rounded outwards, and the staleness margin kept, so the
				// narrower span can still only ever add cells to the test.
				rowStartX = static_cast<INT32>(std::floor(rowMinX)) - 1;
				rowEndX = static_cast<INT32>(std::ceil(rowMaxX)) + 1;

				rowStartX = math::Max(rowStartX, startX);
				rowEndX = math::Min(rowEndX, endX);
			}

			const FLOAT32* const row = mHiZReadbackData.data() + static_cast<size_t>(y) * mHiZReadbackWidth;

			for (INT32 x = math::Max(0, rowStartX); x <= rowEndX && x < static_cast<INT32>(mHiZReadbackWidth); x++)
			{
				if (nearestNDCZ < row[x])
				{
					bIsOccluded = false;
					break;
				}
			}
		}

		if (bIsOccluded)
		{
			entityBlock->SetCulled(entityIndex, cameraIndex);

			// Bucketed by screen coverage, because that is what decides whether
			// spending hull vertices on this object was worth it.
			const size_t sizeBucket =
				(coveredCellCount < 2) ? 0 :
				(coveredCellCount < 8) ? 1 :
				(coveredCellCount < 32) ? 2 :
				(coveredCellCount < 128) ? 3 : 4;

			graphicsSetting::CullStatHullCullsBySize[sizeBucket]++;
		}
	}

	GetOccludeeHullStatistics(
		graphicsSetting::CullStatHullMeshCount,
		graphicsSetting::CullStatHullVertexCount);

	graphicsSetting::CpuStatHiZTestMilliseconds += static_cast<FLOAT32>(
		std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
			std::chrono::steady_clock::now() - testStartTime).count());
}

void dooms::graphics::DeferredRenderingPipeLine::MeasureTrueVisibility(dooms::Camera* const targetCamera, const size_t cameraIndex)
{
	if (GraphicsAPI::CreateQuery == nullptr || GraphicsAPI::BeginQuery == nullptr ||
		GraphicsAPI::EndQuery == nullptr || GraphicsAPI::GetQueryResult == nullptr)
	{
		return;
	}

	// Read last frame's answers before issuing this frame's, so the gpu has had
	// a whole frame to finish them and nothing here ever blocks on it.
	if (mVisibilityOraclePendingCount > 0)
	{
		UINT32 resolvedCount = 0;
		UINT32 invisibleCount = 0;

		for (UINT32 queryIndex = 0; queryIndex < mVisibilityOraclePendingCount; queryIndex++)
		{
			unsigned long long passedSampleCount = 0;

			if (GraphicsAPI::GetQueryResult(
					mVisibilityOracleQueries[queryIndex],
					GraphicsAPI::QUERY_OCCLUSION,
					&passedSampleCount,
					nullptr) != 0)
			{
				resolvedCount++;

				if (passedSampleCount == 0)
				{
					invisibleCount++;
				}
			}
		}

		// Only published when every query came back. A partial answer would
		// read as an improvement rather than as a missing measurement.
		if (resolvedCount == mVisibilityOraclePendingCount)
		{
			graphicsSetting::CullStatOracleTestedCount = resolvedCount;
			graphicsSetting::CullStatOracleInvisibleCount = invisibleCount;
		}

		mVisibilityOraclePendingCount = 0;
	}

	if (graphicsSetting::IsVisibilityOracleEnabled == false)
	{
		graphicsSetting::CullStatOracleTestedCount = 0;
		graphicsSetting::CullStatOracleInvisibleCount = 0;
		return;
	}

	const std::vector<Renderer*>& renderers = RendererComponentStaticIterator::GetSingleton()->GetSortedRendererInLayer();
	const bool bIsCameraCulling = targetCamera->GetCameraFlag(dooms::eCameraFlag::IS_CULLED);

	// Depth only, testing against the finished buffer and writing nothing to
	// it, so this measures the frame rather than changing it.
	FixedMaterial::GetSingleton()->SetFixedMaterial(GetDepthOnlyMaterial());
	GraphicsAPI::SetIsDepthTestEnabled(true);
	GraphicsAPI::SetDepthMask(false);
	GraphicsAPI::SetDepthFunc(GraphicsAPI::eTestFuncType::LEQUAL);

	Mesh::ResetBoundMeshCache();

	UINT32 queryIndex = 0;

	for (Renderer* const renderer : renderers)
	{
		if (IsValid(renderer) == false ||
			renderer->GetIsComponentEnabled() == false ||
			renderer->GetIsBatched() == true)
		{
			continue;
		}

		if (bIsCameraCulling && renderer->GetIsCulled(targetCamera->CameraIndexInCullingSystem) != 0)
		{
			continue;
		}

		if (queryIndex >= mVisibilityOracleQueries.size())
		{
			mVisibilityOracleQueries.push_back(GraphicsAPI::CreateQuery(GraphicsAPI::QUERY_OCCLUSION));
		}

		if (mVisibilityOracleQueries[queryIndex] == 0)
		{
			continue;
		}

		GraphicsAPI::BeginQuery(mVisibilityOracleQueries[queryIndex]);
		renderer->Draw();
		GraphicsAPI::EndQuery(mVisibilityOracleQueries[queryIndex]);

		queryIndex++;
	}

	mVisibilityOraclePendingCount = queryIndex;

	FixedMaterial::GetSingleton()->SetFixedMaterial(nullptr);
	GraphicsAPI::SetDepthMask(true);

	// This pass drew the whole scene again. Its binds and indices belong to the
	// measurement, not to the geometry pass being measured.
	Mesh::GetAndResetMeshBindCount();
	Mesh::GetAndResetIndexCount();
	Mesh::ResetBoundMeshCache();
}

void dooms::graphics::DeferredRenderingPipeLine::BeginGpuTimer(GpuTimerRing& gpuTimerRing, FLOAT32& destinationMilliseconds)
{
	if (GraphicsAPI::CreateQuery == nullptr || GraphicsAPI::BeginQuery == nullptr ||
		GraphicsAPI::EndQuery == nullptr || GraphicsAPI::GetQueryResult == nullptr)
	{
		return;
	}

	if (gpuTimerRing.bmAreQueriesCreated == false)
	{
		for (UINT32 frameIndex = 0; frameIndex < GPU_TIMER_FRAME_COUNT; frameIndex++)
		{
			gpuTimerRing.mFrames[frameIndex].mDisjointQuery = GraphicsAPI::CreateQuery(GraphicsAPI::QUERY_TIMESTAMP_DISJOINT);
			gpuTimerRing.mFrames[frameIndex].mStartQuery = GraphicsAPI::CreateQuery(GraphicsAPI::QUERY_TIMESTAMP);
			gpuTimerRing.mFrames[frameIndex].mEndQuery = GraphicsAPI::CreateQuery(GraphicsAPI::QUERY_TIMESTAMP);
		}

		gpuTimerRing.bmAreQueriesCreated = true;
	}

	// Collect the oldest one first. Going round the ring means this is the
	// frame furthest from the one about to be issued, so it has had the most
	// time to finish.
	GpuTimerFrame& oldestTimer = gpuTimerRing.mFrames[(gpuTimerRing.mFrameIndex + 1) % GPU_TIMER_FRAME_COUNT];

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
				destinationMilliseconds =
					static_cast<FLOAT32>(static_cast<double>(endTicks - startTicks) * 1000.0 / static_cast<double>(frequency));
			}

			oldestTimer.bmIsPending = false;
		}
	}

	GpuTimerFrame& currentTimer = gpuTimerRing.mFrames[gpuTimerRing.mFrameIndex];

	if (currentTimer.mDisjointQuery != 0 && currentTimer.bmIsPending == false)
	{
		GraphicsAPI::BeginQuery(currentTimer.mDisjointQuery);
		GraphicsAPI::EndQuery(currentTimer.mStartQuery);
	}
}

void dooms::graphics::DeferredRenderingPipeLine::EndGpuTimer(GpuTimerRing& gpuTimerRing)
{
	if (GraphicsAPI::EndQuery == nullptr || gpuTimerRing.bmAreQueriesCreated == false)
	{
		return;
	}

	GpuTimerFrame& currentTimer = gpuTimerRing.mFrames[gpuTimerRing.mFrameIndex];

	if (currentTimer.mDisjointQuery != 0 && currentTimer.bmIsPending == false)
	{
		GraphicsAPI::EndQuery(currentTimer.mEndQuery);
		GraphicsAPI::EndQuery(currentTimer.mDisjointQuery);

		currentTimer.bmIsPending = true;
	}

	gpuTimerRing.mFrameIndex = (gpuTimerRing.mFrameIndex + 1) % GPU_TIMER_FRAME_COUNT;
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

	BeginGpuTimer(mHiZGpuTimer, graphicsSetting::GpuStatHiZBuildMilliseconds);

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

	EndGpuTimer(mHiZGpuTimer);

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

	// Rebuilt when the resolution is changed, so the effect of testing at a
	// finer granularity can be measured rather than argued about.
	if (mHiZReadbackTexture != 0 && mHiZReadbackTargetWidthInUse != graphicsSetting::HiZReadbackTargetWidth)
	{
		GraphicsAPI::DestroyTextureObject(mHiZReadbackTexture);
		mHiZReadbackTexture = 0;
		bmIsHiZReadbackDataValid = false;
		bmIsHiZReadbackPending = false;
	}

	if (mHiZReadbackTexture == 0)
	{
		// The coarsest level still at least this wide. The pyramid holds the
		// farthest depth per cell, so a cell is only useful for culling when
		// nothing in it is background: coarser cells are cheaper to scan and
		// far more likely to contain a speck of sky that makes them useless.
		mHiZReadbackTargetWidthInUse = graphicsSetting::HiZReadbackTargetWidth;

		mHiZReadbackLevel = 0;
		while ((mHiZReadbackLevel + 1 < mHiZLevelCount) &&
			(mHiZTexture->GetTextureWidth(mHiZReadbackLevel) > static_cast<INT32>(mHiZReadbackTargetWidthInUse)))
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
				// Readback detail was logged here every frame while this was being
				// brought up. It buried the log, including the messages that say
				// how long start up took.
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

			// The probe pushes the tested depth away from the camera, standing
			// in for a proxy whose nearest point is not a box corner sticking
			// out in front of the object.
			const FLOAT32 objectNDCZ =
				entityBlock->mAABBMinNDCZ[entityIndex] + graphicsSetting::HiZProbeDepthPush;

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
			// The probe insets the rectangle towards its centre, standing in for
			// a silhouette that follows the object rather than boxing it.
			FLOAT32 probedMinU = minU;
			FLOAT32 probedMaxU = maxU;
			FLOAT32 probedMinV = minV;
			FLOAT32 probedMaxV = maxV;

			if (graphicsSetting::HiZProbeRectangleShrink > 0.0f)
			{
				const FLOAT32 shrink = math::Min(0.49f, graphicsSetting::HiZProbeRectangleShrink);

				const FLOAT32 insetU = (maxU - minU) * shrink;
				const FLOAT32 insetV = (maxV - minV) * shrink;

				probedMinU += insetU;
				probedMaxU -= insetU;
				probedMinV += insetV;
				probedMaxV -= insetV;
			}

			const INT32 startX = static_cast<INT32>(math::Max(0.0f, probedMinU) * mHiZReadbackWidth) - 1;
			const INT32 endX = static_cast<INT32>(math::Min(1.0f, probedMaxU) * mHiZReadbackWidth) + 1;
			const INT32 startY = static_cast<INT32>(math::Max(0.0f, probedMinV) * mHiZReadbackHeight) - 1;
			const INT32 endY = static_cast<INT32>(math::Min(1.0f, probedMaxV) * mHiZReadbackHeight) + 1;

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
		// Per frame occlusion counts were logged here. The same numbers are on
		// the overlay, which does not cost a formatted string per frame.
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
	ApplyHiZHullOcclusionCulling(targetCamera, cameraIndex);
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
		// Timed separately from the pass it exists to speed up, because the
		// whole question is whether what it saves there is worth what it costs
		// here.
		BeginGpuTimer(mDepthPrePassGpuTimer, graphicsSetting::GpuStatDepthPrePassMilliseconds);
		DrawRenderersWithDepthOnly(targetCamera, cameraIndex);
		EndGpuTimer(mDepthPrePassGpuTimer);
	}
	else
	{
		graphicsSetting::GpuStatDepthPrePassMilliseconds = 0.0f;
	}
	

	{
		D_START_PROFILING(RenderObject, dooms::profiler::eProfileLayers::Rendering);
		BeginGpuTimer(mGeometryPassGpuTimer, graphicsSetting::GpuStatGeometryPassMilliseconds);
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

		EndGpuTimer(mGeometryPassGpuTimer);

		D_END_PROFILING(RenderObject);
	}

	// After the geometry pass, so the depth buffer it tests against is the
	// finished one and a zero result really does mean invisible.
	MeasureTrueVisibility(targetCamera, cameraIndex);

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


