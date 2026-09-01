#pragma once

namespace dooms
{
	namespace graphics
	{
		/// <summary>
		/// What a sweep should do with the frame that just happened.
		/// </summary>
		struct SweepFrameResult
		{
			/// <summary>
			/// This frame's numbers belong in the current step's average.
			/// </summary>
			bool bmShouldAccumulate{ false };

			/// <summary>
			/// The step finished with this frame, so its average is complete and
			/// ready to be written out.
			/// </summary>
			bool bmIsStepComplete{ false };

			/// <summary>
			/// Which step finished, valid only when bmIsStepComplete is set.
			/// </summary>
			unsigned int mCompletedStep{ 0 };

			/// <summary>
			/// Every step has finished; nothing further will be measured.
			/// </summary>
			bool bmIsSweepComplete{ false };
		};

		/// <summary>
		/// Steps a measurement through a series of settings, holding each one
		/// long enough to settle before believing anything it says.
		///
		/// Sweeps here were hand driven for a long time -- press a key, read the
		/// panel, write the number down -- and the plan document records what
		/// that cost: a sweep labelled by press count is one lost keypress away
		/// from comparing the wrong settings against each other, which happened
		/// and produced a table that had to be thrown away. This removes the
		/// hand.
		///
		/// The settle frames exist because nothing in this pipeline answers
		/// immediately. The Hi-Z depth is read back at least a frame late, the
		/// occlusion queries behind the visibility oracle are resolved a frame
		/// after they are issued, and the gpu timers are two or three frames
		/// behind. Measuring a setting before those have flushed through
		/// measures the setting before it.
		///
		/// Pure state, so the frame arithmetic can be tested without a gpu.
		/// </summary>
		class SweepController
		{
		public:

			SweepController() = default;

			SweepController
			(
				const unsigned int settleFrameCount,
				const unsigned int measureFrameCount,
				const unsigned int stepCount
			)
				: mSettleFrameCount(settleFrameCount)
				, mMeasureFrameCount((measureFrameCount == 0) ? 1u : measureFrameCount)
				, mStepCount(stepCount)
				, bmIsFinished(stepCount == 0)
			{
			}

			/// <summary>
			/// Call once per rendered frame.
			/// </summary>
			SweepFrameResult AdvanceFrame()
			{
				SweepFrameResult result;

				if (bmIsFinished == true)
				{
					result.bmIsSweepComplete = true;
					return result;
				}

				result.bmShouldAccumulate = (mFrameInStep >= mSettleFrameCount);

				mFrameInStep++;

				if (mFrameInStep >= (mSettleFrameCount + mMeasureFrameCount))
				{
					result.bmIsStepComplete = true;
					result.mCompletedStep = mCurrentStep;

					mFrameInStep = 0;
					mCurrentStep++;

					if (mCurrentStep >= mStepCount)
					{
						bmIsFinished = true;
						result.bmIsSweepComplete = true;
					}
				}

				return result;
			}

			/// <summary>
			/// The step being measured now. Once the sweep is finished this stays
			/// at the step count, which is not a valid setting index.
			/// </summary>
			unsigned int GetCurrentStep() const
			{
				return mCurrentStep;
			}

			bool IsFinished() const
			{
				return bmIsFinished;
			}

			/// <summary>
			/// How many frames each step contributes to its average, so a caller
			/// can divide its accumulators without tracking the count itself.
			/// </summary>
			unsigned int GetMeasureFrameCount() const
			{
				return mMeasureFrameCount;
			}

		private:

			unsigned int mSettleFrameCount{ 0 };
			unsigned int mMeasureFrameCount{ 1 };
			unsigned int mStepCount{ 0 };

			unsigned int mCurrentStep{ 0 };
			unsigned int mFrameInStep{ 0 };
			bool bmIsFinished{ true };
		};
	}
}
