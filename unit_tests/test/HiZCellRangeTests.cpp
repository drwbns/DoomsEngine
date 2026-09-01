#include <gtest/gtest.h>

#include <Rendering/Culling/HiZCellRange.h>

namespace
{
	void ExpectCellRange(const float minNormalised, const float maxNormalised,
		const unsigned int cellCount, const int marginInCells,
		const int expectedStart, const int expectedEnd)
	{
		int startCell = 0;
		int endCell = 0;
		dooms::graphics::GetHiZCellRange(minNormalised, maxNormalised, cellCount, marginInCells, startCell, endCell);

		EXPECT_EQ(expectedStart, startCell);
		EXPECT_EQ(expectedEnd, endCell);
	}
}

// The margin is the number that trades cull rate against correctness, so
// what it does to the range is worth pinning exactly rather than by eye.
TEST(HiZCellRange, MarginWidensBothEndsByItsOwnSize)
{
	ExpectCellRange(0.25f, 0.75f, 64, 0, 16, 48);
	ExpectCellRange(0.25f, 0.75f, 64, 1, 15, 49);
	ExpectCellRange(0.25f, 0.75f, 64, 2, 14, 50);
}

// A zero margin has to mean no widening at all, since that is the setting
// the measurement compares everything else against.
TEST(HiZCellRange, ZeroMarginIsTheBareFootprint)
{
	ExpectCellRange(0.0f, 1.0f, 32, 0, 0, 32);
	ExpectCellRange(0.5f, 0.5f, 32, 0, 16, 16);
}

// Normalised coordinates arrive from a projection that can put them outside
// the screen, and the clamp has to happen before the scaling or the margin
// lands in a different place than intended.
TEST(HiZCellRange, CoordinatesOutsideTheScreenAreClampedBeforeScaling)
{
	ExpectCellRange(-0.5f, 1.5f, 64, 0, 0, 64);
	ExpectCellRange(-0.5f, 1.5f, 64, 1, -1, 65);

	// Only the offending end is clamped.
	ExpectCellRange(-2.0f, 0.5f, 64, 0, 0, 32);
	ExpectCellRange(0.5f, 4.0f, 64, 0, 32, 64);
}

// Truncation, not rounding: a span ending nine tenths of the way into a cell
// still ends in that cell, and one starting nine tenths of the way in still
// starts there.
TEST(HiZCellRange, PartialCellsTruncateTowardsZero)
{
	ExpectCellRange(0.0f, 0.999f, 8, 0, 0, 7);
	ExpectCellRange(0.124f, 0.126f, 8, 0, 0, 1);
	ExpectCellRange(0.126f, 0.249f, 8, 0, 1, 1);
}

// The range is returned unclamped on purpose: the callers walk it with their
// own bounds, and clamping here would quietly discard a margin that reaches
// past the edge of the buffer.
TEST(HiZCellRange, RangeIsNotClampedToTheBuffer)
{
	int startCell = 0;
	int endCell = 0;
	dooms::graphics::GetHiZCellRange(0.0f, 1.0f, 16, 3, startCell, endCell);

	EXPECT_EQ(-3, startCell);
	EXPECT_EQ(19, endCell);
}

// A degenerate span still has to produce a usable single cell rather than an
// inverted range, which the callers treat as "nothing to test".
TEST(HiZCellRange, DegenerateSpanStaysOrdered)
{
	int startCell = 0;
	int endCell = 0;
	dooms::graphics::GetHiZCellRange(0.3f, 0.3f, 100, 1, startCell, endCell);

	EXPECT_LE(startCell, endCell);
	EXPECT_EQ(29, startCell);
	EXPECT_EQ(31, endCell);
}
