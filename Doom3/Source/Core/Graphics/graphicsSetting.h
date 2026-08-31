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

			// The visibility oracle: what a perfect culler would have achieved.
			//
			// Every drawn object is re-drawn against the finished depth buffer
			// inside an occlusion query, so an object that returns no samples
			// contributed nothing to the image and should not have been drawn.
			// Expensive by design and off by default; it exists to put a number
			// on the headroom a culling technique is competing for, rather than
			// leaving it to be guessed from a heatmap.
			extern inline bool IsVisibilityOracleEnabled{ false };
			extern inline unsigned int CullStatOracleTestedCount{ 0 };
			extern inline unsigned int CullStatOracleInvisibleCount{ 0 };

			// How many cells across the Hi-Z test reads back.
			//
			// This decides how coarse the occlusion test is, and it was picked
			// out of the air. It matters more than it looks: the pyramid holds
			// the farthest depth in each cell, so a single background pixel sets
			// its whole cell to the far plane and nothing behind that cell can
			// ever be culled. At sixty cells across a 1280 wide screen, one
			// speck of visible background poisons a 21 by 22 pixel block.
			// 256 is the knee of the measured curve. Going 64 to 512 culls 697
			// more objects and takes 3.5 ms off the geometry pass; most of that
			// arrives by 256, and 512 costs twice the cpu for another tenth of a
			// frame. The original 64 was a guess and cost about 3 ms.
			extern inline unsigned int HiZReadbackTargetWidth{ 256 };

			// Two probes for attributing the waste the finest grid still leaves.
			//
			// Neither is conservative, so neither is shippable: they deliberately
			// cull things that may be visible, and the oracle is what catches it.
			// They exist to answer which half of the box test is costing the
			// most, before any effort goes into replacing it. Shrinking the
			// rectangle stands in for a tighter silhouette; pushing the depth
			// back stands in for a bounding sphere's nearer surface instead of
			// the box's protruding corner.
			extern inline float HiZProbeRectangleShrink{ 0.0f };
			extern inline float HiZProbeDepthPush{ 0.0f };

			// Test the mesh's convex hull instead of its bounding box.
			//
			// The hull contains the mesh, so the test stays conservative, but it
			// gives the true nearest depth rather than a box corner sticking out
			// in front of the object, and a screen rectangle that follows the
			// mesh rather than the box around it.
			extern inline bool IsHiZHullOccludeeEnabled{ false };
			extern inline unsigned int CullStatHullMeshCount{ 0 };
			extern inline unsigned int CullStatHullVertexCount{ 0 };

			// Vertices kept per hull. The exact hull of a rock is around 740,
			// which is unaffordable to project per object per frame.
			extern inline unsigned int HiZHullVertexBudget{ 24 };

			// Rasterise the projected hull as a polygon instead of testing the
			// rectangle around it.
			//
			// Off, because it was measured and it loses. It takes waste from 33%
			// to 27% and the test from 5.7 ms to 30.5 ms, which is six points of
			// waste for twenty five milliseconds. The scanline rescans every edge
			// for every row it covers, so the cost lands on exactly the large
			// objects the polygon was meant to help. Kept because the rewrite
			// that would fix it -- walking edges incrementally down the rows
			// rather than rescanning -- is worth trying against this number.
			extern inline bool IsHiZHullPolygonEnabled{ false };
			extern inline unsigned int HiZHullPolygonMinCellCount{ 16 };

			// Where the hull wins, by how many cells the object covers.
			//
			// Culling a distant rock saves exactly as many triangles as culling a
			// near one, so if the hull's extra culls turn out to be small distant
			// objects, spending more hull vertices on near ones would discard the
			// benefit along with the cost.
			extern inline unsigned int CullStatHullCullsBySize[5]{ 0, 0, 0, 0, 0 };

			// Below this many cells the hull is not tested at all.
			//
			// Every object the hull ever culled covered at least eight cells, and
			// nothing under that was culled even once, so the gate costs nothing.
			// It also saves little: at a 256 cell grid a cell is about four
			// pixels, so only 480 of 3501 objects are under eight cells at all.
			// The first reading of that bucket data was that small objects gain
			// nothing from hulls, which is true, and that skipping them would be
			// a large saving, which is not.
			extern inline unsigned int HiZHullMinCellCount{ 8 };
			extern inline unsigned int CullStatHullTestedCount{ 0 };
			extern inline unsigned int CullStatHullSkippedCount{ 0 };

			// What a perfect level of detail scheme would have submitted.
			//
			// For every object drawn, the smaller of the triangles it actually
			// has and the pixels it actually covers. Nothing is built to achieve
			// this: it is the ceiling, measured before any mesh simplifier is
			// written, because the geometry pass is linear in triangles and this
			// says how far down that line a perfect scheme could reach.
			extern inline unsigned long long CullStatIdealIndexCount{ 0 };
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

