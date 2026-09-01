#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include <Rendering/Culling/EveryCulling/EveryCulling.h>
#include <Rendering/Culling/EveryCulling/DataType/Math/Common.h>

namespace
{
	// Every test here is built around the canonical camera: at the origin,
	// looking down -Z, 90 degree field of view, square aspect, near plane
	// 1, far plane 100. That is the one frustum whose six planes are known
	// exactly, so the extraction is checked against exact values rather
	// than against a second implementation of the same formulas.
	constexpr float kSqrt2Half = 0.70710678118654752f;

	// The projection the engine itself builds for that camera: JINMATH is
	// configured with JINMATH_OPEN_GL, so depth spans -1..1 in clip space,
	// and this is glm's perspectiveRH_NO with the numbers above plugged
	// in. f = 1 / tan(fov / 2) is 1 at 90 degrees.
	culling::Mat4x4 CanonicalProjectionMatrix()
	{
		culling::Mat4x4 projection;
		for (unsigned int column = 0; column < 4; column++)
		{
			for (unsigned int row = 0; row < 4; row++)
			{
				projection[column][row] = 0.0f;
			}
		}

		projection[0][0] = 1.0f;
		projection[1][1] = 1.0f;
		projection[2][2] = -(100.0f + 1.0f) / (100.0f - 1.0f);
		projection[3][2] = -(2.0f * 100.0f * 1.0f) / (100.0f - 1.0f);
		projection[2][3] = -1.0f;

		return projection;
	}

	// The view for a camera at worldPosition looking down -Z: translate
	// the world by minus the position. What lands in the matrix is the
	// identity with (0, 0, -z) in the last column.
	culling::Mat4x4 ViewMatrixForCameraAtZ(const float cameraZ)
	{
		culling::Mat4x4 view;
		for (unsigned int column = 0; column < 4; column++)
		{
			for (unsigned int row = 0; row < 4; row++)
			{
				view[column][row] = 0.0f;
			}
		}

		view[0][0] = 1.0f;
		view[1][1] = 1.0f;
		view[2][2] = 1.0f;
		view[3][3] = 1.0f;
		view[3][2] = -cameraZ;

		return view;
	}

	void ExpectPlaneNear(const culling::Vec4& plane, const float expected[4], const float tolerance)
	{
		for (unsigned int component = 0; component < 4; component++)
		{
			EXPECT_NEAR(expected[component], plane[component], tolerance);
		}
	}

	// The exact planes of the canonical camera frustum, in the order the
	// extractors write them: left, right, top, bottom, near, far.
	void ExpectCanonicalPlanes(const culling::Vec4* planes)
	{
		const float expected[6][4] =
		{
			{ kSqrt2Half, 0.0f, -kSqrt2Half, 0.0f }, // left, the plane x = z
			{ -kSqrt2Half, 0.0f, -kSqrt2Half, 0.0f }, // right, the plane -x = z
			{ 0.0f, -kSqrt2Half, -kSqrt2Half, 0.0f }, // top, the plane -y = z
			{ 0.0f, kSqrt2Half, -kSqrt2Half, 0.0f }, // bottom, the plane y = z
			{ 0.0f, 0.0f, -1.0f, -1.0f }, // near, z <= -1
			{ 0.0f, 0.0f, 1.0f, 100.0f }, // far, z >= -100
		};

		for (unsigned int planeIndex = 0; planeIndex < 6; planeIndex++)
		{
			ExpectPlaneNear(planes[planeIndex], expected[planeIndex], 1e-3f);
		}
	}

	float SignedDistance(const culling::Vec4& plane, const float point[3])
	{
		return plane[0] * point[0] + plane[1] * point[1] + plane[2] * point[2] + plane[3];
	}

	bool IsInsideAllPlanes(const culling::Vec4* planes, const float point[3])
	{
		for (unsigned int planeIndex = 0; planeIndex < 6; planeIndex++)
		{
			if (SignedDistance(planes[planeIndex], point) < 0.0f)
			{
				return false;
			}
		}

		return true;
	}

	void ExpectTileResolution(const std::uint32_t width, const std::uint32_t height,
		const std::uint32_t expectedWidth, const std::uint32_t expectedHeight)
	{
		std::uint32_t tiledWidth = 0;
		std::uint32_t tiledHeight = 0;
		culling::GetTiledResolution(width, height, tiledWidth, tiledHeight);

		EXPECT_EQ(expectedWidth, tiledWidth);
		EXPECT_EQ(expectedHeight, tiledHeight);
	}
}

// The scalar extractor writes six planes as six plain Vec4s.
TEST(FrustumPlaneExtraction, CanonicalCameraPlanes)
{
	const culling::Mat4x4 viewProjectionMatrix = CanonicalProjectionMatrix();

	culling::Vec4 scalarPlanes[6];
	culling::ExtractPlanesFromVIewProjectionMatrix(viewProjectionMatrix, scalarPlanes, true);
	ExpectCanonicalPlanes(scalarPlanes);
}

// The SIMD extractor writes the same six planes, but transposed for the
// two-entity test: the first four slots carry one component each of the
// lateral planes across their lanes, and the last four carry the near
// and far planes duplicated into lane pairs. Reading those slots as
// planes gives an answer that is not wrong so much as not even a plane,
// which is exactly the kind of layout assumption worth pinning.
void DeinterleaveSIMDPlanes(const culling::Vec4* eightPlanes, culling::Vec4* outSixPlanes)
{
	for (unsigned int planeIndex = 0; planeIndex < 4; planeIndex++)
	{
		outSixPlanes[planeIndex] = culling::Vec4
		{
			eightPlanes[0][planeIndex],
			eightPlanes[1][planeIndex],
			eightPlanes[2][planeIndex],
			eightPlanes[3][planeIndex]
		};
	}

	for (unsigned int component = 0; component < 4; component++)
	{
		outSixPlanes[4][component] = eightPlanes[4 + component][0];
		outSixPlanes[5][component] = eightPlanes[4 + component][1];
	}
}

TEST(FrustumPlaneExtraction, SIMDLayoutOfCanonicalCameraPlanes)
{
	const culling::Mat4x4 viewProjectionMatrix = CanonicalProjectionMatrix();

	culling::Vec4 planes[8];
	culling::ExtractSIMDPlanesFromViewProjectionMatrix(viewProjectionMatrix, planes, true);

	const float expectedX[4] = { kSqrt2Half, -kSqrt2Half, 0.0f, 0.0f }; // left, right, top, bottom
	const float expectedY[4] = { 0.0f, 0.0f, -kSqrt2Half, kSqrt2Half };
	const float expectedZ[4] = { -kSqrt2Half, -kSqrt2Half, -kSqrt2Half, -kSqrt2Half };
	ExpectPlaneNear(planes[0], expectedX, 1e-3f);
	ExpectPlaneNear(planes[1], expectedY, 1e-3f);
	ExpectPlaneNear(planes[2], expectedZ, 1e-3f);
	for (unsigned int lane = 0; lane < 4; lane++)
	{
		EXPECT_NEAR(0.0f, planes[3][lane], 1e-3f); // lateral planes pass through the camera
	}

	const float expectedDepthZ[4] = { -1.0f, 1.0f, -1.0f, 1.0f }; // near, far, near, far
	const float expectedDepthW[4] = { -1.0f, 100.0f, -1.0f, 100.0f };
	ExpectPlaneNear(planes[6], expectedDepthZ, 1e-3f);
	ExpectPlaneNear(planes[7], expectedDepthW, 1e-3f);

	// Deinterleaved, they are the canonical planes again.
	culling::Vec4 deinterleaved[6];
	DeinterleaveSIMDPlanes(planes, deinterleaved);
	ExpectCanonicalPlanes(deinterleaved);

	// And they classify points the way the frustum does.
	const float insidePoint[3] = { 0.0f, 0.0f, -50.0f };
	EXPECT_TRUE(IsInsideAllPlanes(deinterleaved, insidePoint));

	const float outsidePoint[3] = { -500.0f, 0.0f, -50.0f };
	EXPECT_FALSE(IsInsideAllPlanes(deinterleaved, outsidePoint));
}

// Moving the camera has to move the planes with it, including the depth
// planes, which are the easy ones to get subtly wrong: the near plane of
// a camera at z = 4 sits at z = 3, not at -1.
TEST(FrustumPlaneExtraction, TranslatedCameraNearAndFarPlanes)
{
	const culling::Mat4x4 viewProjectionMatrix =
		CanonicalProjectionMatrix() * ViewMatrixForCameraAtZ(4.0f);

	culling::Vec4 planes[6];
	culling::ExtractPlanesFromVIewProjectionMatrix(viewProjectionMatrix, planes, true);

	const float nearPlane[4] = { 0.0f, 0.0f, -1.0f, 3.0f };
	const float farPlane[4] = { 0.0f, 0.0f, 1.0f, 96.0f };
	const float leftPlane[4] = { kSqrt2Half, 0.0f, -kSqrt2Half, 4.0f * kSqrt2Half };
	ExpectPlaneNear(planes[4], nearPlane, 1e-3f);
	ExpectPlaneNear(planes[5], farPlane, 1e-3f);
	ExpectPlaneNear(planes[0], leftPlane, 1e-3f);

	// And the moved planes still mean what they should: a point in front
	// of the moved camera is inside, a point far to its left is outside.
	const float insidePoint[3] = { 0.0f, 0.0f, -10.0f };
	EXPECT_TRUE(IsInsideAllPlanes(planes, insidePoint));

	const float outsidePoint[3] = { -50.0f, 0.0f, -4.0f };
	EXPECT_FALSE(IsInsideAllPlanes(planes, outsidePoint));

	// The SIMD extractor agrees with the scalar one on the same matrix.
	culling::Vec4 simdPlanes[8];
	culling::ExtractSIMDPlanesFromViewProjectionMatrix(viewProjectionMatrix, simdPlanes, true);
	culling::Vec4 deinterleaved[6];
	DeinterleaveSIMDPlanes(simdPlanes, deinterleaved);
	for (unsigned int planeIndex = 0; planeIndex < 6; planeIndex++)
	{
		ExpectPlaneNear(deinterleaved[planeIndex], planes[planeIndex].values, 1e-3f);
	}
}

// The corner of the canonical frustum at (-1, 0, -1) lies exactly on the
// left plane and exactly on the near plane, which is the degenerate case
// a sign error in one row of the extraction would break.
TEST(FrustumPlaneExtraction, FrustumCornerLiesOnItsPlanes)
{
	const culling::Mat4x4 viewProjectionMatrix = CanonicalProjectionMatrix();

	culling::Vec4 planes[6];
	culling::ExtractPlanesFromVIewProjectionMatrix(viewProjectionMatrix, planes, true);

	const float corner[3] = { -1.0f, 0.0f, -1.0f };
	EXPECT_NEAR(0.0f, SignedDistance(planes[0], corner), 1e-4f); // left
	EXPECT_NEAR(0.0f, SignedDistance(planes[4], corner), 1e-4f); // near
	EXPECT_TRUE(IsInsideAllPlanes(planes, corner));
}

TEST(GetTiledResolution, WholeTileSizesPassThrough)
{
	ExpectTileResolution(640, 480, 640, 480);
}

TEST(GetTiledResolution, RoundsBothAxesDown)
{
	ExpectTileResolution(1295, 757, 1280, 752);
}

TEST(GetTiledResolution, SmallerThanOneTileRoundsToZero)
{
	// Tiles are 32 wide and 8 tall.
	ExpectTileResolution(31, 7, 0, 0);
}

TEST(GetTiledResolution, OneTileOverRoundsToOneTile)
{
	ExpectTileResolution(33, 9, 32, 8);
}
// The full path the engine runs every frame, with everything but the
// frustum module switched off so the verdicts are frustum verdicts.
// Six entities, because the frustum test processes them in pairs.
TEST(ViewFrustumCulling, EndToEndAgainstCanonicalFrustum)
{
	culling::EveryCulling cullingSystem(640, 480);
	cullingSystem.SetCameraCount(1);

	cullingSystem.SetEnabledCullingModule(culling::EveryCulling::CullingModuleType::PreCulling, false);
	cullingSystem.SetEnabledCullingModule(culling::EveryCulling::CullingModuleType::MaskedSWOcclusionCulling, false);
	cullingSystem.SetEnabledCullingModule(culling::EveryCulling::CullingModuleType::DistanceCulling, false);
	cullingSystem.SetEnabledCullingModule(culling::EveryCulling::CullingModuleType::ViewFrustumCulling, true);

	struct EntityCase
	{
		float mPosition[3];
		float mHalfExtent;
		bool bmExpectCulled;
	};

	// The shipped test is an intersection test, not a containment test: the
	// SIMD comparison is dot(plane, centre) > -(radius + 0.1), with the
	// radius negated by a bitwise OR against -0.0 in
	// CheckInFrustumSIMDWithTwoPoint. A sphere is therefore kept while it
	// still reaches any plane's inside halfspace, and culled only once it is
	// entirely beyond one. That is the conservative direction -- a
	// containment test would wrongly cull an object straddling a plane and
	// partially visible -- so the near-plane straddler here is kept, and the
	// test pins that.
	const EntityCase entityCases[6] =
	{
		{ { 0.0f, 0.0f, -50.0f }, 1.0f, false }, // dead centre
		{ { 0.0f, 0.0f, 50.0f }, 1.0f, true }, // behind the camera
		{ { 0.0f, 0.0f, -500.0f }, 1.0f, true }, // past the far plane
		{ { -500.0f, 0.0f, -50.0f }, 1.0f, true }, // far out the left plane
		{ { 0.0f, 0.0f, 0.0f }, 60.0f, false }, // straddles the near plane, still kept
		{ { 30.0f, 20.0f, -50.0f }, 1.0f, false }, // inside, off the axis
	};

	std::vector<culling::EntityBlockViewer> entityViewers;
	entityViewers.reserve(6);

	const float identityModelMatrix[16] =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
	};

	for (const EntityCase& entityCase : entityCases)
	{
		const float minPosition[3] =
		{
			entityCase.mPosition[0] - entityCase.mHalfExtent,
			entityCase.mPosition[1] - entityCase.mHalfExtent,
			entityCase.mPosition[2] - entityCase.mHalfExtent
		};
		const float maxPosition[3] =
		{
			entityCase.mPosition[0] + entityCase.mHalfExtent,
			entityCase.mPosition[1] + entityCase.mHalfExtent,
			entityCase.mPosition[2] + entityCase.mHalfExtent
		};

		culling::EntityBlockViewer entityViewer = cullingSystem.AllocateNewEntity();
		ASSERT_TRUE(entityViewer.IsValid());
		entityViewer.UpdateEntityData(entityCase.mPosition, minPosition, maxPosition, identityModelMatrix);

		// The frustum module assumes every entity's bounding sphere is
		// already derived from its AABB. PreCulling is the module that
		// derives it, once per frame, and that module is switched off here
		// -- so the test derives it the same way PreCulling does.
		entityViewer.GetTargetEntityBlock()->UpdateBoundingSphereRadius(entityViewer.GetEntityIndexInBlock());

		entityViewers.push_back(std::move(entityViewer));
	}

	culling::EveryCulling::GlobalDataForCullJob globalData{};
	globalData.mViewProjectionMatrix = CanonicalProjectionMatrix();
	globalData.mFieldOfViewInDegree = 90.0f;
	globalData.mCameraNearPlaneDistance = 1.0f;
	globalData.mCameraFarPlaneDistance = 100.0f;
	globalData.mCameraWorldPosition = culling::Vec3(0.0f, 0.0f, 0.0f);
	globalData.mCameraRotation = culling::Vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
	cullingSystem.UpdateGlobalDataForCullJob(0, globalData);

	cullingSystem.PreCullJob();
	cullingSystem.ThreadCullJob(0, cullingSystem.GetTickCount());
	cullingSystem.WaitToFinishCullJob(0);

	for (unsigned int entityIndex = 0; entityIndex < 6; entityIndex++)
	{
		ASSERT_TRUE(entityViewers[entityIndex].IsValid());
		EXPECT_EQ(entityCases[entityIndex].bmExpectCulled, entityViewers[entityIndex].GetIsCulled(0))
			<< "entity " << entityIndex;
	}
}
