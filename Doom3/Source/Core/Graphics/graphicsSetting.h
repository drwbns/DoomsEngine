#pragma once

namespace dooms
{
	namespace graphics
	{
		namespace graphicsSetting
		{
			// Draw everything sharing a mesh, a material and a detail level in
			// one call, each copy reading its own model matrix from a per
			// instance vertex stream.
			//
			// Correct: with it on the oracle counts 1.6448 million drawn
			// pixels against 1.64492 million with it off, so every object
			// lands in the pixels it did before.
			//
			// Measured, at margin 1:
			//
			//                    off       on
			//   draws issued     742.8    413.9
			//   geometry cpu     0.196    0.173 ms
			//   submission cpu   0.113    0.094 ms
			//
			// So it halves the draws and saves about 0.023 ms, under one
			// percent of a 2.8 ms frame. The plan carried half a millisecond
			// for this, which was twenty five times too generous.
			//
			// Off, because the result is not clean rather than because it is
			// small: at margin 2 the same measurement goes the other way,
			// 0.112 ms to 0.118, so run to run variation is the size of the
			// effect. One sweep per configuration is not enough to call it.
			//
			// It does not reach the 51 draws the group count suggests, because
			// a run must share a detail level too -- one draw carries one
			// index buffer -- and that splits each group about eight ways.
			// CullStatDrawGroupCount overstates the ceiling whenever level of
			// detail is on.
			extern inline bool IsInstancingEnabled{ false };

			// How many draw calls the geometry pass actually issued, and how
			// many objects those carried. Equal means nothing was instanced.
			extern inline unsigned int CullStatInstancedDrawCallCount{ 0 };
			extern inline unsigned int CullStatInstancedObjectCount{ 0 };

			// The vertex inputs carrying per instance data are recognised by
			// name, because that is the only thing the shader reflection
			// carries that the shader author controls. Anything starting with
			// this shares one buffer slot and steps once per instance.
			constexpr const char* INSTANCE_SHADER_INPUT_NAME_PREFIX = "aInstance";

			// The vertex buffer slot that per instance stream is bound at.
			// Every other input takes the slot matching its location, so this
			// has to sit above the highest location any mesh attribute uses.
			constexpr unsigned int INSTANCE_VERTEX_BUFFER_SLOT = 12;

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

			// What the geometry pass costs the cpu, beside what it costs the
			// gpu.
			//
			// Built because instancing could not be judged without it. It
			// collapses 742.8 draws to 413.9 and the gpu timer does not move,
			// which is not evidence either way: what fewer draws save is the
			// cpu time spent issuing them, and nothing here was measuring
			// that. A technique whose whole benefit is submission cost needs a
			// submission number or it cannot be argued about honestly.
			//
			// Two numbers rather than one, because instancing moves work
			// around inside the pass as well as removing it: it adds a walk
			// over the visible set to gather matrices and a buffer upload,
			// and it removes per draw constant buffer writes and draw calls.
			// The total says whether the pass got cheaper; submission says
			// whether the part instancing targets got cheaper.
			extern inline float CpuStatGeometryPassMilliseconds{ 0.0f };
			extern inline float CpuStatDrawSubmissionMilliseconds{ 0.0f };

			// Wall time between one frame and the next.
			//
			// The denominator every other timer here was missing. Seven of
			// them report milliseconds and none of them said what a
			// millisecond is worth, so a pass taking 1.45 ms could have been
			// half the frame or a tenth of it and nothing on screen
			// distinguished those.
			//
			// This matters more than it sounds. Two techniques were pursued
			// this far on the strength of numbers that turned out to be
			// fractions of a percent of a frame -- instancing at 0.023 ms and
			// the Hi-Z staleness margin at 0.09% of drawn pixels -- and a
			// budget line would have said so before either was built rather
			// than after.
			extern inline float CpuStatFrameMilliseconds{ 0.0f };

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
			// On by default: measured in Release at 2.71 ms down to 2.58 ms, 339
			// to 355 fps. It gives up front to back ordering, which was the
			// larger cost in Debug and is the smaller one here.
			extern inline bool IsGroupDrawsByStateEnabled{ true };

			// Skip rebinding a mesh that is already bound. Does almost nothing
			// in front to back order and a great deal in grouped order.
			extern inline bool IsSkipRedundantMeshBindEnabled{ true };

			// How many mesh binds the geometry pass issued, against how many
			// draws. Equal means every draw rebound its geometry.
			extern inline unsigned int CullStatMeshBindCount{ 0 };
			extern inline unsigned int CullStatIndexBindCount{ 0 };

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

			// The other half of the oracle: what the culler got *wrong*.
			//
			// The counts above only look at objects that survived culling, so
			// they measure conservatism -- drawing what did not need drawing.
			// They cannot see the opposite error, an object culled that would
			// have been visible, because a culled object is never drawn and so
			// never queried. That error is invisible on screen too, until it
			// is a hole in the world.
			//
			// So the objects the Hi-Z tests culled are re-drawn against the
			// finished depth buffer, in the same way and at the same time. Any
			// that return samples were culled wrongly. Without this the
			// staleness margin below cannot be evaluated at all: its whole job
			// is to prevent an error nothing was counting.
			extern inline unsigned int CullStatOracleFalseCullTestedCount{ 0 };
			extern inline unsigned int CullStatOracleFalseCullCount{ 0 };

			// How much of the image those wrong culls actually cost.
			//
			// The count above treats a three pixel sliver at a screen edge and
			// a missing boulder identically, because it only asks whether an
			// object would have drawn anything at all. The occlusion query
			// already answers with how many samples passed, so keeping that
			// number is free and is the difference between a defect worth
			// stopping for and one nobody could see.
			//
			// The worst single object matters separately from the total: forty
			// slivers and one hole in a wall add up the same way and are not
			// the same bug. The drawn total is here as the scale to read them
			// against -- a number of pixels means nothing until it is beside
			// the number the frame drew.
			//
			// Samples rather than pixels if multisampling is ever switched on.
			extern inline unsigned long long CullStatOracleFalseCullPixelCount{ 0 };
			extern inline unsigned long long CullStatOracleWorstFalseCullPixelCount{ 0 };
			extern inline unsigned long long CullStatOracleDrawnPixelCount{ 0 };

			// How many cells across the Hi-Z test reads back.
			//
			// This decides how coarse the occlusion test is, and it matters
			// more than it looks: the pyramid holds the farthest depth in each
			// cell, so a single background pixel sets its whole cell to the far
			// plane and nothing behind that cell can ever be culled. At sixty
			// cells across a 1300 pixel window, one speck of visible background
			// poisons a 21 by 22 pixel block.
			//
			// 512 is the knee of the curve as re-measured in Release on a
			// frozen dense view (the 256 this default carried for a while came
			// from a Debug measurement, where the three times pricier cpu test
			// made every finer grid look bad):
			//
			//   grid  drawn  hi-z cpu  geometry  fps
			//    64    3603    0.327     3.062    299
			//   128    3336    0.540     2.840    320
			//   256    2932    0.712     2.610    345
			//   512    2553    0.845     2.446    360
			//
			// Finer still would keep culling, but the cpu test keeps growing
			// while the geometry it saves shrinks. Measured with the H key and
			// the grid line on the DrawCall panel; the view is one dense
			// frozen frame, so re-check when the scene or window size changes
			// materially.
			extern inline unsigned int HiZReadbackTargetWidth{ 512 };

			// How many cells the Hi-Z test widens an object's footprint by,
			// on every side, to absorb the staleness of the readback.
			//
			// The depth read back is at least a frame old and never waited on,
			// so the object or the camera may have moved since. A wider
			// footprint can only add cells, which can only raise the farthest
			// depth found, which can only make the test less willing to cull:
			// the error goes towards drawing something needlessly rather than
			// dropping something visible.
			//
			// It was a literal 1 at three call sites before it was a setting.
			// Making it adjustable is what lets its cost be measured against
			// the false cull count above, rather than assumed -- which is the
			// prerequisite for deciding whether two phase occlusion culling is
			// worth building to remove the staleness properly.
			extern inline unsigned int HiZStalenessMarginCells{ 1 };

			// Test occludees against the camera the read back depth was built
			// with, rather than against where the camera is now.
			//
			// The depth is at least a frame old, so comparing an object's
			// current screen rectangle against it compares two different
			// moments: when the camera turns, an object is looked up in cells
			// belonging to a part of the screen it was not in. That is a plain
			// misalignment, and it is separable from the real problem, which is
			// that an object may genuinely have been disoccluded since.
			//
			// Projecting through the stored matrix makes the test self
			// consistent -- it answers "was this occluded, in the frame this
			// depth came from" -- and leaves only the disocclusion error, which
			// is what two phase occlusion culling exists to fix. Only the hull
			// path honours this; the box path reads screen bounds PreCulling
			// computed with the current matrix and would have to reproject them
			// itself.
			//
			// Off, because measuring it did not support the idea it was built
			// on. It does cut false culls by about a third at every margin, but
			// it draws more at the same time, and compared at matched points it
			// is no better than simply widening the margin:
			//
			//   margin 1, as shipped     22.9 false culls, 742.5 drawn
			//   margin 0, reprojected    24.0 false culls, 761.5 drawn
			//   margin 2, as shipped     14.9 false culls, 777.9 drawn
			//   margin 1, reprojected    15.3 false culls, 782.3 drawn
			//
			// So misalignment was not the dominant error after all: both knobs
			// buy the same thing at the same rate, and neither reaches zero.
			// That is the argument for two phase occlusion culling rather than
			// for either of them. Kept, off, because it is the honest control
			// for that claim, and one sweep each is not enough to call the
			// small differences above real.
			extern inline bool IsHiZReprojectionEnabled{ false };

			// Set to start an automatic sweep of the margin, which holds each
			// value long enough to settle, averages what it sees, and writes a
			// row per value to hiz_margin_sweep.csv beside the executable.
			//
			// Cleared by the sweep once it has taken the request. Sweeps used
			// to be driven by hand -- press the key, read the panel, write it
			// down -- and that is how a grid table got built from mismatched
			// settings once already. Averaging over frames also matters more
			// here than it looks: the false cull count depends on where the
			// camera is, and a single frame of it says almost nothing.
			extern inline bool IsHiZMarginSweepRequested{ false };

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
			// On by default: measured in Release at 2.04 ms down to 1.72 ms and
			// 275 more objects culled, 440 to 511 fps. It was off because in a
			// Debug build the same code cost 5.7 ms of cpu to save 0.8 ms.
			extern inline bool IsHiZHullOccludeeEnabled{ true };
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

			// Draw each object at a detail level matched to its size on screen.
			//
			// The measured ceiling for this is at least 4.8 ms on a frame where
			// the geometry pass costs 15.7 ms, and it costs almost nothing per
			// object because the projected size it needs is already computed.
			// On by default: measured in Release at 2.99 ms down to 1.98 ms on the
			// geometry pass, 337 to 447 fps.
			extern inline bool IsMeshLodEnabled{ true };
			extern inline float MeshLodTrianglesPerPixel{ 1.0f };
			extern inline unsigned int MeshLodMeshCount{ 0 };
			extern inline unsigned int MeshLodLevelCount{ 0 };

			// Probe: skip the per draw model matrix write.
			//
			// Not shippable -- every object draws with whatever matrix was left
			// in the buffer, so the scene collapses into a heap. It exists to
			// price one line. Every draw does a string keyed lookup for the
			// "ModelData" buffer and then writes a 4x4 matrix into it, and at
			// about 3 microseconds a draw across three and a half thousand
			// objects, that line is a candidate for most of the frame.
			extern inline bool IsSkipPerDrawUboWriteEnabled{ false };
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

