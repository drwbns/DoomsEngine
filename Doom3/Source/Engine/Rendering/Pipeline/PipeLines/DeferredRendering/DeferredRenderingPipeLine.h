#pragma once

#include <Core.h>

#include "../DefaultGraphcisPipeLine.h"
#include "DeferredRenderingDrawer.h"

#include "DeferredRenderingPipeLine.reflection.h"
namespace dooms
{
	namespace asset
	{
		class TextureAsset;
	}

	namespace graphics
	{
		class D_CLASS DeferredRenderingPipeLine : public DefaultGraphcisPipeLine
		{
			GENERATE_BODY()

		private:

			D_PROPERTY()
			DeferredRenderingDrawer mDeferredRenderingDrawer;

			// The depth buffer view, presented over the frame when it is on.
			//
			// It lives here rather than in a RenderingDebuggerModule of its own
			// because the only thing it needs is the camera's depth attachment,
			// which this class already has in hand at the right moment. A module
			// would mean a new reflected class and a project file entry to reach
			// back for the same pointer.
			class PicktureInPickture* mDepthBufferPIP{ nullptr };
			class Material* mDepthBufferPresentMaterial{ nullptr };
			class PicktureInPickture* mAlbedoPIP{ nullptr };

			/// <summary>
			/// A picture-in-picture covering the whole viewport, showing one
			/// texture. Returns null if the texture is not available yet.
			/// </summary>
			class PicktureInPickture* CreateFullscreenPIP(class TextureView* const textureView);

			/// <summary>
			/// Shows or hides the depth view, building it on first use. The depth
			/// texture belongs to the camera, so this cannot be done any earlier.
			/// </summary>
			void UpdateDepthBufferVisualization(dooms::Camera* const targetCamera);

			/// <summary>
			/// Shows or hides the unlit albedo view for the textured render mode.
			/// </summary>
			void UpdateAlbedoVisualization(dooms::Camera* const targetCamera);

			// The hierarchical depth pyramid: one R32F texture whose top level is
			// a copy of the camera depth buffer and whose every lower level is
			// the maximum of the four texels above it.
			//
			// One frame buffer per level, all pointing at the same texture at a
			// different mip, because that is the only way to render a chain.
			dooms::asset::TextureAsset* mHiZTexture{ nullptr };
			std::vector<class FrameBuffer*> mHiZFrameBuffers{};

			// One view per level, each covering only that level, so reading a
			// level never overlaps the level being written.
			std::vector<class TextureView*> mHiZSourceViews{};
			class Material* mHiZCopyMaterial{ nullptr };
			class Material* mHiZDownsampleMaterial{ nullptr };
			class PicktureInPickture* mHiZPIP{ nullptr };
			class Mesh* mHiZQuadMesh{ nullptr };
			class Material* mHiZPresentMaterial{ nullptr };
			UINT32 mHiZLevelCount{ 0 };

			// A cpu readable copy of one coarse level of the pyramid.
			//
			// Copied at the end of one frame and mapped at the start of a later
			// one, never waited on. Reading it back in the frame that produced it
			// would stall the cpu until the gpu had caught up, which costs more
			// than any culling it could inform would save.
			unsigned long long mHiZReadbackTexture{ 0 };
			UINT32 mHiZReadbackLevel{ 0 };
			UINT32 mHiZReadbackTargetWidthInUse{ 0 };
			UINT32 mHiZReadbackWidth{ 0 };
			UINT32 mHiZReadbackHeight{ 0 };
			bool bmIsHiZReadbackPending{ false };

			// The last level that arrived, kept so the occlusion test can read it
			// without mapping again.
			std::vector<FLOAT32> mHiZReadbackData{};
			bool bmIsHiZReadbackDataValid{ false };
			UINT64 mHiZFrameCounter{ 0 };

			/// <summary>
			/// Takes last frame's copy if it has arrived, and queues this frame's.
			/// </summary>
			void ReadBackHiZLevel();

			/// <summary>
			/// Reports what a Hi-Z occlusion test would decide, without acting on
			/// it.
			///
			/// Measuring before culling because the two inputs are in depth
			/// spaces that may not match: the entity blocks carry a min NDC z
			/// written by PreCulling, and the pyramid holds whatever the depth
			/// buffer holds. Guessing wrong culls things that are visible, which
			/// is the failure that is hard to notice and easy to ship.
			/// </summary>
			void ApplyHiZOcclusionCulling(const size_t cameraIndex);

			/// <summary>
			/// Counts what survived culling, whichever techniques were involved.
			///
			/// Runs after every culling module and after the Hi-Z pass, so it sees
			/// the final set. Deliberately technique agnostic: the number that
			/// makes two modes comparable is how many objects are left, not how
			/// each one arrived at that.
			/// </summary>
			void UpdateCullStatistics(const size_t cameraIndex);

			// Reused between frames so a traversal does not allocate. Indexed by
			// BVH node index.
			std::vector<bool> mBVHVisibleNodes{};
			std::vector<INT32> mBVHTraversalStack{};

			/// <summary>
			/// Rejects whole subtrees of the renderer BVH against the frustum.
			///
			/// The tree is built and kept up to date already -- every Renderer
			/// inserts itself into it -- but nothing has ever culled with it, so
			/// every module walks all of the objects instead.
			/// </summary>
			void ApplyBVHFrustumCulling(dooms::Camera* const targetCamera, const size_t cameraIndex);

			// One frame's worth of gpu timing: a disjoint query bracketing the
			// work, and a timestamp either side of it.
			//
			// Three frames of them in rotation, because a result is not ready in
			// the frame that issued it and asking for one that is not ready either
			// stalls or returns nothing.
			struct GpuTimerFrame
			{
				unsigned long long mDisjointQuery{ 0 };
				unsigned long long mStartQuery{ 0 };
				unsigned long long mEndQuery{ 0 };
				bool bmIsPending{ false };
			};

			static constexpr UINT32 GPU_TIMER_FRAME_COUNT = 3;

			/// <summary>
			/// A rotating set of the above, measuring one span of gpu work.
			///
			/// One of these per thing worth timing. Comparing two rendering
			/// techniques means being able to time whatever the next one turns out
			/// to need, so this is a value to declare rather than a pass to
			/// hand write.
			/// </summary>
			struct GpuTimerRing
			{
				GpuTimerFrame mFrames[GPU_TIMER_FRAME_COUNT]{};
				UINT32 mFrameIndex{ 0 };
				bool bmAreQueriesCreated{ false };
			};

			GpuTimerRing mHiZGpuTimer{};
			GpuTimerRing mGeometryPassGpuTimer{};
			GpuTimerRing mDepthPrePassGpuTimer{};

			/// <summary>
			/// Opens a gpu timing span, and publishes whichever earlier frame's
			/// result has become available into destinationMilliseconds.
			///
			/// The result written is two or three frames old. That is inherent to
			/// asking a gpu how long it took without stalling on the answer.
			/// </summary>
			/// <summary>
			/// One occlusion query per object drawn, grown as needed and reused.
			/// </summary>
			std::vector<unsigned long long> mVisibilityOracleQueries;
			UINT32 mVisibilityOraclePendingCount{ 0 };

			/// <summary>
			/// Re-draws everything the geometry pass drew, against the depth
			/// buffer it produced, counting which objects turn out to have
			/// contributed no pixels at all.
			///
			/// That count is the exact answer to how much better any culling
			/// technique could have done on this frame, which is otherwise a
			/// matter of opinion.
			/// </summary>
			void MeasureTrueVisibility(dooms::Camera* const targetCamera, const size_t cameraIndex);

			void BeginGpuTimer(GpuTimerRing& gpuTimerRing, FLOAT32& destinationMilliseconds);
			void EndGpuTimer(GpuTimerRing& gpuTimerRing);

			/// <summary>
			/// Draws a quad covering the target with whatever material is bound.
			/// The deferred drawer's own quad cannot be borrowed for this: it
			/// binds its own material before drawing.
			/// </summary>
			void DrawHiZQuad();

			/// <summary>
			/// Builds the pyramid for this frame, creating it on first use.
			/// Must run after the geometry pass has filled the depth buffer and
			/// before anything that wants to read the pyramid.
			/// </summary>
			void BuildHiZPyramid(dooms::Camera* const targetCamera);

			/// <summary>
			/// Shows or hides the pyramid, so it can be seen rather than trusted.
			/// </summary>
			void UpdateHiZVisualization();

			void CameraRender(dooms::Camera* const targetCamera, const size_t cameraIndex) override;

		public:

			DeferredRenderingPipeLine(dooms::graphics::Graphics_Server& graphicsServer);

			virtual void Initialize() final;
			virtual void LateInitialize() final;

			virtual void PreRender() final;
			virtual void Render() final;
			virtual void PostRender() final;

			virtual eGraphicsPipeLineType GetGraphicsPipeLineType() const override;
			GraphicsPipeLineCamera* CreateGraphicsPipeLineCamera() const override;
		};
	}
}

