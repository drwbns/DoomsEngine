#include "PicktureInPickture.h"

#include "Game/AssetManager/AssetManager.h"
#include "../Texture/SingleTexture.h"


void doom::graphics::PicktureInPickture::InitializeDefaultPIPMaterial()
{
	if (PicktureInPickture::mDefualtPIPMaterial.IsGenerated() == false)
	{
		doom::asset::ShaderAsset* const pipMaterial = doom::assetimporter::AssetManager::GetAsset<asset::eAssetType::SHADER>("Default2DTextureShader.glsl");
		PicktureInPickture::mDefualtPIPMaterial.SetShaderAsset(pipMaterial);
	}

	
}

doom::graphics::PicktureInPickture::PicktureInPickture(const math::Vector2& leftBottomNDCPoint, const math::Vector2& rightTopNDCPoint, SingleTexture* const _drawedTexture)
	:mPlaneMesh{ Mesh::GetQuadMesh(leftBottomNDCPoint, rightTopNDCPoint) }, mDrawedTexture(_drawedTexture), mPIPMaterial(nullptr)
{
	InitializeDefaultPIPMaterial();
	mPIPMaterial = &(PicktureInPickture::mDefualtPIPMaterial);
}

doom::graphics::PicktureInPickture::PicktureInPickture(const math::Vector2& leftBottomNDCPoint, const math::Vector2& rightTopNDCPoint, SingleTexture* const _drawedTexture, Material* const _pipMaterial)
	:mPlaneMesh{ Mesh::GetQuadMesh(leftBottomNDCPoint, rightTopNDCPoint) }, mDrawedTexture(_drawedTexture), mPIPMaterial(_pipMaterial)
{
	
}

doom::graphics::PicktureInPickture::~PicktureInPickture()
{
}

void doom::graphics::PicktureInPickture::SetTexture(SingleTexture* const texture)
{
	mDrawedTexture = texture;
}

void doom::graphics::PicktureInPickture::SetMaterial(Material* const _pipMaterial)
{
	mPIPMaterial = _pipMaterial;
}

void doom::graphics::PicktureInPickture::DrawPictureInPicture()
{
	if (mDrawedTexture != nullptr && mPIPMaterial != nullptr)
	{
		mPIPMaterial->UseProgram();
		mDrawedTexture->ActiveTexture(0);
		mDrawedTexture->BindTexture();
		mPlaneMesh.Draw();
	}
	else
	{
		D_DEBUG_LOG("Plase Set Texture of PIP", eLogType::D_ERROR);
	}
	
}
