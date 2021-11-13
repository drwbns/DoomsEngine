﻿#include "Graphics_Server.h"

#include <functional>

#include "../Game/GameCore.h"
#include "../Scene/Layer.h"
#include "../Game/AssetManager/AssetManager.h"

#include "GraphicsAPI.h"

#include "Buffer/UniformBufferObjectManager.h"

#include <Rendering/Renderer/Renderer.h>
#include "FrameBuffer/DefferedRenderingFrameBuffer.h"
#include <Rendering/Renderer/RendererStaticIterator.h>

#include "Rendering/Camera.h"

#include "Acceleration/LinearData_ViewFrustumCulling/EveryCulling.h"

#include "graphicsAPIManager.h"
#include "Graphics_Setting.h"
#include "MainTimer.h"

#include "Acceleration/SortFrontToBackSolver.h"
#include "DebugGraphics/OverDrawVisualization.h"

//#define D_DEBUG_CPU_VENDOR_PROFILER

using namespace dooms::graphics;

void Graphics_Server::Init()
{
	Graphics_Setting::LoadData();
	graphicsAPIManager::Initialize();

	mCullingSystem = std::make_unique<culling::EveryCulling>(Graphics_Setting::GetScreenWidth(), Graphics_Setting::GetScreenHeight());

	return;
}

void dooms::graphics::Graphics_Server::LateInit()
{
#ifdef DEBUG_DRAWER
	mDebugGraphics.Init();
#endif 

	SetRenderingMode(Graphics_Server::eRenderingMode::DeferredRendering);


	//mQueryOcclusionCulling.InitQueryOcclusionCulling();
	//mCullDistance.Initialize();
}



void Graphics_Server::Update()
{		
#ifdef DEBUG_DRAWER
	mDebugGraphics.Update();
	mRenderingDebugger.DrawRenderingBoundingBox();
#endif

	Renderder_UpdateComponent();

#ifdef DEBUG_DRAWER
	mRenderingDebugger.UpdateInputForPrintDrawCallCounter();
#endif

	//auto t_start = std::chrono::high_resolution_clock::now();
	
	D_START_PROFILING_IN_RELEASE(RENDER);
	Render();
	D_END_PROFILING_IN_RELEASE(RENDER);

	//Render();

	//auto t_end = std::chrono::high_resolution_clock::now();
	//FLOAT64 elapsed_time_ms = std::chrono::duration<FLOAT64, std::milli>(t_end - t_start).count();
	//dooms::ui::PrintText("elapsed tick count : %lf", elapsed_time_ms);
}

void Graphics_Server::OnEndOfFrame()
{
	Renderder_OnEndOfFrameComponent();

#ifdef DEBUG_DRAWER
	mDebugGraphics.Reset();
#endif

	graphicsAPIManager::SwapBuffer();
}

void dooms::graphics::Graphics_Server::Renderder_InitComponent()
{
	for (UINT32 layerIndex = 0; layerIndex < MAX_LAYER_COUNT; layerIndex++)
	{
		const std::vector<Renderer*>& renderersInLayer = RendererComponentStaticIterator::GetSingleton()->GetWorkingRendererInLayer(0, layerIndex);
		for (size_t rendererIndex = 0; rendererIndex < renderersInLayer.size(); rendererIndex++)
		{
			//renderersInLayer[rendererIndex]->InitComponent_Internal();
			renderersInLayer[rendererIndex]->InitComponent();
		}
	}
}

void dooms::graphics::Graphics_Server::Renderder_UpdateComponent()
{
	for (UINT32 layerIndex = 0; layerIndex < MAX_LAYER_COUNT; layerIndex++)
	{
		const std::vector<Renderer*>& renderersInLayer = RendererComponentStaticIterator::GetSingleton()->GetWorkingRendererInLayer(0, layerIndex);
		for (size_t rendererIndex = 0; rendererIndex < renderersInLayer.size(); rendererIndex++)
		{
			renderersInLayer[rendererIndex]->UpdateComponent_Internal();
			renderersInLayer[rendererIndex]->UpdateComponent();
		}
	}
}

void dooms::graphics::Graphics_Server::Renderder_OnEndOfFrameComponent()
{
	for (UINT32 layerIndex = 0; layerIndex < MAX_LAYER_COUNT; layerIndex++)
	{
		const std::vector<Renderer*>& renderersInLayer = RendererComponentStaticIterator::GetSingleton()->GetWorkingRendererInLayer(0, layerIndex);
		for (size_t rendererIndex = 0; rendererIndex < renderersInLayer.size(); rendererIndex++)
		{
			renderersInLayer[rendererIndex]->OnEndOfFrame_Component_Internal();
			renderersInLayer[rendererIndex]->OnEndOfFrame_Component();
		}
	}
}



Graphics_Server::Graphics_Server()
{

}

Graphics_Server::~Graphics_Server()
{
	graphicsAPIManager::DeInitialize();
}


void Graphics_Server::DoCullJob()
{
	const std::vector<dooms::Camera*>& spawnedCameraList = StaticContainer<dooms::Camera>::GetAllStaticComponents();

	UINT32 CullJobAvailiableCameraCount = 0;
	for (size_t cameraIndex = 0; cameraIndex < spawnedCameraList.size(); cameraIndex++)
	{
		dooms::Camera* const camera = spawnedCameraList[cameraIndex];

		if (camera->GetIsCullJobEnabled() == true)
		{
			camera->CameraIndexInCullingSystem = CullJobAvailiableCameraCount;

			mCullingSystem->SetViewProjectionMatrix(CullJobAvailiableCameraCount, *reinterpret_cast<const culling::Mat4x4*>(&(camera->GetViewProjectionMatrix())));
			mCullingSystem->SetCameraPosition(
				CullJobAvailiableCameraCount,
				*reinterpret_cast<const culling::Vec3*>(&(camera->GetTransform()->GetPosition()))
			);

			CullJobAvailiableCameraCount++;
		}
	
	}

	if (CullJobAvailiableCameraCount > 0)
	{
		mCullingSystem->SetCameraCount(CullJobAvailiableCameraCount);

		D_START_PROFILING(mFrotbiteCullingSystem_ResetCullJobStat, dooms::profiler::eProfileLayers::Rendering);
		mCullingSystem->ResetCullJobState();
		D_END_PROFILING(mFrotbiteCullingSystem_ResetCullJobStat);

		D_START_PROFILING(Push_Culling_Job_To_Linera_Culling_System, dooms::profiler::eProfileLayers::Rendering);
		resource::JobSystem::GetSingleton()->PushBackJobToAllThreadWithNoSTDFuture(std::function<void()>(mCullingSystem->GetCullJobInLambda()));
		D_END_PROFILING(Push_Culling_Job_To_Linera_Culling_System);
	}
	
}

void dooms::graphics::Graphics_Server::Render()
{
	DoCullJob(); // do this first
	//TODO : Think where put this, as early as good
	
	D_START_PROFILING(Update_Uniform_Buffer, dooms::profiler::eProfileLayers::Rendering);
	mUniformBufferObjectManager.UpdateUniformObjects();
	D_END_PROFILING(Update_Uniform_Buffer);
	
	const std::vector<dooms::Camera*>& spawnedCameraList = StaticContainer<dooms::Camera>::GetAllStaticComponents();

	FrameBuffer::UnBindFrameBuffer();
	//Clear ScreenBuffer
	GraphicsAPI::DefaultClearColor(Graphics_Setting::DefaultClearColor);
	GraphicsAPI::Clear(GraphicsAPI::eClearMask::COLOR_BUFFER_BIT, GraphicsAPI::eClearMask::DEPTH_BUFFER_BIT);

	for (size_t cameraIndex = 0 ; cameraIndex < spawnedCameraList.size() ; cameraIndex++)
	{
		dooms::Camera* const targetCamera = spawnedCameraList[cameraIndex];
		targetCamera->UpdateUniformBufferObject();

		targetCamera->mDefferedRenderingFrameBuffer.ClearFrameBuffer();
		targetCamera->mDefferedRenderingFrameBuffer.BindFrameBuffer();
		
		D_START_PROFILING(RenderObject, dooms::profiler::eProfileLayers::Rendering);
		//GraphicsAPI::Enable(GraphicsAPI::eCapability::DEPTH_TEST);
		RenderObject(targetCamera, cameraIndex);
		D_END_PROFILING(RenderObject);

		targetCamera->mDefferedRenderingFrameBuffer.UnBindFrameBuffer();
	
		// 
		//Blit DepthBuffer To ScreenBuffer

		if (targetCamera->IsMainCamera() == true)
		{
			//Only Main Camera can draw to screen buffer

			UpdateOverDrawVisualization(targetCamera, cameraIndex);


			targetCamera->mDefferedRenderingFrameBuffer.BlitDepthBufferToScreenBuffer();

			mPIPManager.DrawPIPs();

			targetCamera->mDefferedRenderingFrameBuffer.BindGBufferTextures();
			
			
			mDeferredRenderingDrawer.DrawDeferredRenderingQuadDrawer();
			

#ifdef DEBUG_DRAWER
			//�̰� ������ �������. �׳� ����� ����.
			mDebugGraphics.BufferVertexDataToGPU();

		
			mDebugGraphics.Draw();
#endif
		}
		
	}

	RendererComponentStaticIterator::GetSingleton()->ChangeWorkingIndexRenderers();

#ifdef DEBUG_DRAWER
	mDebugGraphics.SetIsVertexDataSendToGPUAtCurrentFrame(false);
#endif


	
	
}

void dooms::graphics::Graphics_Server::UpdateOverDrawVisualization(dooms::Camera* const targetCamera, const size_t cameraIndex)
{

#ifdef DEBUG_DRAWER
	OverDrawVisualization::ShowOverDrawVisualizationPIP(Graphics_Setting::IsOverDrawVisualizationEnabled);
	if (Graphics_Setting::IsOverDrawVisualizationEnabled == true)
	{
		OverDrawVisualization::SetOverDrawVisualizationRenderingState(true);
		RenderObject(targetCamera, cameraIndex);
		OverDrawVisualization::SetOverDrawVisualizationRenderingState(false);
	}
#endif

}



void dooms::graphics::Graphics_Server::RenderObject(dooms::Camera* const targetCamera, const size_t cameraIndex)
{
	targetCamera->UpdateUniformBufferObject();



	std::future<void> IsFinishedSortingReferernceRenderers;

	if (targetCamera->GetIsCullJobEnabled() == true)
	{
		D_START_PROFILING_IN_RELEASE(WAIT_CULLJOB);
		mCullingSystem->WaitToFinishCullJob(targetCamera->CameraIndexInCullingSystem); // Waiting time is almost zero
		//resource::JobSystem::GetSingleton()->SetMemoryBarrierOnAllSubThreads();
		D_END_PROFILING_IN_RELEASE(WAIT_CULLJOB);

		if (Graphics_Setting::IsSortObjectFrontToBack == true)
		{
			//Push Multithread Sorting Renderer Front To Back  TO  JobSystem.
			IsFinishedSortingReferernceRenderers = resource::JobSystem::GetSingleton()->PushBackJobToPriorityQueue(std::function<void()>(dooms::graphics::SortFrontToBackSolver::GetSortRendererLambda(cameraIndex)));
		}
	}


	const bool targetCamera_IS_CULLED_flag_on = targetCamera->GetCameraFlag(dooms::eCameraFlag::IS_CULLED);
	for (UINT32 layerIndex = 0; layerIndex < MAX_LAYER_COUNT; layerIndex++)
	{
		const std::vector<Renderer*>& renderersInLayer = RendererComponentStaticIterator::GetSingleton()->GetWorkingRendererInLayer(cameraIndex, layerIndex);
		for (Renderer* renderer : renderersInLayer)
		{
			if (
				targetCamera_IS_CULLED_flag_on == false ||
				renderer->GetIsCulled(targetCamera->CameraIndexInCullingSystem) == false
				)
			{
				//renderer->ColliderUpdater<dooms::physics::AABB3D>::GetWorldCollider()->DrawPhysicsDebugColor(eColor::Blue);
				renderer->Draw();
			}
		}
	}


	//Wait Multithread Sorting Renderer Front To Back  TO  JobSystem finished.
	if(IsFinishedSortingReferernceRenderers.valid() == true)
	{
		D_START_PROFILING_IN_RELEASE(WAIT_SORTING_RENDERER_JOB);
		IsFinishedSortingReferernceRenderers.wait();
		D_END_PROFILING_IN_RELEASE(WAIT_SORTING_RENDERER_JOB);
	}
	
}








void dooms::graphics::Graphics_Server::SetRenderingMode(eRenderingMode renderingMode)
{
	mCurrentRenderingMode = renderingMode;
	if (mCurrentRenderingMode == eRenderingMode::DeferredRendering)
	{
		mDeferredRenderingDrawer.Initialize();
	}
}



