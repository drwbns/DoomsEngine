#include "MeshHelper.h"

#include <cmath>

#include "Asset/ThreeDModelAsset.h"
#include <Game/AssetManager/AssetManager.h>

namespace dooms::graphics::meshHelper
{
	static asset::ThreeDModelAsset* DefaultQuadThreeDModelAsset{ nullptr };
	static asset::ThreeDModelAsset* DefaultTriangleThreeDModelAsset{ nullptr };

	static MeshData GetTriangleMeshVertexData(const math::Vector2& pointA, const FLOAT32 width, const FLOAT32 height)
	{
		FLOAT32 QuadMeshData[]
		{
			pointA.x, pointA.y + height, 0.0f, 0.0f, 1.0f,
			pointA.x, pointA.y, 0.0f, 0.0f, 0.0f,
			pointA.x + width, pointA.y, 0.0f, 1.0f, 0.0f
		};

		MeshData MeshVertexDataList{ 3 };
		for (size_t i = 0; i < 3; i++)
		{
			MeshVertexDataList.mVertex[i] = math::Vector3(QuadMeshData[5 * i + 0], QuadMeshData[5 * i + 1], QuadMeshData[5 * i + 2]);
			MeshVertexDataList.mTexCoord[i] = math::Vector3(QuadMeshData[5 * i + 3], QuadMeshData[5 * i + 4], 0.0f);
		}

		return MeshVertexDataList;
	}

	static asset::ThreeDModelAsset* GetTriangleThreeDModelAssset(const math::Vector2& pointA, const FLOAT32 width, const FLOAT32 height)
	{
		std::vector<ThreeDModelMesh> threeDModelMeshes{};
		threeDModelMeshes.emplace_back(nullptr);

		threeDModelMeshes[0].mPrimitiveType = ePrimitiveType::TRIANGLES;
		threeDModelMeshes[0].mVertexArrayFlag = eVertexArrayFlag::VertexVector3 | eVertexArrayFlag::TexCoord;

		threeDModelMeshes[0].mVerticeStride = 12;

		MeshData meshVertexData = GetTriangleMeshVertexData(pointA, width, height);
		threeDModelMeshes[0].mMeshDatas = std::move(meshVertexData);
		threeDModelMeshes[0].mMeshIndices = { 0, 1, 2 };
		threeDModelMeshes[0].bHasIndices = true;

		const math::Vector3 minX = math::Vector3(pointA.x, pointA.y, 0.001f);
		const math::Vector3 maxX = math::Vector3(pointA.x + width, pointA.y + height, 0.001f);
		threeDModelMeshes[0].mAABB3D = physics::AABB3D(minX, maxX);
		threeDModelMeshes[0].mSphere = physics::Sphere(math::Vector3(width * 0.5f, height * 0.5f, 0.0f), std::max(width, height) * 0.5f);

		ThreeDModelNode* const threeDModelNode = dooms::CreateDObject<ThreeDModelNode>(nullptr);
		threeDModelNode->mThreeDModelNodeParent = nullptr;
		threeDModelNode->mModelMeshIndexs.push_back(0);

		asset::ThreeDModelAsset* const threeDModelAsset = dooms::CreateDObject<asset::ThreeDModelAsset>(std::move(threeDModelMeshes), threeDModelNode);
		assetImporter::AssetManager::GetSingleton()->AddAssetToAssetContainer(threeDModelAsset);

		return threeDModelAsset;
	}

	static MeshData GetQuadMeshVertexData(const math::Vector2& leftbottom, const math::Vector2& rightup)
	{
		FLOAT32 QuadMeshData[]
		{
			leftbottom.x, rightup.y, 0.0f, 0.0f, 1.0f,
			leftbottom.x, leftbottom.y, 0.0f, 0.0f, 0.0f,
			rightup.x, leftbottom.y, 0.0f, 1.0f, 0.0f,

			rightup.x, leftbottom.y, 0.0f, 1.0f, 0.0f,
			rightup.x, rightup.y, 0.0f, 1.0f, 1.0f,
			leftbottom.x, rightup.y, 0.0f, 0.0f, 1.0f,
		};

		MeshData MeshVertexDataList{6};
		for(size_t i = 0 ; i < 6 ; i++)
		{
			MeshVertexDataList.mVertex[i] = math::Vector3(QuadMeshData[5 * i + 0], QuadMeshData[5 * i + 1], QuadMeshData[5 * i + 2]);
			MeshVertexDataList.mTexCoord[i] = math::Vector3(QuadMeshData[5 * i + 3], QuadMeshData[5 * i + 4], 0.0f);
		}

		return MeshVertexDataList;
	}

	static asset::ThreeDModelAsset* GetQuadThreeDModelAssset(const math::Vector2& leftbottom, const math::Vector2& rightup)
	{
		std::vector<ThreeDModelMesh> threeDModelMeshes{};
		threeDModelMeshes.emplace_back(nullptr);

		threeDModelMeshes[0].mPrimitiveType = ePrimitiveType::TRIANGLES;
		threeDModelMeshes[0].mVertexArrayFlag = eVertexArrayFlag::VertexVector3 | eVertexArrayFlag::TexCoord;

		threeDModelMeshes[0].mVerticeStride = 12;
		
		MeshData meshVertexData = GetQuadMeshVertexData(leftbottom, rightup);
		threeDModelMeshes[0].mMeshDatas = std::move(meshVertexData);
		threeDModelMeshes[0].mMeshIndices = { 0, 1, 2, 3, 4, 5 };
		threeDModelMeshes[0].bHasIndices = true;

		const math::Vector3 minX = math::Vector3(std::min(leftbottom.x, rightup.x), std::min(leftbottom.y, rightup.y), 0.001f);
		const math::Vector3 maxX = math::Vector3(std::max(leftbottom.x, rightup.x), std::max(leftbottom.y, rightup.y), 0.001f);
		threeDModelMeshes[0].mAABB3D = physics::AABB3D(minX, maxX);
		threeDModelMeshes[0].mSphere = physics::Sphere(math::Vector3(0.0f), 0.5f * std::max(std::abs(rightup.x - leftbottom.x), std::abs(rightup.y - leftbottom.y)));

		ThreeDModelNode* const threeDModelNode = dooms::CreateDObject<ThreeDModelNode>(nullptr);
		threeDModelNode->mThreeDModelNodeParent = nullptr;
		threeDModelNode->mModelMeshIndexs.push_back(0);

		asset::ThreeDModelAsset* const threeDModelAsset = dooms::CreateDObject<asset::ThreeDModelAsset>(std::move(threeDModelMeshes), threeDModelNode);
		assetImporter::AssetManager::GetSingleton()->AddAssetToAssetContainer(threeDModelAsset);

		return threeDModelAsset;
	}
}

dooms::graphics::Mesh* dooms::graphics::meshHelper::GetQuadMesh()
{
	if (dooms::graphics::meshHelper::DefaultQuadThreeDModelAsset == nullptr)
	{
		DefaultQuadThreeDModelAsset = GetQuadThreeDModelAssset(math::Vector2(-1.0f, -1.0f), math::Vector2(1.0f, 1.0f));
		
	}

	return DefaultQuadThreeDModelAsset->GetMesh(0);
}

dooms::graphics::Mesh* dooms::graphics::meshHelper::GetQuadMesh(const math::Vector2& leftbottom, const math::Vector2& rightup)
{
	asset::ThreeDModelAsset* const threeDModelAsset = GetQuadThreeDModelAssset(leftbottom, rightup);

	return threeDModelAsset->GetMesh(0);
}

dooms::graphics::Mesh* dooms::graphics::meshHelper::GetTriangleMesh()
{
	if (dooms::graphics::meshHelper::DefaultTriangleThreeDModelAsset == nullptr)
	{
		DefaultTriangleThreeDModelAsset = GetTriangleThreeDModelAssset(math::Vector2(-1.0f, -1.0f), 1.0f, 1.0f);

	}

	return DefaultTriangleThreeDModelAsset->GetMesh(0);
}

dooms::graphics::Mesh* dooms::graphics::meshHelper::GetTriangleMesh
(
	const math::Vector2& pointA, 
	const FLOAT32 width,
	const FLOAT32 height
)
{
	asset::ThreeDModelAsset* const threeDModelAsset = GetTriangleThreeDModelAssset(pointA, width, height);

	return threeDModelAsset->GetMesh(0);
}
