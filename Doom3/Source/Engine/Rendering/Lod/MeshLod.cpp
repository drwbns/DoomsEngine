#include "MeshLod.h"

#include <unordered_map>
#include <vector>
#include <cmath>

#include <Rendering/Buffer/Mesh.h>
#include <Asset/ThreeDModelAsset.h>
#include <DObject/DObjectGlobals.h>
#include <Graphics/graphicsSetting.h>

namespace
{
	// Vertex clustering: snap every vertex to a grid cell, keep one
	// representative per cell, and drop the triangles that collapse to a line
	// or a point once their corners land in the same cell.
	//
	// Chosen over quadric error simplification because it cannot fail. There
	// are no degenerate configurations to handle, no boundary rules, and the
	// output is always a subset of the original vertices, which is what lets
	// every level share one vertex buffer. It is a blunter instrument than
	// quadric error and shows it on silhouettes, which is exactly where it is
	// least visible: on objects small enough to have been given a coarse level
	// in the first place.
	void BuildClusteredIndices(
		const dooms::ThreeDModelMesh& modelMesh,
		const unsigned int gridResolution,
		std::vector<UINT32>& outIndices)
	{
		outIndices.clear();

		const dooms::MeshData& meshData = modelMesh.mMeshDatas;

		if (meshData.mVertex == nullptr || meshData.mVerticeCount == 0 || modelMesh.mMeshIndices.empty())
		{
			return;
		}

		math::Vector3 lowerBound = meshData.mVertex[0];
		math::Vector3 upperBound = meshData.mVertex[0];

		for (UINT64 vertexIndex = 1; vertexIndex < meshData.mVerticeCount; vertexIndex++)
		{
			const math::Vector3& vertex = meshData.mVertex[vertexIndex];

			lowerBound.x = (vertex.x < lowerBound.x) ? vertex.x : lowerBound.x;
			lowerBound.y = (vertex.y < lowerBound.y) ? vertex.y : lowerBound.y;
			lowerBound.z = (vertex.z < lowerBound.z) ? vertex.z : lowerBound.z;

			upperBound.x = (vertex.x > upperBound.x) ? vertex.x : upperBound.x;
			upperBound.y = (vertex.y > upperBound.y) ? vertex.y : upperBound.y;
			upperBound.z = (vertex.z > upperBound.z) ? vertex.z : upperBound.z;
		}

		const float extentX = upperBound.x - lowerBound.x;
		const float extentY = upperBound.y - lowerBound.y;
		const float extentZ = upperBound.z - lowerBound.z;

		const float inverseCellX = (extentX > 1e-9f) ? (gridResolution / extentX) : 0.0f;
		const float inverseCellY = (extentY > 1e-9f) ? (gridResolution / extentY) : 0.0f;
		const float inverseCellZ = (extentZ > 1e-9f) ? (gridResolution / extentZ) : 0.0f;

		// The first vertex to land in a cell speaks for the cell. Averaging
		// would move it off the surface and shrink the object slightly, which
		// matters when the same buffer is shared with the full detail level.
		std::unordered_map<unsigned long long, UINT32> representativePerCell;
		representativePerCell.reserve(static_cast<size_t>(meshData.mVerticeCount));

		std::vector<UINT32> representativeForVertex(static_cast<size_t>(meshData.mVerticeCount), 0);

		for (UINT64 vertexIndex = 0; vertexIndex < meshData.mVerticeCount; vertexIndex++)
		{
			const math::Vector3& vertex = meshData.mVertex[vertexIndex];

			const unsigned long long cellX = static_cast<unsigned long long>((vertex.x - lowerBound.x) * inverseCellX);
			const unsigned long long cellY = static_cast<unsigned long long>((vertex.y - lowerBound.y) * inverseCellY);
			const unsigned long long cellZ = static_cast<unsigned long long>((vertex.z - lowerBound.z) * inverseCellZ);

			const unsigned long long cellKey = (cellX << 42) | (cellY << 21) | cellZ;

			const auto inserted = representativePerCell.emplace(cellKey, static_cast<UINT32>(vertexIndex));
			representativeForVertex[static_cast<size_t>(vertexIndex)] = inserted.first->second;
		}

		outIndices.reserve(modelMesh.mMeshIndices.size());

		for (size_t indexIndex = 0; indexIndex + 2 < modelMesh.mMeshIndices.size(); indexIndex += 3)
		{
			const UINT32 a = representativeForVertex[modelMesh.mMeshIndices[indexIndex]];
			const UINT32 b = representativeForVertex[modelMesh.mMeshIndices[indexIndex + 1]];
			const UINT32 c = representativeForVertex[modelMesh.mMeshIndices[indexIndex + 2]];

			// Two corners in one cell means the triangle has collapsed to an
			// edge, and it contributes nothing but work.
			if (a == b || b == c || a == c)
			{
				continue;
			}

			outIndices.push_back(a);
			outIndices.push_back(b);
			outIndices.push_back(c);
		}
	}

	std::unordered_map<const dooms::graphics::Mesh*, dooms::graphics::MeshLodChain> gMeshLodChains;
	std::vector<dooms::ThreeDModelMesh*> gLodModelMeshes;
	dooms::graphics::MeshLodChain gEmptyChain;
	unsigned int gTotalLevelCount = 0;
}

const dooms::graphics::MeshLodChain& dooms::graphics::GetMeshLodChain(const Mesh* const mesh)
{
	if (mesh == nullptr)
	{
		return gEmptyChain;
	}

	const auto found = gMeshLodChains.find(mesh);
	if (found != gMeshLodChains.end())
	{
		return found->second;
	}

	MeshLodChain& chain = gMeshLodChains[mesh];
	chain.bmIsBuilt = true;

	const dooms::ThreeDModelMesh* const modelMesh = mesh->GetTargetThreeDModelMesh();

	if (modelMesh == nullptr || modelMesh->mMeshIndices.empty())
	{
		return chain;
	}

	// Level zero is the mesh itself, so selection can always fall back to it
	// without a special case.
	chain.mLevels.push_back(const_cast<Mesh*>(mesh));
	chain.mIndexCounts.push_back(mesh->GetNumOfIndices());

	// Roughly a quarter of the triangles at each step, which is a level about
	// every halving of the object's size on screen.
	const unsigned int gridResolutions[3] = { 24, 12, 6 };

	for (const unsigned int gridResolution : gridResolutions)
	{
		std::vector<UINT32> simplifiedIndices;
		BuildClusteredIndices(*modelMesh, gridResolution, simplifiedIndices);

		if (simplifiedIndices.size() < 3)
		{
			break;
		}

		// Not worth a level of its own, and not worth the buffer.
		if (simplifiedIndices.size() * 10 > chain.mIndexCounts.back() * 8)
		{
			continue;
		}

		// Copied so the level owns vertex data of its own for upload. The
		// indices are the only part that differs, but the upload path takes a
		// whole model mesh.
		dooms::ThreeDModelMesh* const lodModelMesh = new dooms::ThreeDModelMesh(*modelMesh);
		lodModelMesh->mMeshIndices = simplifiedIndices;
		lodModelMesh->bHasIndices = true;

		gLodModelMeshes.push_back(lodModelMesh);

		Mesh* const lodMesh = dooms::CreateDObject<Mesh>();
		if (IsValid(lodMesh) == false)
		{
			break;
		}

		*lodMesh = *lodModelMesh;

		chain.mLevels.push_back(lodMesh);
		chain.mIndexCounts.push_back(lodMesh->GetNumOfIndices());

		gTotalLevelCount++;
	}

	return chain;
}

const dooms::graphics::Mesh* dooms::graphics::SelectMeshLod(const Mesh* const mesh, const float coveredPixelCount)
{
	if (graphicsSetting::IsMeshLodEnabled == false || mesh == nullptr)
	{
		return mesh;
	}

	const MeshLodChain& chain = GetMeshLodChain(mesh);

	if (chain.mLevels.size() < 2)
	{
		return mesh;
	}

	// One triangle per pixel is the point past which more geometry cannot be
	// resolved. The multiplier is there so the trade can be measured rather
	// than assumed correct at exactly one.
	const float affordableTriangleCount =
		coveredPixelCount * graphicsSetting::MeshLodTrianglesPerPixel;

	const unsigned long long affordableIndexCount =
		static_cast<unsigned long long>((affordableTriangleCount > 0.0f) ? (affordableTriangleCount * 3.0f) : 0.0f);

	// Coarsest first, so the answer is the cheapest level that still has enough
	// triangles to be worth having.
	for (size_t levelIndex = chain.mLevels.size(); levelIndex-- > 1; )
	{
		if (chain.mIndexCounts[levelIndex] >= affordableIndexCount)
		{
			return chain.mLevels[levelIndex];
		}
	}

	return chain.mLevels[0];
}

void dooms::graphics::GetMeshLodStatistics(unsigned int& outMeshCount, unsigned int& outLevelCount)
{
	outMeshCount = static_cast<unsigned int>(gMeshLodChains.size());
	outLevelCount = gTotalLevelCount;
}
