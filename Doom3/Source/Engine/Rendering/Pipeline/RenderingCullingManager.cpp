#include "RenderingCullingManager.h"

#include "EngineConfigurationData/ConfigData.h"
#include "Graphics/GraphicsAPI/graphicsAPISetting.h"
#include <ResourceManagement/Thread/JobPool.h>
#include <Rendering/Culling/EveryCulling/CullingModule/MaskedSWOcclusionCulling/MaskedSWOcclusionCulling.h>
#include <Rendering/Camera.h>
#include <EngineGUI/GUIModules/Modules/MaskedOcclusionCulliingDebugger.h>

#include "ResourceManagement/Thread/RunnableThread/RunnableThread.h"

dooms::graphics::RenderingCullingManager::RenderingCullingManager()
	: mCullingSystem()
{
}

void dooms::graphics::RenderingCullingManager::Initialize()
{
	mCullingSystem = std::make_unique<culling::EveryCulling>(graphicsAPISetting::GetScreenWidth(), graphicsAPISetting::GetScreenHeight());
	mCullingSystem->mMaskedSWOcclusionCulling->mSolveMeshRoleStage.mOccluderAABBScreenSpaceMinArea = ConfigData::GetSingleton()->GetConfigData().GetValue<FLOAT64>("Graphics", "MASKED_OC_OCCLUDER_AABB_SCREEN_SPACE_MIN_AREA");

	if(dooms::ui::MaskedOcclusionCulliingDebugger::GetSingleton() != nullptr)
	{
		dooms::ui::MaskedOcclusionCulliingDebugger::GetSingleton()->SetMaskedSWOcclusionCulling(mCullingSystem->mMaskedSWOcclusionCulling.get());
	}

}

void dooms::graphics::RenderingCullingManager::PreCullJob()
{
	mCullingCameraCount = 0;

	D_START_PROFILING(CullingSystemPreCullJob, dooms::profiler::eProfileLayers::Rendering);
	mCullingSystem->PreCullJob();
	D_END_PROFILING(CullingSystemPreCullJob);

	D_START_PROFILING(UpdateCameraIndexInCullingSystemOfCameraComponent, dooms::profiler::eProfileLayers::Rendering);
	UpdateCameraIndexInCullingSystemOfCameraComponent();
	D_END_PROFILING(UpdateCameraIndexInCullingSystemOfCameraComponent);
}

void dooms::graphics::RenderingCullingManager::UpdateCameraIndexInCullingSystemOfCameraComponent()
{
	const std::vector<dooms::Camera*>& spawnedCameraList = StaticContainer<dooms::Camera>::GetAllStaticComponents();
	for (size_t cameraIndex = 0; cameraIndex < spawnedCameraList.size(); cameraIndex++)
	{
		dooms::Camera* const targetCamera = spawnedCameraList[cameraIndex];

		if (targetCamera->GetIsCullJobEnabled() == true)
		{
			targetCamera->CameraIndexInCullingSystem = mCullingCameraCount;

			mCullingCameraCount++;
		}
	}

	mCullingSystem->SetCameraCount(mCullingCameraCount);
}


void dooms::graphics::RenderingCullingManager::CameraCullJob(dooms::Camera* const camera)
{
	if (camera->GetIsCullJobEnabled() == true)
	{
		std::atomic_thread_fence(std::memory_order_acquire);

		culling::EveryCulling::GlobalDataForCullJob cullingSettingParameters;
		std::memcpy(cullingSettingParameters.mViewProjectionMatrix.data(), camera->GetViewProjectionMatrix(true).data(), sizeof(culling::Mat4x4));
		cullingSettingParameters.mFieldOfViewInDegree = camera->GetFieldOfViewInDegree();
		cullingSettingParameters.mCameraFarPlaneDistance = camera->GetClippingPlaneFar();
		cullingSettingParameters.mCameraNearPlaneDistance = camera->GetClippingPlaneNear();
		std::memcpy(cullingSettingParameters.mCameraWorldPosition.data(), camera->GetTransform()->GetPosition().data(), sizeof(culling::Vec3));
		std::memcpy(cullingSettingParameters.mCameraRotation.data(), camera->GetTransform()->GetRotation().data(), sizeof(culling::Vec4));

		mCullingSystem->UpdateGlobalDataForCullJob(camera->CameraIndexInCullingSystem, cullingSettingParameters);

		std::atomic_thread_fence(std::memory_order_release);

		const UINT64 TickCount = mCullingSystem.get()->GetTickCount();
		auto Job = [cullingSystem = mCullingSystem.get(), cameraIndexInCullingSystem = camera->CameraIndexInCullingSystem, TickCount]()
		{
			cullingSystem->ThreadCullJob(cameraIndexInCullingSystem, TickCount);
		};

		auto ReturnedFutures = dooms::thread::ParallelForWithReturn(Job);
		D_START_PROFILING(CameraCullJob, dooms::profiler::eProfileLayers::Rendering);
		mCullingSystem->ThreadCullJob(camera->CameraIndexInCullingSystem, TickCount);
		D_END_PROFILING(CameraCullJob);

		D_START_PROFILING(WaitSubThreadsCullJob, dooms::profiler::eProfileLayers::Rendering);
		for (auto& future : ReturnedFutures)
		{
			future.wait();
		}
		D_END_PROFILING(WaitSubThreadsCullJob);

	}
}