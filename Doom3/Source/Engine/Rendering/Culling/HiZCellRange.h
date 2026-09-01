#pragma once

namespace dooms
{
	namespace graphics
	{
		/// <summary>
		/// The inclusive range of Hi-Z cells an object's screen span covers,
		/// widened by the staleness margin.
		///
		/// The depth being tested against is a frame or more old -- the
		/// readback never waits on the gpu -- so an object may have moved, or
		/// the camera may have. Widening the range can only bring in more
		/// cells, which can only raise the farthest depth found, which can
		/// only make the test less willing to cull. The error goes towards
		/// drawing something needlessly rather than dropping something
		/// visible.
		///
		/// Pure arithmetic, in its own header, because the margin is the one
		/// number in the Hi-Z test that trades cull rate against correctness
		/// and it was previously a literal 1 repeated at each call site. The
		/// returned range is deliberately *not* clamped to the buffer: callers
		/// already clamp per axis as they walk it, and clamping here would
		/// hide a margin that reaches past the edge.
		/// </summary>
		inline void GetHiZCellRange
		(
			const float minNormalised,
			const float maxNormalised,
			const unsigned int cellCount,
			const int marginInCells,
			int& outStartCell,
			int& outEndCell
		)
		{
			const float clampedMin = (minNormalised < 0.0f) ? 0.0f : minNormalised;
			const float clampedMax = (maxNormalised > 1.0f) ? 1.0f : maxNormalised;

			outStartCell = static_cast<int>(clampedMin * static_cast<float>(cellCount)) - marginInCells;
			outEndCell = static_cast<int>(clampedMax * static_cast<float>(cellCount)) + marginInCells;
		}
	}
}
