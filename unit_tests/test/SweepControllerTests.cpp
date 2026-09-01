#include <gtest/gtest.h>

#include <vector>

#include <Rendering/Measurement/SweepController.h>

namespace
{
	struct SweepRun
	{
		std::vector<unsigned int> mAccumulatedFramesPerStep;
		std::vector<unsigned int> mCompletedSteps;
		unsigned int mFrameCountUntilComplete{ 0 };
		bool bmCompleted{ false };
	};

	// Drives a sweep to completion the way a frame loop would, recording what
	// it was asked to do.
	SweepRun RunSweep(dooms::graphics::SweepController& sweepController,
		const unsigned int stepCount, const unsigned int frameLimit)
	{
		SweepRun run;
		run.mAccumulatedFramesPerStep.assign(stepCount, 0);

		for (unsigned int frameIndex = 0; frameIndex < frameLimit; frameIndex++)
		{
			const unsigned int stepBeforeFrame = sweepController.GetCurrentStep();
			const dooms::graphics::SweepFrameResult result = sweepController.AdvanceFrame();

			if (result.bmShouldAccumulate && stepBeforeFrame < stepCount)
			{
				run.mAccumulatedFramesPerStep[stepBeforeFrame]++;
			}

			if (result.bmIsStepComplete)
			{
				run.mCompletedSteps.push_back(result.mCompletedStep);
			}

			if (result.bmIsSweepComplete)
			{
				run.mFrameCountUntilComplete = frameIndex + 1;
				run.bmCompleted = true;
				break;
			}
		}

		return run;
	}
}

// Each step has to contribute exactly the frames it was asked for, no more:
// an average over a different number of frames than intended is the kind of
// error that produces a table nobody can reproduce.
TEST(SweepController, EveryStepAccumulatesExactlyTheMeasureFrames)
{
	dooms::graphics::SweepController sweepController(5, 10, 3);
	const SweepRun run = RunSweep(sweepController, 3, 1000);

	ASSERT_TRUE(run.bmCompleted);
	ASSERT_EQ(static_cast<size_t>(3), run.mAccumulatedFramesPerStep.size());

	for (const unsigned int accumulatedFrames : run.mAccumulatedFramesPerStep)
	{
		EXPECT_EQ(10u, accumulatedFrames);
	}
}

// The settle frames are the whole reason this exists, so they must actually
// be excluded rather than merely counted.
TEST(SweepController, SettleFramesAreNotAccumulated)
{
	dooms::graphics::SweepController sweepController(4, 2, 1);

	for (unsigned int frameIndex = 0; frameIndex < 4; frameIndex++)
	{
		const dooms::graphics::SweepFrameResult result = sweepController.AdvanceFrame();
		EXPECT_FALSE(result.bmShouldAccumulate) << "settle frame " << frameIndex;
		EXPECT_FALSE(result.bmIsStepComplete);
	}

	const dooms::graphics::SweepFrameResult firstMeasured = sweepController.AdvanceFrame();
	EXPECT_TRUE(firstMeasured.bmShouldAccumulate);
	EXPECT_FALSE(firstMeasured.bmIsStepComplete);

	const dooms::graphics::SweepFrameResult lastMeasured = sweepController.AdvanceFrame();
	EXPECT_TRUE(lastMeasured.bmShouldAccumulate);
	EXPECT_TRUE(lastMeasured.bmIsStepComplete);
	EXPECT_EQ(0u, lastMeasured.mCompletedStep);
	EXPECT_TRUE(lastMeasured.bmIsSweepComplete);
}

// Steps have to complete in order and exactly once each, since the caller
// writes one row per completion and a repeat would silently overwrite.
TEST(SweepController, StepsCompleteOnceEachInOrder)
{
	dooms::graphics::SweepController sweepController(2, 3, 4);
	const SweepRun run = RunSweep(sweepController, 4, 1000);

	ASSERT_TRUE(run.bmCompleted);
	ASSERT_EQ(static_cast<size_t>(4), run.mCompletedSteps.size());

	for (unsigned int stepIndex = 0; stepIndex < 4; stepIndex++)
	{
		EXPECT_EQ(stepIndex, run.mCompletedSteps[stepIndex]);
	}

	// Four steps of five frames each, and not one frame more.
	EXPECT_EQ(20u, run.mFrameCountUntilComplete);
}

// Once finished it must stay finished and stop asking for samples, because
// the frame loop keeps calling it.
TEST(SweepController, FinishedSweepStaysFinished)
{
	dooms::graphics::SweepController sweepController(0, 1, 1);

	const dooms::graphics::SweepFrameResult first = sweepController.AdvanceFrame();
	EXPECT_TRUE(first.bmShouldAccumulate);
	EXPECT_TRUE(first.bmIsSweepComplete);
	EXPECT_TRUE(sweepController.IsFinished());

	for (unsigned int frameIndex = 0; frameIndex < 5; frameIndex++)
	{
		const dooms::graphics::SweepFrameResult later = sweepController.AdvanceFrame();
		EXPECT_FALSE(later.bmShouldAccumulate);
		EXPECT_FALSE(later.bmIsStepComplete);
		EXPECT_TRUE(later.bmIsSweepComplete);
	}
}

// A default constructed controller is inert, so a sweep that was never
// configured cannot quietly start measuring.
TEST(SweepController, DefaultConstructedSweepDoesNothing)
{
	dooms::graphics::SweepController sweepController;

	EXPECT_TRUE(sweepController.IsFinished());

	const dooms::graphics::SweepFrameResult result = sweepController.AdvanceFrame();
	EXPECT_FALSE(result.bmShouldAccumulate);
	EXPECT_FALSE(result.bmIsStepComplete);
	EXPECT_TRUE(result.bmIsSweepComplete);
}

// Zero steps is the same as nothing to do, rather than an infinite sweep.
TEST(SweepController, ZeroStepsIsAlreadyComplete)
{
	dooms::graphics::SweepController sweepController(3, 3, 0);

	EXPECT_TRUE(sweepController.IsFinished());
	EXPECT_TRUE(sweepController.AdvanceFrame().bmIsSweepComplete);
}

// A zero measure window would divide by zero in the caller's average, so it
// is promoted to one frame rather than accepted.
TEST(SweepController, ZeroMeasureFramesIsPromotedToOne)
{
	dooms::graphics::SweepController sweepController(1, 0, 2);

	EXPECT_EQ(1u, sweepController.GetMeasureFrameCount());

	const SweepRun run = RunSweep(sweepController, 2, 100);
	ASSERT_TRUE(run.bmCompleted);
	EXPECT_EQ(1u, run.mAccumulatedFramesPerStep[0]);
	EXPECT_EQ(1u, run.mAccumulatedFramesPerStep[1]);
}
