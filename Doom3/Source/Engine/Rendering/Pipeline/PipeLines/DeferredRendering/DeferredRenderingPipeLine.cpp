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

	TextureView* const hiZTextureView = mHiZTexture->GenerateTextureView(0, GraphicsAPI::eGraphicsPipeLineStage::PIXEL_SHADER);

	if (IsValid(hiZTextureView))
	{
		UniformBufferObjectView* const hiZDataView = mHiZDownsampleMaterial->GetUniformBufferObjectViewFromUBOName("HiZData");

		for (UINT32 levelIndex = 1; levelIndex < mHiZLevelCount; levelIndex++)
		{
			const UINT32 sourceLevel = levelIndex - 1;

			mHiZFrameBuffers[levelIndex]->BindFrameBuffer();
			mHiZDownsampleMaterial->BindMaterial();

			if (hiZDataView != nullptr)
			{
				hiZDataView->SetVector4((UINT64)0, math::Vector4(
					static_cast<FLOAT32>(sourceLevel),
					1.0f / static_cast<FLOAT32>(mHiZTexture->GetTextureWidth(sourceLevel)),
					1.0f / static_cast<FLOAT32>(mHiZTexture->GetTextureHeight(sourceLevel)),
					0.0f));
			}

			// Reading the level above while writing the one below. They are
			// different mips of the same texture, which D3D11 allows only
			// because the views do not overlap.
			hiZTextureView->BindTexture(0, GraphicsAPI::eGraphicsPipeLineStage::PIXEL_SHADER);

			DrawHiZQuad();

			hiZTextureView->UnBindTexture(0, GraphicsAPI::eGraphicsPipeLineStage::PIXEL_SHADER);
		}
	}

	GraphicsAPI::SetIsDepthTestEnabled(true);
	GraphicsAPI::SetDepthMask(true);
	FrameBuffer::StaticBindBackFrameBuffer();
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


