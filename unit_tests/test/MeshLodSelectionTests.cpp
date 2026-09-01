#include <gtest/gtest.h>

#include <vector>

#include <Rendering/Lod/MeshLod.h>

// MeshLod.h pulls in the DObject headers through BufferID, and those
// carry a global container whose constructor normally lives in
// DObjectManager.cpp. Nothing here exercises the DObject system, so the
// constructor is stubbed rather than linking the subsystem it belongs
// to. Remove this if that subsystem ever joins the test project.
dooms::DObjectsContainer::DObjectsContainer()
{
}

namespace
{
	// A chain the way the engine builds them: level zero is the full mesh,
	// each following level a simplification with fewer indices. The index
	// counts are the only thing selection looks at.
	std::vector<dooms::graphics::MeshLodLevel> ChainWithIndexCounts(const std::vector<unsigned long long>& indexCounts)
	{
		std::vector<dooms::graphics::MeshLodLevel> levels;
		levels.reserve(indexCounts.size());

		for (const unsigned long long indexCount : indexCounts)
		{
			dooms::graphics::MeshLodLevel level{};
			level.mIndexCount = indexCount;
			levels.push_back(level);
		}

		return levels;
	}

	void ExpectSelectedLevel(const std::vector<dooms::graphics::MeshLodLevel>& levels,
		const unsigned long long affordableIndexCount, const unsigned int expectedLevelIndex)
	{
		EXPECT_EQ(expectedLevelIndex,
			dooms::graphics::SelectMeshLodLevelIndex(levels, affordableIndexCount));
	}
}

// The selector answers: the coarsest level that still has at least as
// many indices as the object's screen coverage can resolve. These tests
// pin the boundaries, which is where the previous fall-off-the-end
// behaviour went wrong.

TEST(MeshLodSelection, NothingAffordableFallsBackToLevelZero)
{
	const auto levels = ChainWithIndexCounts({ 100000, 40000, 8000, 1000 });
	ExpectSelectedLevel(levels, 150000, 0);
}

TEST(MeshLodSelection, FinestLevelPickedWhenItExactlyMatches)
{
	const auto levels = ChainWithIndexCounts({ 100000, 40000, 8000, 1000 });
	ExpectSelectedLevel(levels, 100000, 0);
}

TEST(MeshLodSelection, CoarserLevelPickedWhenFinerOnesAreTooBig)
{
	const auto levels = ChainWithIndexCounts({ 100000, 40000, 8000, 1000 });
	ExpectSelectedLevel(levels, 8001, 1);
}

TEST(MeshLodSelection, BoundaryIsInclusive)
{
	const auto levels = ChainWithIndexCounts({ 100000, 40000, 8000, 1000 });
	ExpectSelectedLevel(levels, 8000, 2);
	ExpectSelectedLevel(levels, 1000, 3);
}

TEST(MeshLodSelection, JustUnderTheCoarsestIndexCountStillPicksIt)
{
	const auto levels = ChainWithIndexCounts({ 100000, 40000, 8000, 1000 });
	ExpectSelectedLevel(levels, 999, 3);
}

TEST(MeshLodSelection, ZeroCoveragePicksTheCoarsestLevel)
{
	const auto levels = ChainWithIndexCounts({ 100000, 40000, 8000, 1000 });
	ExpectSelectedLevel(levels, 0, 3);
}

TEST(MeshLodSelection, SingleLevelChainAlwaysAnswersZero)
{
	const auto levels = ChainWithIndexCounts({ 500 });
	ExpectSelectedLevel(levels, 600, 0);
	ExpectSelectedLevel(levels, 100, 0);
}

TEST(MeshLodSelection, EmptyChainAnswersZeroWithoutCrashing)
{
	const auto levels = ChainWithIndexCounts({});
	ExpectSelectedLevel(levels, 1000, 0);
}
