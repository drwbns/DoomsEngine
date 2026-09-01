#include "OccludeeHull.h"

#include <ConvexHull.h>

#include <unordered_map>

#include <Rendering/Buffer/Mesh.h>
#include <Asset/ThreeDModelAsset.h>
#include <Graphics/graphicsSetting.h>

namespace
{
	std::unordered_map<const dooms::graphics::Mesh*, dooms::graphics::OccludeeHull> gOccludeeHulls;
	dooms::graphics::OccludeeHull gEmptyHull;
	unsigned int gTotalHullVertexCount = 0;
}

const dooms::graphics::OccludeeHull& dooms::graphics::GetOccludeeHull(const Mesh* const mesh)
{
	if (mesh == nullptr)
	{
		return gEmptyHull;
	}

	const auto found = gOccludeeHulls.find(mesh);
	if (found != gOccludeeHulls.end())
	{
		return found->second;
	}

	OccludeeHull& hull = gOccludeeHulls[mesh];
	hull.bmIsBuilt = true;

	const dooms::ThreeDModelMesh* const modelMesh = mesh->GetTargetThreeDModelMesh();

	if (modelMesh != nullptr && modelMesh->mMeshDatas.mVertex != nullptr && modelMesh->mMeshDatas.mVerticeCount >= 4)
	{
		std::vector<math::Vector3> points;
		points.reserve(static_cast<size_t>(modelMesh->mMeshDatas.mVerticeCount));

		for (UINT64 vertexIndex = 0; vertexIndex < modelMesh->mMeshDatas.mVerticeCount; vertexIndex++)
		{
			points.push_back(modelMesh->mMeshDatas.mVertex[vertexIndex]);
		}

		BuildConvexHullVertices(points, hull.mVertices);

		// An exact hull of a rock is about 740 vertices, which costs as much to
		// project every frame as drawing the rock does. The budget is what makes
		// the technique affordable at all.
		DecimateHullConservatively(hull.mVertices, graphicsSetting::HiZHullVertexBudget);
	}

	gTotalHullVertexCount += static_cast<unsigned int>(hull.mVertices.size());

	return hull;
}

void dooms::graphics::GetOccludeeHullStatistics(unsigned int& outMeshCount, unsigned int& outTotalVertexCount)
{
	outMeshCount = static_cast<unsigned int>(gOccludeeHulls.size());
	outTotalVertexCount = gTotalHullVertexCount;
}
