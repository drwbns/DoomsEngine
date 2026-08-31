#pragma once

namespace dooms
{
	namespace graphics
	{
		namespace graphicsSetting
		{
			extern inline bool IsSortObjectFrontToBack{ true };
			extern inline bool IsDrawDebuggersEnabled{ true };
			extern inline bool IsDrawMaskedOcclusionCullingBinTriangleStageDebugger{ false };
			extern inline bool IsDrawMaskedOcclusionCullingTileCoverageMaskDebugger{ false };
			extern inline bool IsDrawMaskedOcclusionCullingTileL0MaxDepthValueDebugger{ false };
			extern inline bool IsDrawMaskedOcclusionCullingOcculderBoundingBoxDebugger{ false };
			extern inline bool IsOverDrawVisualizationEnabled{ false };
			extern inline bool IsDepthBufferVisualizationEnabled{ false };
			extern inline bool IsHiZVisualizationEnabled{ false };

			// Which level of the hierarchical depth pyramid the view shows.
			// Stepped through with F9, so the chain can be inspected rather than
			// assumed correct.
			extern inline unsigned int HiZVisualizationLevel{ 0 };

			// Whether the Hi-Z test actually culls, as opposed to only reporting
			// what it would cull. Driven by the F7 cycle.
			extern inline bool IsHiZOcclusionCullingEnabled{ false };

			// Written by the pipeline once culling has finished, read by the
			// interface. What a culling mode is worth is the number of objects it
			// removes, and until now that had to be inferred from the draw call
			// count, which moves for other reasons too.
			extern inline unsigned int CullStatEntityCount{ 0 };
			extern inline unsigned int CullStatCulledCount{ 0 };

			// Milliseconds the gpu spent building the Hi-Z pyramid, from gpu
			// timestamps rather than cpu clocks. Zero until the first result
			// comes back, which is a frame or two after the work was issued.
			extern inline float GpuStatHiZBuildMilliseconds{ 0.0f };

			// Cpu milliseconds spent testing every object against the read back
			// pyramid. The counterpart to the gpu build time above: Hi-Z spends on
			// both processors and only the pair of numbers describes its cost.
			extern inline float CpuStatHiZTestMilliseconds{ 0.0f };

			// Reject whole subtrees of the renderer BVH against the frustum,
			// instead of testing every object one at a time. A toggle rather than
			// a replacement, so it can be measured against what it replaces.
			extern inline bool IsBVHFrustumCullingEnabled{ false };

			// Forces every renderer to push its current bounds into the tree on
			// the next frame, regardless of whether its transform reports dirty.
			//
			// The tree is normally maintained from transform dirty flags, which
			// means it only becomes correct for objects that have moved since it
			// was last used. Switching culling on therefore has to start from a
			// full refresh: otherwise the first frames cull against wherever
			// objects were when the tree last ran, and while the scene is paused
			// nothing is dirty at all, so it would never correct itself.
			extern inline bool IsBVHFullRefreshRequested{ true };
			extern inline float CpuStatBVHCullMilliseconds{ 0.0f };

			// Objects the tree rejected that the per object frustum test kept.
			//
			// Both test the same boxes against the same frustum, so the tree can
			// only ever cull a subset of what the per object pass culls. Anything
			// here is the tree culling something visible, which is precisely the
			// failure that stale bounds produced, and it should read zero.
			extern inline unsigned int CullStatBVHDisagreementCount{ 0 };

			// Wall time of the per renderer pre render pass.
			//
			// Here because the BVH's real price is not its traversal but keeping
			// the tree current, which happens in that pass and so appeared in no
			// timer at all: the overlay reported a few milliseconds for a change
			// that cost a hundred and seventy. Measured for the whole pass rather
			// than per renderer, because timing 5806 objects individually costs
			// more than the thing being timed.
			extern inline float CpuStatPreRenderRendererMilliseconds{ 0.0f };

			// Gpu time for the passes that actually touch pixels.
			//
			// Overdraw only matters because it costs time, so a technique that
			// claims to reduce it has to be judged on these rather than on how
			// the heatmap looks.
			extern inline float GpuStatGeometryPassMilliseconds{ 0.0f };
			extern inline float GpuStatDepthPrePassMilliseconds{ 0.0f };

			// Skip rewriting an entity's culling data when its transform did not
			// move. A toggle rather than an unconditional change, so the saving
			// can be read off rather than asserted.
			extern inline bool IsSkipUnchangedCullingDataEnabled{ true };

			// How many draws the geometry pass would issue if every object
			// sharing a mesh and a material were drawn together, beside how many
			// it issues now. The ratio is the ceiling on what instancing could
			// win, and is worth knowing before building it.
			extern inline unsigned int CullStatDrawGroupCount{ 0 };
			extern inline unsigned int CullStatDrawnRendererCount{ 0 };

			// Draw in mesh and material order instead of front to back.
			//
			// A trade rather than an improvement, which is why it is off by
			// default: it collapses redundant binds, and it gives up the depth
			// rejection that front to back ordering buys. Measured, the two can
			// be compared instead of argued about.
			extern inline bool IsGroupDrawsByStateEnabled{ false };

			// Skip rebinding a mesh that is already bound. Does almost nothing
			// in front to back order and a great deal in grouped order.
			extern inline bool IsSkipRedundantMeshBindEnabled{ true };

			// How many mesh binds the geometry pass issued, against how many
			// draws. Equal means every draw rebound its geometry.
			extern inline unsigned int CullStatMeshBindCount{ 0 };

			// Indices submitted by the geometry pass.
			//
			// The one number that separates a pass held up by talking to the
			// driver from one held up by transforming vertices, which are very
			// different problems with very different answers.
			extern inline unsigned long long CullStatIndexCount{ 0 };
			extern inline bool DrawRenderingBoundingBox{ false };
			extern inline float DefaultClearColor[4]{ 0.0f, 0.0f, 0.0f, 1.0f };

			// How the scene geometry is rasterised.
			enum class eRenderMode : unsigned int
			{
				Shaded,
				Wireframe,

				// Albedo straight out of the g-buffer, with no lighting applied.
				Textured
			};

			extern inline eRenderMode RenderMode{ eRenderMode::Shaded };

			void LoadData();
		}
	}
};

