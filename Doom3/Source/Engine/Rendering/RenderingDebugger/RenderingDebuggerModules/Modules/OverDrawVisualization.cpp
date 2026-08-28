#include "OverDrawVisualization.h"

#include <Graphics/Graphics_Core.h>

#include <Graphics/GraphicsAPI/graphicsAPISetting.h>
#include <Rendering/Graphics_Server.h>
#include <Asset/AssetManager/AssetManager.h>
#include <Asset/ShaderAsset.h>
#include <Rendering/Material/FixedMaterial.h>
#include <Rendering/Material/Material.h>
#include <Rendering/FrameBuffer/FrameBuffer.h>



void dooms::graphics::OverDrawVisualization::ShowOverDrawVisualizationPIP(const bool isPIPDrawed)
{
	if (OverDrawVisualizationPIP != nullptr)
	{
		OverDrawVisualizationPIP->bmIsDrawOnScreen = isPIPDrawed;
	}
	
}

void dooms::graphics::OverDrawVisualization::OnResolutionChanged()
{
	if (bmIsOverDrawVisualizationInitialized == false)
	{
		// Never built, so there is nothing sized to the old resolution.
		return;
	}

	if (IsValid(OverDrawVisualizationPIP))
	{
		dooms::graphics::Graphics_Server::GetSingleton()->mPIPManager.RemovePIP(OverDrawVisualizationPIP);
		OverDrawVisualizationPIP->SetIsPendingKill();
	}
	OverDrawVisualizationPIP = nullptr;

	if (IsValid(mOverDrawVisualizationFrameBuffer))
	{
		mOverDrawVisualizationFrameBuffer->SetIsPendingKill();
	}
	mOverDrawVisualizationFrameBuffer = nullptr;

	// Initialize is called lazily the next time overdraw visualisation is used,
	// and will size everything to the new resolution.
	bmIsOverDrawVisualizationInitialized = false;
}

void dooms::graphics::OverDrawVisualization::Initialize()
{
	// Deliberately empty. The module is always created, but its frame buffer is
	// screen sized and overdraw is off almost all of the time, so the resources
	// are built on first use by EnsureResourcesCreated instead.
}

void dooms::graphics::OverDrawVisualization::EnsureResourcesCreated()
{
	if (bmIsOverDrawVisualizationInitialized == true)
	{
		return;
	}

	dooms::asset::ShaderAsset* overDrawVisualizationShader = dooms::assetImporter::AssetManager::GetSingleton()->GetAsset<dooms::asset::eAssetType::SHADER>("OverDrawVisualizationShader.glsl");
	mOverDrawVisualizationObjectDrawMaterial = overDrawVisualizationShader->CreateMatrialWithThisShaderAsset();
	mOverDrawVisualizationObjectDrawMaterial->AddToRootObjectList();
	D_ASSERT(IsValid(mOverDrawVisualizationObjectDrawMaterial));

	mOverDrawVisualizationFrameBuffer = dooms::CreateDObject<dooms::graphics::FrameBuffer>();
	mOverDrawVisualizationFrameBuffer->AttachColorTextureToFrameBuffer(0, graphicsAPISetting::GetScreenWidth(), graphicsAPISetting::GetScreenHeight());
	mOverDrawVisualizationFrameBuffer->AttachDepthTextureToFrameBuffer(graphicsAPISetting::GetScreenWidth(), graphicsAPISetting::GetScreenHeight());
	mOverDrawVisualizationFrameBuffer->AddToRootObjectList();

	OverDrawVisualizationPIP = dooms::graphics::Graphics_Server::GetSingleton()->mPIPManager.AddNewPIP(
		math::Vector2(-1.0f, -1.0f),
		math::Vector2(1.0f, 1.0f),
		mOverDrawVisualizationFrameBuffer->GetColorTextureView(0, GraphicsAPI::PIXEL_SHADER)
	);
	OverDrawVisualizationPIP->AddToRootObjectList();
	OverDrawVisualizationPIP->bmIsDrawOnScreen = false;

	// The geometry pass accumulates a layer count in the red channel, which on
	// its own presents as a black-to-red wash with no cold end. This material
	// turns that count into the same cold-to-hot ramp the occlusion heatmap
	// uses, so the two visualisations read the same way.
	dooms::asset::ShaderAsset* const overDrawVisualizationPresentShader
		= dooms::assetImporter::AssetManager::GetSingleton()->GetAsset<dooms::asset::eAssetType::SHADER>("OverDrawVisualizationPresentShader.glsl");
	if (IsValid(overDrawVisualizationPresentShader))
	{
		mOverDrawVisualizationPresentMaterial = overDrawVisualizationPresentShader->CreateMatrialWithThisShaderAsset();
		mOverDrawVisualizationPresentMaterial->AddToRootObjectList();
		OverDrawVisualizationPIP->SetMaterial(mOverDrawVisualizationPresentMaterial);
	}
	else
	{
		// Missing asset is survivable: the PIP keeps its default material and
		// shows the raw accumulation instead of the ramp.
		D_ASSERT(false);
	}

	bmIsOverDrawVisualizationInitialized = true;
}

void dooms::graphics::OverDrawVisualization::PreRender()
{
}

void dooms::graphics::OverDrawVisualization::Render(dooms::Camera* const targetCamera)
{
}

void dooms::graphics::OverDrawVisualization::PostRender()
{
}

const char* dooms::graphics::OverDrawVisualization::GetRenderingDebuggerModuleName()
{
	return "OverDrawVisualization";
}


void dooms::graphics::OverDrawVisualization::BeginOverDrawPass()
{
	SetOverDrawVisualizationRenderingState(true);

	// Every layer counts, including the ones a depth test would have thrown
	// away. Overdraw is the number of times a pixel was shaded, not the number
	// of times it was shaded visibly.
	GraphicsAPI::SetIsDepthTestEnabled(false);
	GraphicsAPI::SetDepthMask(false);
}

void dooms::graphics::OverDrawVisualization::EndOverDrawPass()
{
	GraphicsAPI::SetIsDepthTestEnabled(true);
	GraphicsAPI::SetDepthMask(true);

	SetOverDrawVisualizationRenderingState(false);

	ShowOverDrawVisualizationPIP(true);
}

void dooms::graphics::OverDrawVisualization::HideOverDrawVisualization()
{
	if (bmIsOverDrawVisualizationInitialized == true)
	{
		ShowOverDrawVisualizationPIP(false);
	}
}

void dooms::graphics::OverDrawVisualization::SetOverDrawVisualizationRenderingState(const bool isSet)
{
	EnsureResourcesCreated();

	if (isSet == true)
	{
		GraphicsAPI::SetIsBlendEnabled(true);
		GraphicsAPI::SetBlendFactor(GraphicsAPI::eBlendFactor::ONE, GraphicsAPI::eBlendFactor::ONE);

		D_ASSERT(IsValid(mOverDrawVisualizationObjectDrawMaterial));
		dooms::graphics::FixedMaterial::GetSingleton()->SetFixedMaterial(mOverDrawVisualizationObjectDrawMaterial);

		mOverDrawVisualizationFrameBuffer->ClearColorTexture(0, 0.0f, 0.0f, 0.0f, 1.0f);
		mOverDrawVisualizationFrameBuffer->ClrearDepthTexture(GraphicsAPI::DEFAULT_MAX_DEPTH_VALUE);

		// Without this the pass accumulates into whichever frame buffer happened
		// to be bound, which is the back buffer during a normal frame.
		mOverDrawVisualizationFrameBuffer->BindFrameBuffer();
	}
	else
	{
		GraphicsAPI::SetIsBlendEnabled(graphicsAPISetting::DefaultIsBlendOn);
		GraphicsAPI::SetBlendFactor(graphics::graphicsAPISetting::DefaultBlendSourceFactor, graphics::graphicsAPISetting::DefaultBlendDestinationFactor);

		

		const dooms::graphics::Material* const currentFixedMaterial = dooms::graphics::FixedMaterial::GetSingleton()->GetFixedMaterial();
		D_ASSERT(IsValid(mOverDrawVisualizationObjectDrawMaterial));
		if (currentFixedMaterial == mOverDrawVisualizationObjectDrawMaterial)
		{
			dooms::graphics::FixedMaterial::GetSingleton()->ClearFixedMaterial();
		}
		FrameBuffer::StaticBindBackFrameBuffer();
	}
}


