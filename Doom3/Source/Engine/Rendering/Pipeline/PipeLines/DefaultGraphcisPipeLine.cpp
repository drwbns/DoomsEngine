#include "DefaultGraphcisPipeLine.h"

#include <chrono>
#include <algorithm>
#include <utility>
#include <Rendering/Renderer/MeshRenderer.h>
#include <Rendering/Buffer/Mesh.h>

#include <Rendering/RenderingDebugger/RenderingDebuggerModules/Modules/OverDrawVisualization.h>

#include <Graphics/graphicsSetting.h>
#include <Rendering/Renderer/RendererStaticIterator.h>
#include <Rendering/Acceleration/FrontToBackSorting/SortFrontToBackSolver.h>
#include <Rendering/Camera.h>
#include <ResourceManagement/Thread/JobPool.h>
#include <Rendering/Renderer/Renderer.h>
#include <EngineGUI/engineGUIServer.h>
#include <Rendering/Graphics_Server.h>
#include <Rendering/Renderer/RendererStaticIterator.h>
#include <Rendering/Batch/BatchRenderingManager.h>

#include "Asset/AssetManager/AssetManager.h"
#include "Graphics/GraphicsAPI/graphicsAPISetting.h"

void dooms::graphics::DefaultGraphcisPipeLine::PreRenderRenderer()
{
	const std::chrono::steady_clock::time_point preRenderStartTime = std::chrono::steady_clock::now();

	const std::vector<Renderer*>& renderersInLayer = RendererComponentStaticIterator::GetSingleton()->GetSortedRendererInLayer();
	for (Renderer* renderer : renderersInLayer)
	{
		renderer->PreRender();
	}

	graphicsSetting::CpuStatPreRenderRendererMilliseconds = static_cast<FLOAT32>(
		std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
			std::chrono::steady_clock::now() - preRenderStartTime).count());
}


dooms::graphics::DefaultGraphcisPipeLine::DefaultGraphcisPipeLine(dooms::graphics::Graphics_Server& graphicsServer)
	:
	GraphicsPipeLine(graphicsServer),
	mRenderingCullingManager(),
	mRenderingDebugger()
{
}

void dooms::graphics::DefaultGraphcisPipeLine::Initialize()
{
	GraphicsPipeLine::Initialize();

	mRenderingCullingManager.Initialize();
	mRenderingDebugger.Initialize();

	auto DepthOnlyShader = assetImporter::AssetManager::GetSingleton()->GetAsset<asset::eAssetType::SHADER>("DepthOnlyShader.glsl");
	DepthOnlyMaterial = dooms::CreateDObject<graphics::Material>(DepthOnlyShader);
}

void dooms::graphics::DefaultGraphcisPipeLine::LateInitialize()
{
	GraphicsPipeLine::LateInitialize();

	mRenderingDebugger.LateInitialize();
}

void dooms::graphics::DefaultGraphcisPipeLine::ApplyPendingResolutionChange()
{
	// Null when running against a graphics DLL from before these entry points.
	if (GraphicsAPI::ConsumePendingResize == nullptr || GraphicsAPI::ResizeSwapChainBuffers == nullptr)
	{
		return;
	}

	unsigned int newWidth = 0;
	unsigned int newHeight = 0;

	if (GraphicsAPI::ConsumePendingResize(&newWidth, &newHeight) == 0)
	{
		return;
	}

	if (GraphicsAPI::ResizeSwapChainBuffers(newWidth, newHeight) == 0)
	{
		D_RELEASE_LOG(eLogType::D_ERROR, "Failed to resize swap chain buffers to %u x %u", newWidth, newHeight);
		return;
	}

	graphicsAPISetting::SetScreenSize(static_cast<INT32>(newWidth), static_cast<INT32>(newHeight));

	// Resized in place rather than recreated: the culling system holds the
	// registered entities, which recreating it would throw away.
	if (mRenderingCullingManager.mCullingSystem != nullptr)
	{
		mRenderingCullingManager.mCullingSystem->SetResolution(newWidth, newHeight);
	}

	// Sized from the screen when it is created, so it has to be dropped and
	// rebuilt too. It is built lazily, so this only costs anything if overdraw
	// visualisation has actually been used.
	if (dooms::graphics::OverDrawVisualization::GetSingleton() != nullptr)
	{
		dooms::graphics::OverDrawVisualization::GetSingleton()->OnResolutionChanged();
	}

	D_RELEASE_LOG(eLogType::D_LOG, "Resolution changed to %u x %u", newWidth, newHeight);
}

void dooms::graphics::DefaultGraphcisPipeLine::PreRender()
{
	ApplyPendingResolutionChange();

	D_START_PROFILING(PreRenderRenderer, dooms::profiler::eProfileLayers::Rendering);
	PreRenderRenderer();
	D_END_PROFILING(PreRenderRenderer);

	D_START_PROFILING(engineGUIServer_PreRender, dooms::profiler::eProfileLayers::Rendering);
	dooms::ui::EngineGUIServer::GetSingleton()->PreRender();
	D_END_PROFILING(engineGUIServer_PreRender);

	mRenderingDebugger.PreRender();


	if (Camera::GetMainCamera()->GetIsCullJobEnabled() == true)
	{
		D_START_PROFILING(PreCullJob, dooms::profiler::eProfileLayers::Rendering);
		mRenderingCullingManager.PreCullJob();
		D_END_PROFILING(PreCullJob);
	}
}

void dooms::graphics::DefaultGraphcisPipeLine::Render()
{
	D_START_PROFILING(Update_Uniform_Buffer, dooms::profiler::eProfileLayers::Rendering);
	mGraphicsServer.mUniformBufferObjectManager.UpdateUniformObjects();
	D_END_PROFILING(Update_Uniform_Buffer);

	const std::vector<dooms::Camera*>& spawnedCameraList = StaticContainer<dooms::Camera>::GetAllStaticComponents();

	for (size_t cameraIndex = 0; cameraIndex < spawnedCameraList.size(); cameraIndex++)
	{
		dooms::Camera* const targetCamera = spawnedCameraList[cameraIndex];
		CameraRender(targetCamera, cameraIndex);
	}

	RendererComponentStaticIterator::GetSingleton()->ChangeWorkingIndexRenderers();
}

void dooms::graphics::DefaultGraphcisPipeLine::PostRender()
{
	D_START_PROFILING(engineGUIServer_Render, dooms::profiler::eProfileLayers::Rendering);
	dooms::ui::EngineGUIServer::GetSingleton()->Render();
	D_END_PROFILING(engineGUIServer_Render);

	D_START_PROFILING(engineGUIServer_PostRender, dooms::profiler::eProfileLayers::Rendering);
	dooms::ui::EngineGUIServer::GetSingleton()->PostRender();
	D_END_PROFILING(engineGUIServer_PostRender);

	mRenderingDebugger.PostRender();

	D_START_PROFILING(SwapBuffer, dooms::profiler::eProfileLayers::Rendering);
	graphics::GraphicsAPI::SwapBuffer();
	D_END_PROFILING(SwapBuffer);

}


void dooms::graphics::DefaultGraphcisPipeLine::PushFrontToBackSortJobToJobSystem
(
	dooms::Camera* const targetCamera, const UINT32 cameraIndex, std::atomic<bool>* bIsFinihsed
)
{
	math::Vector3 cameraPos = targetCamera->GetTransform()->GetPosition();
	auto FrontToBackSortJob = [cameraPos, cameraIndex, bIsFinihsed]()
	{
		FrontToBackSort(cameraPos, cameraIndex);

		if (bIsFinihsed != nullptr)
		{
			*bIsFinihsed = true;
		}
	};

	thread::ParallelForWithReturn(FrontToBackSortJob, 1);
}

void dooms::graphics::DefaultGraphcisPipeLine::FrontToBackSort(const math::Vector3& CameraPos, const UINT32 cameraIndex)
{
	std::vector<Renderer*>& renderers = dooms::RendererComponentStaticIterator::GetSingleton()->GetSortingRendererInLayer();

	const size_t startRendererIndex = 0;
	const size_t rendererCount = renderers.size();

	for(
		size_t rendererIndex = startRendererIndex;
		rendererIndex < rendererCount;
		rendererIndex++
	)
	{
		D_ASSERT(IsValid(renderers[rendererIndex]));
		renderers[rendererIndex]->CacheDistanceToCamera(cameraIndex, CameraPos);
	}

	dooms::graphics::SortFrontToBackSolver::SortRenderer(cameraIndex);
}

void dooms::graphics::DefaultGraphcisPipeLine::DrawRenderersWithDepthOnly
(
	dooms::Camera* const targetCamera,
	const size_t cameraIndex
) const
{
	if
	(
		dooms::graphics::graphicsAPISetting::DepthPrePassType == dooms::graphics::eDepthPrePassType::AllOpaque //||
		//dooms::graphics::graphicsAPISetting::DepthPrePassType == dooms::graphics::eDepthPrePassType::ConsiderBound 
	)
	{
		D_START_PROFILING(RenderObject_DepthPrePass, dooms::profiler::eProfileLayers::Rendering);

		dooms::graphics::FixedMaterial::GetSingleton()->SetFixedMaterial(GetDepthOnlyMaterial());
		GraphicsAPI::SetIsDepthTestEnabled(true);
		GraphicsAPI::SetDepthMask(true);
		GraphicsAPI::SetDepthFunc(GraphicsAPI::eTestFuncType::LESS);

		if(dooms::graphics::graphicsAPISetting::DepthPrePassType == dooms::graphics::eDepthPrePassType::AllOpaque)
		{
			DrawRenderers(targetCamera, cameraIndex);
		}
		/*
		else if(dooms::graphics::graphicsAPISetting::DepthPrePassType == dooms::graphics::eDepthPrePassType::ConsiderBound)
		{
			ConditionalDrawRenderers
			(
				targetCamera,
				cameraIndex,
				[](const dooms::Renderer* const Renderer) -> bool
				{
					bool bIsDrawable = false;

					D_ASSERT(Renderer->CheckIsWorldColliderCacheDirty() == false);
					if(const dooms::physics::AABB3D* const AABB = Renderer->GetWorldColliderWithoutUpdate())
					{
						AABB->
					}
				}
			);
		}
		*/

		dooms::graphics::FixedMaterial::GetSingleton()->SetFixedMaterial(nullptr);

		D_END_PROFILING(RenderObject_DepthPrePass);
	}
}


void dooms::graphics::DefaultGraphcisPipeLine::DrawRenderers(dooms::Camera* const targetCamera, const size_t cameraIndex) const
{
	ConditionalDrawRenderers(targetCamera, cameraIndex, nullptr);	
}

void dooms::graphics::DefaultGraphcisPipeLine::ConditionalDrawRenderers
(
	dooms::Camera* const targetCamera,
	const size_t cameraIndex,
	const std::function<bool(const dooms::Renderer* const)> ConditionFunc
) const
{
	D_ASSERT(IsValid(targetCamera) == true);

	{
		D_START_PROFILING(DrawLoop, dooms::profiler::eProfileLayers::Rendering);
		const bool targetCamera_IS_CULLED_flag_on = targetCamera->GetCameraFlag(dooms::eCameraFlag::IS_CULLED);

		const bool IsExistCondtionFunc = static_cast<bool>(ConditionFunc);

		// Nothing outside this loop can be trusted to have left the geometry
		// binding where we last saw it.
		Mesh::ResetBoundMeshCache();

		// Gathered first rather than drawn as they are found, so the order can
		// be chosen. Static because this runs every frame and the size barely
		// changes; clearing keeps the capacity.
		static std::vector<Renderer*> visibleRenderers;
		visibleRenderers.clear();

		const std::vector<Renderer*>& renderersInLayer = RendererComponentStaticIterator::GetSingleton()->GetSortedRendererInLayer();
		for (Renderer* renderer : renderersInLayer)
		{
			if
			(
				IsValid(renderer) == true &&
				renderer->GetIsComponentEnabled() == true &&
				renderer->GetIsBatched() == false &&
				(IsExistCondtionFunc ? ConditionFunc(renderer) : true)
			)
			{
				if
				(
					targetCamera_IS_CULLED_flag_on == false ||
					renderer->GetIsCulled(targetCamera->CameraIndexInCullingSystem) == false
				)
				{
					visibleRenderers.push_back(renderer);
				}
			}
		}

		// Which mesh with which material. Two objects sharing both can be drawn
		// back to back without rebinding anything between them, and could one
		// day be a single instanced draw. Two that do not, never can.
		const auto GetDrawStateKey = [](const Renderer* const renderer)
		{
			const dooms::MeshRenderer* const meshRenderer = dooms::CastTo<const dooms::MeshRenderer*>(renderer);

			return std::pair<const void*, const void*>(
				IsValid(meshRenderer) ? static_cast<const void*>(meshRenderer->GetMesh()) : nullptr,
				static_cast<const void*>(renderer->GetMaterial()));
		};

		if (graphicsSetting::IsGroupDrawsByStateEnabled == true)
		{
			// Stable, so objects keep their front to back order within a group
			// and only the ordering between groups is given up.
			std::stable_sort(visibleRenderers.begin(), visibleRenderers.end(),
				[&GetDrawStateKey](const Renderer* const left, const Renderer* const right)
				{
					return GetDrawStateKey(left) < GetDrawStateKey(right);
				});
		}

		for (Renderer* const renderer : visibleRenderers)
		{
			renderer->Draw();
		}

		// Only for the pass that shades, so a depth pre pass does not report
		// over the top of the numbers for the pass it precedes.
		if (dooms::graphics::FixedMaterial::GetSingleton()->GetFixedMaterial() == nullptr)
		{
			static std::vector<std::pair<const void*, const void*>> drawStateKeys;
			drawStateKeys.clear();
			drawStateKeys.reserve(visibleRenderers.size());

			unsigned long long idealIndexCount = 0;

			for (const Renderer* const renderer : visibleRenderers)
			{
				drawStateKeys.push_back(GetDrawStateKey(renderer));

				// One triangle per pixel covered is the point past which extra
				// geometry cannot be seen. Hardware rasterises in 2x2 quads, so
				// triangles smaller than a pixel are largely wasted shading, and
				// this scene submits about eight triangles for every pixel.
				const dooms::MeshRenderer* const meshRenderer = dooms::CastTo<const dooms::MeshRenderer*>(renderer);
				if (IsValid(meshRenderer) == false || IsValid(meshRenderer->GetMesh()) == false)
				{
					continue;
				}

				const unsigned long long actualIndexCount = meshRenderer->GetMesh()->GetNumOfIndices();

				const culling::EntityBlockViewer& viewer = renderer->mCullingEntityBlockViewer;
				if (viewer.IsValid() == false)
				{
					idealIndexCount += actualIndexCount;
					continue;
				}

				const culling::EntityBlock* const entityBlock = viewer.GetTargetEntityBlock();
				const UINT32 entityIndex = viewer.GetEntityIndexInBlock();

				const FLOAT32 screenWidth =
					entityBlock->mAABBMaxScreenSpacePointX[entityIndex] - entityBlock->mAABBMinScreenSpacePointX[entityIndex];
				const FLOAT32 screenHeight =
					entityBlock->mAABBMaxScreenSpacePointY[entityIndex] - entityBlock->mAABBMinScreenSpacePointY[entityIndex];

				const FLOAT32 coveredPixels = math::Max(0.0f, screenWidth) * math::Max(0.0f, screenHeight);

				// Three indices to a triangle, and never more than the mesh has.
				const unsigned long long affordableIndexCount = static_cast<unsigned long long>(coveredPixels) * 3ull;

				idealIndexCount += (affordableIndexCount < actualIndexCount) ? affordableIndexCount : actualIndexCount;
			}

			graphicsSetting::CullStatIdealIndexCount = idealIndexCount;

			std::sort(drawStateKeys.begin(), drawStateKeys.end());

			graphicsSetting::CullStatDrawnRendererCount = static_cast<unsigned int>(drawStateKeys.size());
			graphicsSetting::CullStatDrawGroupCount = static_cast<unsigned int>(
				std::unique(drawStateKeys.begin(), drawStateKeys.end()) - drawStateKeys.begin());
			graphicsSetting::CullStatMeshBindCount = Mesh::GetAndResetMeshBindCount();
			graphicsSetting::CullStatIndexCount = Mesh::GetAndResetIndexCount();
		}

		D_END_PROFILING(DrawLoop);
	}
}

void dooms::graphics::DefaultGraphcisPipeLine::DrawBatchedRenderers() const
{
	D_START_PROFILING(DrawBatchedRenderers, dooms::profiler::eProfileLayers::Rendering);
	D_ASSERT(IsValid(BatchRenderingManager::GetSingleton()));
	BatchRenderingManager::GetSingleton()->DrawAllBatchedRendererContainers();
	D_END_PROFILING(DrawBatchedRenderers);
}

dooms::graphics::Material* dooms::graphics::DefaultGraphcisPipeLine::GetDepthOnlyMaterial() const
{
	D_ASSERT(IsValid(DepthOnlyMaterial));
	return DepthOnlyMaterial;
}
