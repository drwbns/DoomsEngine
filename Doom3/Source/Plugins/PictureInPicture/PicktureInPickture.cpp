#include "PicktureInPickture.h"

#include <Asset/AssetManager/AssetManager.h>
#include <Rendering/Texture/TextureView.h>
#include <Rendering/Buffer/MeshHelper.h>

dooms::graphics::Material* dooms::graphics::PicktureInPickture::mDefualtPIPMaterial{nullptr};

void dooms::graphics::PicktureInPickture::InitializeDefaultPIPMaterial()
{
	if(IsValid(mDefualtPIPMaterial) == false)
	{
		mDefualtPIPMaterial = dooms::CreateDObject<dooms::graphics::Material>();
	}

	if (PicktureInPickture::mDefualtPIPMaterial->IsMaterialCreated() == false)
	{
		dooms::asset::ShaderAsset* const pipMaterial = dooms::assetImporter::AssetManager::GetSingleton()->GetAsset<asset::eAssetType::SHADER>("Default2DTextureShader.glsl");
		PicktureInPickture::mDefualtPIPMaterial->SetShaderAsset(pipMaterial);
	}

	
}

dooms::graphics::PicktureInPickture::PicktureInPickture()
	: mPlaneMesh{ nullptr }, mDrawedTexture(nullptr), mPIPMaterial(nullptr), bmIsDrawOnScreen(false)
{
}

void dooms::graphics::PicktureInPickture::SetDefaultPIPMaterial()
{
	InitializeDefaultPIPMaterial();
	mPIPMaterial = PicktureInPickture::mDefualtPIPMaterial;
	D_ASSERT(IsValid(mPIPMaterial));
	D_ASSERT(mPIPMaterial->IsMaterialCreated());
}

dooms::graphics::PicktureInPickture::PicktureInPickture(const math::Vector2& leftBottomNDCPoint, const math::Vector2& rightTopNDCPoint, TextureView* const _drawedTexture)
	:mPlaneMesh{ meshHelper::GetQuadMesh(leftBottomNDCPoint, rightTopNDCPoint, meshHelper::GetFlipOptionBasedOnCurrentGraphicsAPI()) }, mDrawedTexture(_drawedTexture), mPIPMaterial(nullptr), bmIsDrawOnScreen(false)
{
	D_ASSERT(IsValid(mDrawedTexture));
	SetDefaultPIPMaterial();
}

dooms::graphics::PicktureInPickture::PicktureInPickture(const math::Vector2& leftBottomNDCPoint, const math::Vector2& rightTopNDCPoint, Material* const _pipMaterial)
	:mPlaneMesh{ meshHelper::GetQuadMesh(leftBottomNDCPoint, rightTopNDCPoint, meshHelper::GetFlipOptionBasedOnCurrentGraphicsAPI()) }, mDrawedTexture(nullptr), mPIPMaterial(_pipMaterial), bmIsDrawOnScreen(false)
{
	D_ASSERT(IsValid(mDrawedTexture));
	D_ASSERT(IsValid(mPIPMaterial));
	D_ASSERT(mPIPMaterial->IsMaterialCreated());
}

/*dooms::graphics::PicktureInPickture& dooms::graphics::PicktureInPickture::operator=
(
	const math::Vector2& leftBottomNDCPoint, 
	const math::Vector2& rightTopNDCPoint, 
	SingleTexture* const _drawedTexture
	)
{
	mPlaneMesh = Mesh::GetQuadMesh(leftBottomNDCPoint, rightTopNDCPoint);
	mDrawedTexture = _drawedTexture;
	SetDefaultPIPMaterial();
	bmIsDrawOnScreen = true;
}

dooms::graphics::PicktureInPickture& dooms::graphics::PicktureInPickture::operator=
(
	const math::Vector2& leftBottomNDCPoint, 
	const math::Vector2& rightTopNDCPoint, 
	SingleTexture* const _drawedTexture,
	Material* const _pipMaterial
	)
{
	mPlaneMesh = Mesh::GetQuadMesh(leftBottomNDCPoint, rightTopNDCPoint);
	mDrawedTexture = _drawedTexture;
	mPIPMaterial = _pipMaterial;
	bmIsDrawOnScreen = true;
}*/

dooms::graphics::PicktureInPickture::~PicktureInPickture()
{
}

void dooms::graphics::PicktureInPickture::SetTexture(TextureView* const texture)
{
	D_ASSERT(IsValid(texture));
	mDrawedTexture = texture;
}

void dooms::graphics::PicktureInPickture::SetMaterial(Material* const _pipMaterial)
{
	D_ASSERT(IsValid(_pipMaterial));
	D_ASSERT(_pipMaterial->IsMaterialCreated());
	mPIPMaterial = _pipMaterial;
}

void dooms::graphics::PicktureInPickture::DrawPictureInPicture()
{
	if (bmIsDrawOnScreen == true)
	{
		D_ASSERT(IsValid(mDrawedTexture) && IsValid(mPIPMaterial));
		if (IsValid(mDrawedTexture) || IsValid(mPIPMaterial))
		{
			if (IsValid(mPIPMaterial))
			{
				mPIPMaterial->BindMaterial();
			}

			if (IsValid(mDrawedTexture))
			{
				mDrawedTexture->BindTexture(0, GraphicsAPI::eGraphicsPipeLineStage::PIXEL_SHADER);
			}

			mPlaneMesh->Draw();

			if (IsValid(mDrawedTexture))
			{
				// Released rather than left bound, because the texture shown
				// here is usually the colour attachment of a frame buffer that
				// is rendered into again next frame. D3D11 will not have the
				// same resource bound as a shader resource and a render target
				// at once: it drops the render target binding and carries on, so
				// the next frame's pass draws into nothing and the view goes
				// black from the second frame onwards.
				mDrawedTexture->UnBindTexture(0, GraphicsAPI::eGraphicsPipeLineStage::PIXEL_SHADER);
			}
		}
		else
		{
			D_DEBUG_LOG(eLogType::D_ERROR, "Plase Set Texture of PIP");
		}
	}
	
	
}
