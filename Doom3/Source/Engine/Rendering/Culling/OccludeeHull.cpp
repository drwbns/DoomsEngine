#include "OccludeeHull.h"

#include <unordered_map>
#include <algorithm>
#include <cmath>

#include <Rendering/Buffer/Mesh.h>
#include <Asset/ThreeDModelAsset.h>
#include <Graphics/graphicsSetting.h>

namespace
{
	// A face of the hull under construction, as three vertex indices and the
	// outward plane they lie on.
	struct HullFace
	{
		int mA{ 0 };
		int mB{ 0 };
		int mC{ 0 };
		math::Vector3 mNormal{ 0.0f, 0.0f, 0.0f };
		float mPlaneDistance{ 0.0f };
		bool bmIsAlive{ true };
	};

	float SignedDistanceToFace(const HullFace& face, const math::Vector3& point)
	{
		return (face.mNormal.x * point.x + face.mNormal.y * point.y + face.mNormal.z * point.z) - face.mPlaneDistance;
	}

	HullFace MakeFace(const std::vector<math::Vector3>& points, const int a, const int b, const int c, const math::Vector3& interiorPoint)
	{
		HullFace face;
		face.mA = a;
		face.mB = b;
		face.mC = c;

		const math::Vector3 edge1 = points[b] - points[a];
		const math::Vector3 edge2 = points[c] - points[a];

		math::Vector3 normal
		(
			edge1.y * edge2.z - edge1.z * edge2.y,
			edge1.z * edge2.x - edge1.x * edge2.z,
			edge1.x * edge2.y - edge1.y * edge2.x
		);

		const float length = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
		if (length > 1e-12f)
		{
			normal.x /= length;
			normal.y /= length;
			normal.z /= length;
		}

		face.mNormal = normal;
		face.mPlaneDistance = normal.x * points[a].x + normal.y * points[a].y + normal.z * points[a].z;

		// Faced away from the inside, so "visible from a point" means that point
		// lies outside this face.
		if (SignedDistanceToFace(face, interiorPoint) > 0.0f)
		{
			face.mNormal.x = -face.mNormal.x;
			face.mNormal.y = -face.mNormal.y;
			face.mNormal.z = -face.mNormal.z;
			face.mPlaneDistance = -face.mPlaneDistance;
			std::swap(face.mB, face.mC);
		}

		return face;
	}

	// Four points that actually span three dimensions, or false when the mesh is
	// degenerate enough that a hull is meaningless.
	bool FindInitialTetrahedron(const std::vector<math::Vector3>& points, int outIndices[4])
	{
		const size_t pointCount = points.size();
		if (pointCount < 4)
		{
			return false;
		}

		int minXIndex = 0;
		int maxXIndex = 0;

		for (size_t index = 1; index < pointCount; index++)
		{
			if (points[index].x < points[minXIndex].x) { minXIndex = static_cast<int>(index); }
			if (points[index].x > points[maxXIndex].x) { maxXIndex = static_cast<int>(index); }
		}

		if (minXIndex == maxXIndex)
		{
			return false;
		}

		outIndices[0] = minXIndex;
		outIndices[1] = maxXIndex;

		// Furthest from the line through the first two.
		const math::Vector3 lineStart = points[outIndices[0]];
		math::Vector3 lineDirection = points[outIndices[1]] - lineStart;
		const float lineLength = std::sqrt(lineDirection.x * lineDirection.x + lineDirection.y * lineDirection.y + lineDirection.z * lineDirection.z);

		if (lineLength <= 1e-12f)
		{
			return false;
		}

		lineDirection.x /= lineLength;
		lineDirection.y /= lineLength;
		lineDirection.z /= lineLength;

		int bestIndex = -1;
		float bestDistance = 1e-8f;

		for (size_t index = 0; index < pointCount; index++)
		{
			const math::Vector3 offset = points[index] - lineStart;
			const float along = offset.x * lineDirection.x + offset.y * lineDirection.y + offset.z * lineDirection.z;

			const math::Vector3 perpendicular
			(
				offset.x - lineDirection.x * along,
				offset.y - lineDirection.y * along,
				offset.z - lineDirection.z * along
			);

			const float distance = std::sqrt(perpendicular.x * perpendicular.x + perpendicular.y * perpendicular.y + perpendicular.z * perpendicular.z);

			if (distance > bestDistance)
			{
				bestDistance = distance;
				bestIndex = static_cast<int>(index);
			}
		}

		if (bestIndex < 0)
		{
			return false;
		}

		outIndices[2] = bestIndex;

		// Furthest off the plane of the first three.
		const math::Vector3 edge1 = points[outIndices[1]] - points[outIndices[0]];
		const math::Vector3 edge2 = points[outIndices[2]] - points[outIndices[0]];

		const math::Vector3 planeNormal
		(
			edge1.y * edge2.z - edge1.z * edge2.y,
			edge1.z * edge2.x - edge1.x * edge2.z,
			edge1.x * edge2.y - edge1.y * edge2.x
		);

		bestIndex = -1;
		bestDistance = 1e-8f;

		for (size_t index = 0; index < pointCount; index++)
		{
			const math::Vector3 offset = points[index] - points[outIndices[0]];
			const float distance = std::fabs(offset.x * planeNormal.x + offset.y * planeNormal.y + offset.z * planeNormal.z);

			if (distance > bestDistance)
			{
				bestDistance = distance;
				bestIndex = static_cast<int>(index);
			}
		}

		if (bestIndex < 0)
		{
			return false;
		}

		outIndices[3] = bestIndex;
		return true;
	}

	void BuildConvexHullVertices(const std::vector<math::Vector3>& points, std::vector<math::Vector3>& outVertices)
	{
		outVertices.clear();

		int initialIndices[4] = { 0, 0, 0, 0 };
		if (FindInitialTetrahedron(points, initialIndices) == false)
		{
			return;
		}

		const math::Vector3 interiorPoint
		(
			(points[initialIndices[0]].x + points[initialIndices[1]].x + points[initialIndices[2]].x + points[initialIndices[3]].x) * 0.25f,
			(points[initialIndices[0]].y + points[initialIndices[1]].y + points[initialIndices[2]].y + points[initialIndices[3]].y) * 0.25f,
			(points[initialIndices[0]].z + points[initialIndices[1]].z + points[initialIndices[2]].z + points[initialIndices[3]].z) * 0.25f
		);

		std::vector<HullFace> faces;
		faces.reserve(256);

		faces.push_back(MakeFace(points, initialIndices[0], initialIndices[1], initialIndices[2], interiorPoint));
		faces.push_back(MakeFace(points, initialIndices[0], initialIndices[1], initialIndices[3], interiorPoint));
		faces.push_back(MakeFace(points, initialIndices[0], initialIndices[2], initialIndices[3], interiorPoint));
		faces.push_back(MakeFace(points, initialIndices[1], initialIndices[2], initialIndices[3], interiorPoint));

		// Scaled to the model, so a large rock and a pebble are treated alike.
		float extent = 0.0f;
		for (const math::Vector3& point : points)
		{
			extent = std::max(extent, std::fabs(point.x));
			extent = std::max(extent, std::fabs(point.y));
			extent = std::max(extent, std::fabs(point.z));
		}

		const float epsilon = std::max(1e-6f, extent * 1e-5f);

		std::vector<int> visibleFaceIndices;
		std::vector<std::pair<int, int>> horizonEdges;

		for (size_t pointIndex = 0; pointIndex < points.size(); pointIndex++)
		{
			const math::Vector3& point = points[pointIndex];

			visibleFaceIndices.clear();

			for (size_t faceIndex = 0; faceIndex < faces.size(); faceIndex++)
			{
				if (faces[faceIndex].bmIsAlive && SignedDistanceToFace(faces[faceIndex], point) > epsilon)
				{
					visibleFaceIndices.push_back(static_cast<int>(faceIndex));
				}
			}

			if (visibleFaceIndices.empty())
			{
				continue;
			}

			// The horizon is every edge belonging to exactly one visible face.
			// Edges shared by two visible faces are interior to the region being
			// replaced and disappear with it.
			horizonEdges.clear();

			for (const int faceIndex : visibleFaceIndices)
			{
				const HullFace& face = faces[faceIndex];

				const int edges[3][2] = { { face.mA, face.mB }, { face.mB, face.mC }, { face.mC, face.mA } };

				for (const auto& edge : edges)
				{
					bool bIsShared = false;

					for (const int otherFaceIndex : visibleFaceIndices)
					{
						if (otherFaceIndex == faceIndex)
						{
							continue;
						}

						const HullFace& other = faces[otherFaceIndex];
						const int otherEdges[3][2] = { { other.mA, other.mB }, { other.mB, other.mC }, { other.mC, other.mA } };

						for (const auto& otherEdge : otherEdges)
						{
							if ((otherEdge[0] == edge[1] && otherEdge[1] == edge[0]) ||
								(otherEdge[0] == edge[0] && otherEdge[1] == edge[1]))
							{
								bIsShared = true;
								break;
							}
						}

						if (bIsShared)
						{
							break;
						}
					}

					if (bIsShared == false)
					{
						horizonEdges.emplace_back(edge[0], edge[1]);
					}
				}
			}

			for (const int faceIndex : visibleFaceIndices)
			{
				faces[faceIndex].bmIsAlive = false;
			}

			for (const std::pair<int, int>& edge : horizonEdges)
			{
				faces.push_back(MakeFace(points, edge.first, edge.second, static_cast<int>(pointIndex), interiorPoint));
			}

			// Compacted occasionally rather than every step, since the scan above
			// already skips dead faces and erasing from the middle is worse.
			if (faces.size() > 4096)
			{
				faces.erase(
					std::remove_if(faces.begin(), faces.end(), [](const HullFace& face) { return face.bmIsAlive == false; }),
					faces.end());
			}
		}

		std::vector<int> uniqueVertexIndices;
		uniqueVertexIndices.reserve(64);

		for (const HullFace& face : faces)
		{
			if (face.bmIsAlive == false)
			{
				continue;
			}

			for (const int vertexIndex : { face.mA, face.mB, face.mC })
			{
				if (std::find(uniqueVertexIndices.begin(), uniqueVertexIndices.end(), vertexIndex) == uniqueVertexIndices.end())
				{
					uniqueVertexIndices.push_back(vertexIndex);
				}
			}
		}

		outVertices.reserve(uniqueVertexIndices.size());
		for (const int vertexIndex : uniqueVertexIndices)
		{
			outVertices.push_back(points[vertexIndex]);
		}
	}

	// Reduce a hull to a vertex budget without losing containment.
	//
	// Rocks are convex blobs, so nearly every mesh vertex lands on the hull and
	// an exact hull costs as much to project as the mesh does. Vertices are
	// bucketed by direction from the centroid and the furthest in each bucket is
	// kept, which alone would produce a shape *inside* the original and cull
	// visible objects. So every kept vertex is then pushed out from the centroid
	// by the largest factor any dropped vertex needed, which restores
	// containment at the cost of a slightly looser fit.
	void DecimateHullConservatively(std::vector<math::Vector3>& hullVertices, const unsigned int vertexBudget)
	{
		if (hullVertices.size() <= vertexBudget || vertexBudget < 8)
		{
			return;
		}

		math::Vector3 centroid(0.0f, 0.0f, 0.0f);
		for (const math::Vector3& vertex : hullVertices)
		{
			centroid.x += vertex.x;
			centroid.y += vertex.y;
			centroid.z += vertex.z;
		}

		const float inverseCount = 1.0f / static_cast<float>(hullVertices.size());
		centroid.x *= inverseCount;
		centroid.y *= inverseCount;
		centroid.z *= inverseCount;

		// Buckets over the sphere of directions, as a coarse latitude and
		// longitude grid. Not equal area, which only means some buckets hold
		// more than others, never that a direction is missed.
		const int longitudeBucketCount = static_cast<int>(std::sqrt(static_cast<float>(vertexBudget) * 2.0f));
		const int latitudeBucketCount = std::max(2, static_cast<int>(vertexBudget) / std::max(1, longitudeBucketCount));

		std::vector<int> bestVertexPerBucket(static_cast<size_t>(longitudeBucketCount) * latitudeBucketCount, -1);
		std::vector<float> bestDistancePerBucket(bestVertexPerBucket.size(), -1.0f);

		std::vector<float> vertexDistances(hullVertices.size(), 0.0f);

		for (size_t vertexIndex = 0; vertexIndex < hullVertices.size(); vertexIndex++)
		{
			const math::Vector3 offset = hullVertices[vertexIndex] - centroid;
			const float distance = std::sqrt(offset.x * offset.x + offset.y * offset.y + offset.z * offset.z);

			vertexDistances[vertexIndex] = distance;

			if (distance <= 1e-12f)
			{
				continue;
			}

			const float latitude = std::acos(std::max(-1.0f, std::min(1.0f, offset.z / distance)));
			const float longitude = std::atan2(offset.y, offset.x) + 3.14159265f;

			int latitudeBucket = static_cast<int>(latitude / 3.14159265f * latitudeBucketCount);
			int longitudeBucket = static_cast<int>(longitude / 6.28318531f * longitudeBucketCount);

			latitudeBucket = std::max(0, std::min(latitudeBucketCount - 1, latitudeBucket));
			longitudeBucket = std::max(0, std::min(longitudeBucketCount - 1, longitudeBucket));

			const size_t bucket = static_cast<size_t>(latitudeBucket) * longitudeBucketCount + longitudeBucket;

			if (distance > bestDistancePerBucket[bucket])
			{
				bestDistancePerBucket[bucket] = distance;
				bestVertexPerBucket[bucket] = static_cast<int>(vertexIndex);
			}
		}

		std::vector<math::Vector3> keptVertices;
		std::vector<float> keptDistances;

		for (size_t bucket = 0; bucket < bestVertexPerBucket.size(); bucket++)
		{
			if (bestVertexPerBucket[bucket] >= 0)
			{
				keptVertices.push_back(hullVertices[bestVertexPerBucket[bucket]]);
				keptDistances.push_back(bestDistancePerBucket[bucket]);
			}
		}

		if (keptVertices.size() < 4)
		{
			return;
		}

		// How far out the kept set has to move so that nothing dropped sticks
		// out of it. Measured radially, which is conservative for a shape that
		// is star shaped about its centroid, and a convex hull always is.
		float requiredExpansion = 1.0f;

		for (size_t vertexIndex = 0; vertexIndex < hullVertices.size(); vertexIndex++)
		{
			const math::Vector3 offset = hullVertices[vertexIndex] - centroid;
			const float distance = vertexDistances[vertexIndex];

			if (distance <= 1e-12f)
			{
				continue;
			}

			// The nearest kept direction, and how far out it reaches.
			float bestAlignment = -2.0f;
			float bestKeptDistance = 0.0f;

			for (size_t keptIndex = 0; keptIndex < keptVertices.size(); keptIndex++)
			{
				const math::Vector3 keptOffset = keptVertices[keptIndex] - centroid;
				const float keptDistance = keptDistances[keptIndex];

				if (keptDistance <= 1e-12f)
				{
					continue;
				}

				const float alignment =
					(offset.x * keptOffset.x + offset.y * keptOffset.y + offset.z * keptOffset.z) / (distance * keptDistance);

				if (alignment > bestAlignment)
				{
					bestAlignment = alignment;
					bestKeptDistance = keptDistance;
				}
			}

			if (bestKeptDistance > 1e-12f)
			{
				requiredExpansion = std::max(requiredExpansion, distance / bestKeptDistance);
			}
		}

		for (math::Vector3& keptVertex : keptVertices)
		{
			keptVertex.x = centroid.x + (keptVertex.x - centroid.x) * requiredExpansion;
			keptVertex.y = centroid.y + (keptVertex.y - centroid.y) * requiredExpansion;
			keptVertex.z = centroid.z + (keptVertex.z - centroid.z) * requiredExpansion;
		}

		hullVertices.swap(keptVertices);
	}

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
