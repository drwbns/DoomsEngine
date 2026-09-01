#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include <Vector3.h>
#include <Rendering/Culling/ConvexHull.h>

namespace
{
	std::vector<math::Vector3> CubeCorners()
	{
		return
		{
			math::Vector3(-1.0f, -1.0f, -1.0f),
			math::Vector3(1.0f, -1.0f, -1.0f),
			math::Vector3(-1.0f, 1.0f, -1.0f),
			math::Vector3(1.0f, 1.0f, -1.0f),
			math::Vector3(-1.0f, -1.0f, 1.0f),
			math::Vector3(1.0f, -1.0f, 1.0f),
			math::Vector3(-1.0f, 1.0f, 1.0f),
			math::Vector3(1.0f, 1.0f, 1.0f)
		};
	}

	// Points spread over a sphere by the golden angle spiral, then pushed
	// away from the origin, so the hull has to hold under an offset and
	// nothing sits on an axis or plane where symmetry hides a bug.
	std::vector<math::Vector3> OffsetSpherePoints(const unsigned int pointCount, const math::Vector3& offset)
	{
		std::vector<math::Vector3> points;
		points.reserve(pointCount);

		constexpr float kGoldenAngle = 2.39996322972865332f;

		for (unsigned int index = 0; index < pointCount; index++)
		{
			const float y = 1.0f - 2.0f * (static_cast<float>(index) + 0.5f) / static_cast<float>(pointCount);
			const float ringRadius = std::sqrt(std::max(0.0f, 1.0f - y * y));
			const float angle = kGoldenAngle * static_cast<float>(index);

			points.push_back(math::Vector3
			(
				offset.x + ringRadius * std::cos(angle),
				offset.y + y,
				offset.z + ringRadius * std::sin(angle)
			));
		}

		return points;
	}

	void ExpectPointInside(const std::vector<math::Vector3>& hullVertices,
		const float x, const float y, const float z, const float epsilon)
	{
		EXPECT_TRUE(dooms::graphics::IsPointInsideHullVertices(hullVertices, math::Vector3(x, y, z), epsilon));
	}

	void ExpectPointOutside(const std::vector<math::Vector3>& hullVertices,
		const float x, const float y, const float z, const float epsilon)
	{
		EXPECT_FALSE(dooms::graphics::IsPointInsideHullVertices(hullVertices, math::Vector3(x, y, z), epsilon));
	}
}

// The containment property is the whole point of the hull: an occludee
// proxy that does not contain its mesh culls visible objects.
TEST(ConvexHull, ExactHullOfCubeContainsEveryInput)
{
	const std::vector<math::Vector3> points = CubeCorners();

	std::vector<math::Vector3> hullVertices;
	dooms::graphics::BuildConvexHullVertices(points, hullVertices);

	// Every corner of a cube is on its hull, and nothing else is.
	ASSERT_EQ(static_cast<size_t>(8), hullVertices.size());

	for (const math::Vector3& point : points)
	{
		ExpectPointInside(hullVertices, point.x, point.y, point.z, 1e-3f);
	}

	// And the hull is the cube, not something smaller or shifted: each
	// output vertex is one of the inputs.
	for (const math::Vector3& hullVertex : hullVertices)
	{
		bool bFoundCorner = false;

		for (const math::Vector3& point : points)
		{
			if (std::fabs(hullVertex.x - point.x) < 1e-4f &&
				std::fabs(hullVertex.y - point.y) < 1e-4f &&
				std::fabs(hullVertex.z - point.z) < 1e-4f)
			{
				bFoundCorner = true;
				break;
			}
		}

		EXPECT_TRUE(bFoundCorner);
	}
}

TEST(ConvexHull, PointsJustOutsideTheCubeAreOutside)
{
	const std::vector<math::Vector3> points = CubeCorners();

	std::vector<math::Vector3> hullVertices;
	dooms::graphics::BuildConvexHullVertices(points, hullVertices);

	ExpectPointInside(hullVertices, 0.0f, 0.0f, 0.0f, 1e-3f); // centre
	ExpectPointInside(hullVertices, 1.0f, 1.0f, 1.0f, 1e-3f); // on the hull itself
	ExpectPointInside(hullVertices, 0.999f, 0.999f, 0.999f, 1e-3f);
	ExpectPointOutside(hullVertices, 2.0f, 0.0f, 0.0f, 1e-3f);
	ExpectPointOutside(hullVertices, -2.0f, -2.0f, -2.0f, 1e-3f);
	ExpectPointOutside(hullVertices, 1.1f, 1.1f, 1.1f, 1e-3f);
}

TEST(ConvexHull, HullOfATetrahedronIsTheTetrahedron)
{
	const std::vector<math::Vector3> points =
	{
		math::Vector3(1.0f, 1.0f, 1.0f),
		math::Vector3(-1.0f, 1.0f, -1.0f),
		math::Vector3(1.0f, -1.0f, -1.0f),
		math::Vector3(-1.0f, -1.0f, 1.0f)
	};

	std::vector<math::Vector3> hullVertices;
	dooms::graphics::BuildConvexHullVertices(points, hullVertices);

	ASSERT_EQ(static_cast<size_t>(4), hullVertices.size());
	ExpectPointInside(hullVertices, 0.0f, 0.0f, 0.0f, 1e-3f);
	ExpectPointOutside(hullVertices, 2.0f, 2.0f, 2.0f, 1e-3f);
}

// Points strictly inside the input set must not become hull vertices,
// and the hull must still contain them.
TEST(ConvexHull, HullOfASpherePointSetContainsAllInputs)
{
	std::vector<math::Vector3> points = OffsetSpherePoints(40, math::Vector3(10.0f, -5.0f, 3.0f));
	points.push_back(math::Vector3(10.0f, -5.0f, 3.0f)); // centre
	points.push_back(math::Vector3(10.3f, -5.2f, 3.1f)); // interior points
	points.push_back(math::Vector3(9.7f, -4.8f, 2.9f));

	std::vector<math::Vector3> hullVertices;
	dooms::graphics::BuildConvexHullVertices(points, hullVertices);

	ASSERT_GE(hullVertices.size(), static_cast<size_t>(4));
	ASSERT_LE(hullVertices.size(), points.size());

	for (const math::Vector3& point : points)
	{
		ExpectPointInside(hullVertices, point.x, point.y, point.z, 2e-3f);
	}
}

// The decimated hull is what the engine actually projects, so it, not
// just the exact hull, has to contain everything: the expansion step is
// the only thing standing between a dropped vertex and a wrongly culled
// visible object.
TEST(ConvexHull, DecimatedHullStillContainsEveryInput)
{
	const std::vector<math::Vector3> points = OffsetSpherePoints(60, math::Vector3(0.0f, 0.0f, 0.0f));

	std::vector<math::Vector3> hullVertices;
	dooms::graphics::BuildConvexHullVertices(points, hullVertices);
	ASSERT_GE(hullVertices.size(), static_cast<size_t>(20));

	dooms::graphics::DecimateHullConservatively(hullVertices, 16);

	EXPECT_LE(hullVertices.size(), static_cast<size_t>(16));
	EXPECT_GE(hullVertices.size(), static_cast<size_t>(4));

	for (const math::Vector3& point : points)
	{
		ExpectPointInside(hullVertices, point.x, point.y, point.z, 2e-2f);
	}

	ExpectPointInside(hullVertices, 0.0f, 0.0f, 0.0f, 2e-2f);
}

TEST(ConvexHull, DecimationIsNoOpWhenHullIsAlreadyUnderBudget)
{
	const std::vector<math::Vector3> points = CubeCorners();

	std::vector<math::Vector3> hullVertices;
	dooms::graphics::BuildConvexHullVertices(points, hullVertices);
	const std::vector<math::Vector3> before = hullVertices;

	dooms::graphics::DecimateHullConservatively(hullVertices, 32);

	ASSERT_EQ(before.size(), hullVertices.size());
	for (size_t index = 0; index < before.size(); index++)
	{
		EXPECT_FLOAT_EQ(before[index].x, hullVertices[index].x);
		EXPECT_FLOAT_EQ(before[index].y, hullVertices[index].y);
		EXPECT_FLOAT_EQ(before[index].z, hullVertices[index].z);
	}
}

// Below eight vertices there is no bucketing scheme worth running, so
// the function is specified to leave the hull alone.
TEST(ConvexHull, DecimationBelowMinimumBudgetIsNoOp)
{
	const std::vector<math::Vector3> points = OffsetSpherePoints(60, math::Vector3(0.0f, 0.0f, 0.0f));

	std::vector<math::Vector3> hullVertices;
	dooms::graphics::BuildConvexHullVertices(points, hullVertices);
	const std::vector<math::Vector3> before = hullVertices;

	dooms::graphics::DecimateHullConservatively(hullVertices, 4);

	ASSERT_EQ(before.size(), hullVertices.size());
	for (size_t index = 0; index < before.size(); index++)
	{
		EXPECT_FLOAT_EQ(before[index].x, hullVertices[index].x);
		EXPECT_FLOAT_EQ(before[index].y, hullVertices[index].y);
		EXPECT_FLOAT_EQ(before[index].z, hullVertices[index].z);
	}
}

// Flat input does not span three dimensions, and a hull of it is
// meaningless. The mesh path answers an empty hull, which the caller
// treats as "fall back to the bounding box".
TEST(ConvexHull, DegeneratePointSetsProduceNoHull)
{
	std::vector<math::Vector3> hullVertices;

	const std::vector<math::Vector3> coplanarPoints =
	{
		math::Vector3(0.0f, 0.0f, 0.0f),
		math::Vector3(1.0f, 0.0f, 0.0f),
		math::Vector3(1.0f, 1.0f, 0.0f),
		math::Vector3(0.0f, 1.0f, 0.0f),
		math::Vector3(0.5f, 0.5f, 0.0f)
	};
	dooms::graphics::BuildConvexHullVertices(coplanarPoints, hullVertices);
	EXPECT_TRUE(hullVertices.empty());

	const std::vector<math::Vector3> threePoints =
	{
		math::Vector3(0.0f, 0.0f, 0.0f),
		math::Vector3(1.0f, 0.0f, 0.0f),
		math::Vector3(0.0f, 1.0f, 1.0f)
	};
	dooms::graphics::BuildConvexHullVertices(threePoints, hullVertices);
	EXPECT_TRUE(hullVertices.empty());

	const std::vector<math::Vector3> identicalPoints(4, math::Vector3(2.0f, 3.0f, 4.0f));
	dooms::graphics::BuildConvexHullVertices(identicalPoints, hullVertices);
	EXPECT_TRUE(hullVertices.empty());
}
