# Plan A — Rendering & Culling Research Harness

The original goal, and the one the codebase is already shaped for: a tool for
implementing rendering and culling techniques and **measuring them against each
other** in the same scene.

## Why this fits

Three things already here are hard to build and rare to find together:

- **Live reflection-driven inspection.** clReflect exposes any `D_PROPERTY` in
  the inspector at runtime. Tuning a culling parameter and seeing the effect
  without a rebuild is the core loop of this kind of work.
- **A culling module framework.** `EveryCulling::SetEnabledCullingModule` with
  per-module `IsEnabled` flags, plus `OnStartCullingModule` /
  `OnEndCullingModule` hooks that are begging to be timing instrumentation.
- **Two graphics backends** behind a function-pointer DLL boundary, so the same
  technique can be compared across D3D11 and OpenGL.

It also routes around every major gap: a harness needs no audio, no physics, no
scene serialization, and no shipping-grade profiler.

## Phases

### 1. Foundation

Small, and everything else leans on it.

- **Overdraw framebuffer resize.** `OverDrawVisualization` sizes its framebuffer
  from the screen at creation and is not rebuilt on resize, so it is wrong after
  any window change. Blocks phase 2.
- **Camera position/rotation readout** in the Display panel. Currently there is
  no way to read camera state; during this session that repeatedly made it
  impossible to tell camera movement from scene animation when verifying
  changes. Cheap, and it pays for itself immediately.

### 2. Visualisation

Mostly surfacing what exists rather than building new.

Already implemented but buried on a demo component
(`PortfolioComponent.cpp:35-38`):

| Toggle | Drives |
| --- | --- |
| `See_MaskedSWOcclusionCulling_Occluder` | `IsDrawMaskedOcclusionCullingBinTriangleStageDebugger` |
| `See_MaskedSWOcclusionCulling_DepthBuffer` | `IsDrawMaskedOcclusionCullingTileL0MaxDepthValueDebugger` |
| `See_MaskedSWOcclusionCulling_OccluderBoudingBox` | `IsDrawMaskedOcclusionCullingOcculderBoundingBoxDebugger` |

Also present and disabled in `config.ini`: `OVERDRAW_VISUALIZATION`,
`DRAW_MASKED_OCCLUSION_CULLING_TILE_COVERAGE_MASK_DEBUGGER`.

Work:

- A dedicated **Visualisation panel**, so these are discoverable rather than
  hidden behind a demo component's checkboxes.
- A proper **colour ramp** for the per-tile data. The binned-triangle-count and
  L0-max-depth debuggers are already per-tile, which is exactly heatmap-shaped;
  they just render as raw debug draws today.
- **Occluder visualisation**: which objects were chosen as occluders, and which
  were rejected and why.

### 3. Measurement spine

The part that turns a viewer into a harness. Without it you can implement a new
method but cannot honestly say whether it is better.

- **Per-module timing** on the existing `OnStartCullingModule` /
  `OnEndCullingModule` hooks: time per module, objects tested, objects culled.
- **Unreal-style stat display** — `stat fps`, `stat unit`, `stat detailed`,
  `stat none`. The overlay mode (F2) already exists as the presentation vehicle.
- **GPU timing.** There is none anywhere in the engine today; this is the real
  work in this phase. D3D11 timestamp queries need a ring buffer, since results
  land a frame or two late.
- Note that `D_START_PROFILING` compiles out unless `PROFILING_RELEASE_MODE`,
  and its data is averaged over a full second — right for `stat detailed`, wrong
  for per-frame `stat unit`, which needs its own lightweight path.

### 4. Method selection

- A **culling panel** driving the existing `SetEnabledCullingModule`, so methods
  can be switched at runtime and compared without a rebuild.
- Combine with phase 3 so switching a method immediately shows its cost.

### 5. Tests around the culling math

Deliberately before new techniques, not after.

`unit_tests` currently contains **zero source files**. For a harness whose
output is comparative numbers, this is the gap that undermines the results
themselves: a wrong number and a real number look identical.

The `ConsumeToken` off-by-one found in this codebase had been silently
corrupting memory for years and would have been caught by a five-line test.

Minimum worth having:

- Frustum culling: known-inside, known-outside, straddling.
- The tiled depth buffer: tile alignment, the rounding introduced by
  `EveryCulling::SetResolution`, resize correctness.
- Occludee/occluder selection at known configurations.

### 6. New techniques

With the harness in place, implement something genuinely absent to prove the
framework accommodates new ideas. **GPU Hi-Z occlusion culling** is the natural
candidate — see `PLAN-full-engine.md` for why the choice matters less than what
it exposes.

The engine culls entirely on the CPU today. A GPU method produces results
asynchronously, which will break assumptions in `CullingModule` and
`SetEnabledCullingModule`. **That is the point.** If the interface cannot absorb
it, that is worth knowing after one technique rather than five.

Reference implementations already in this workspace:

- `Hierarchical-Z-Buffer-master` — D3D11 Hi-Z, matching the engine's default API
- `gl_occlusion_culling` — NVIDIA's batched compute culling with multi-draw-indirect

## Definition of done

The harness works when you can:

1. Load a scene, pick two culling methods, and switch between them at runtime.
   — **done.** F7 cycles None / Frustum / Frustum + SW occlusion / Frustum + Hi-Z,
   with distance culling as an independent toggle so the modes stay comparable.
2. Read per-method timing and cull counts side by side.
   — **done.** The draw call overlay shows objects drawn and culled, per module
   CPU time, and GPU time for the Hi-Z build.
3. See visually *why* a method culled what it did.
   — **done.** F6 cycles occluder bounds, binned triangles, tile coverage, tile
   depth, overdraw, depth buffer, and the Hi-Z pyramid with F9 stepping levels.
4. Trust the numbers, because the math underneath has tests.
   — **partly.** `unit_tests` still has no source files. Standing in for them:
   F5 freezes the scene so two modes can be compared on one frame at SSIM
   1.000000, which caught the flipped V that made Hi-Z cull visible geometry;
   and the BVH pass now audits itself every frame against bounds it cannot have
   corrupted, reporting a count the overlay shows in red if it is ever not zero.
   The second is the shape the real tests should take.

## What the harness has actually shown

Measured on one frozen frame of 5806 objects, both modes verified to render
identical images:

| | culled | draws | culling cost |
| --- | --- | --- | --- |
| Frustum + SW occlusion | 3619 | 2199 | 0.76 ms cpu |
| Frustum + Hi-Z | 3760 | 2058 | 0.22 ms cpu + 0.11 ms gpu |

Hi-Z removes more and costs roughly a third of the CPU.

**The occluder gate does not explain the gap.** The obvious theory was that the
software culler loses because `config.ini` only lets huge, near objects be
occluders. Widening both gates — area 300000 to 10000, distance 80 to 500 —
moved it from 3480 to 3565 culled while `RasterizeOccludersStage` went from
0.61 ms to 1.19 ms. It buys 85 objects for double the cost, and still trails
Hi-Z's 3665 at a fifth of it.

The real difference is where the occluder information comes from. The software
culler has to rasterise occluders, so a wider occluder set costs it linearly.
Hi-Z reads the depth buffer the frame already produced, where every drawn object
occludes and the information was free.

### Hierarchical culling over the BVH does not pay here

Implemented behind a toggle, bound to **B**, and measured against the per object
module it replaces. Correct, and roughly two thousand times more expensive.

| | PreRender | BVH traversal | frustum module | fps |
| --- | --- | --- | --- | --- |
| tree off | 4.9 ms | | 0.086 ms | 55 |
| tree on | 164 ms | 4.0 ms | 0 ms | 5.5 |

Both render the same image, verified at SSIM 1.000000, and the audit reports
zero objects rejected by the tree that the frustum accepts. The tree is in fact
slightly *tighter* than the module. This is a comparison between two correct
implementations.

**The traversal was never the problem.** It is the 4 ms column, and even that is
beatable. The cost is keeping the tree current: 159 ms per frame, to save the
0.086 ms of frustum culling it replaces. Every one of 5806 objects moves every
frame, and each one removes and reinserts itself, restructuring the tree as it
goes. A BVH is an acceleration structure for scenes that mostly hold still, and
nothing in this scene holds still.

This was measured wrong the first time and the mistake is worth recording. The
first pass ran on a frame paused with F5, where no transform is dirty, so
maintenance never ran and the tree looked merely forty times too slow. Freezing
the scene is the right way to compare culling *results*, and the wrong way to
compare culling *costs*, because it silently zeroes the cost of staying current.
The `PreRender` timer exists now so that cost can never hide again: it was
invisible before, since the only BVH timer covered the traversal.

Two conclusions, and only the first is about BVHs:

- Hierarchical frustum culling wants static or slow geometry, far more objects,
  or a per object test expensive enough to be worth avoiding. This scene offers
  none of the three, and `ViewFrustumCulling` at 0.086 ms leaves nothing to win.
- `PreRender` costs 4.9 ms even with the tree off, which is ten times the entire
  culling budget beneath it. That is the entity block bounds update for every
  renderer, and it is now the largest CPU cost in the frame that is not drawing.

### Overdraw is not where the time is

The obvious lever against overdraw is a depth pre pass, and the engine already
had one: written, correct, and disabled since it was added. Switched on with
**P** and measured on one frozen frame of 3758 drawn objects, output identical
between the two at SSIM 1.000000.

| | draws | depth pre | geometry | gpu total | fps |
| --- | --- | --- | --- | --- | --- |
| pre pass off | 3770 | | 16.67 ms | 16.67 ms | 46 |
| pre pass on | 7528 | 8.92 ms | 13.61 ms | 22.53 ms | 37 |

It does what it claims. Laying depth down first removes overdraw shading from
the geometry pass and takes 3.05 ms off it. It costs 8.92 ms to do that, so the
frame is 5.9 ms worse.

The useful number is not the verdict but the 3.05 ms. **That is the entire
prize available to any overdraw reduction technique in this scene**, about 18%
of the geometry pass. A perfect one that cost nothing could not beat it.

Where the rest goes took three more experiments to establish, and the first
answer was wrong. It looked like draw call submission, because the depth only
pass shades nothing and still costs 53% of the full pass, and the thing it has
in common with the full pass is 3758 draw calls. That inference does not
survive measurement.

**Binds are nearly free.** Same draw order, same frame, 3724 objects:

| | mesh binds | geometry | fps |
| --- | --- | --- | --- |
| skip redundant binds | 1249 | 14.88 ms | 52 |
| issue every bind | 3736 | 15.33 ms | 52 |

2487 extra api calls cost 0.456 ms, about 0.18 us each. At that price all 3853
draw submissions come to well under a millisecond of a fifteen millisecond
pass, so submission is not the bottleneck and instancing would not fix one.

**Grouping by state loses.** Drawing objects that share a mesh and material
together collapses 1279 binds to 17, and makes the pass *slower*, 14.21 ms to
16.11 ms, because giving up front to back order costs more in lost depth
rejection than the binds were worth. Kept as a toggle rather than deleted: it
is precisely the ordering instancing would require, so its price is part of
instancing's price.

**The triangle count is the answer.** The geometry pass submits **7.85 M
triangles** for 3853 objects, against **0.92 M pixels** on screen. Two thousand
triangles per rock, and an average triangle covering roughly an eighth of a
pixel.

That is the whole story. Hardware rasterises in 2x2 quads, so a triangle
smaller than a pixel wastes most of the shading it triggers, and there are
eight times more triangles here than there are pixels to put them in. It is
also why the depth only pass costs 8.9 ms while shading nothing: it transforms
and rasterises all 7.85 M of them a second time.

So the technique this scene is asking for is **level of detail**, not
instancing and not a better occlusion test. Distant rocks drawn at full density
are the cost, and no amount of culling or batching addresses geometry that is
drawn but far too dense for the size it appears at. Front to back ordering
already recovers about 1.9 ms of overdraw, and the depth pre pass shows only
3.05 ms was there to recover in total, which puts every remaining overdraw idea
in perspective beside a fifteen millisecond pass.

The Debug caveat is sharper for this result than for the others. Debug inflates
per draw submission cost far more than it inflates shading, so the balance
between the two columns would shift toward fill in Release. The 3.05 ms ceiling
on overdraw work is measured, but how much of the remaining 13.6 ms is really
per draw overhead is not, until it is re measured.

### What perfect culling would be worth

Pressing **O** re-draws every object the geometry pass drew, against the depth
buffer it just produced, inside an occlusion query. An object returning no
samples contributed nothing to the image. That count is the exact headroom any
culling technique is competing for, and it replaces guessing at it from a
heatmap.

Three culling configurations on one frozen frame:

| culling | drawn | wasted | truly visible | triangles | geometry |
| --- | --- | --- | --- | --- | --- |
| none | 5795 | 4345 (75%) | **1450** | 12.11 M | 21.76 ms |
| frustum + sw occlusion | 3737 | 2287 (61%) | **1450** | 7.59 M | 15.27 ms |
| frustum + Hi-Z | 3177 | 1727 (54%) | **1450** | 6.39 M | 13.93 ms |

The truly visible count is identical to the object across all three, which is
the oracle checking itself: what is visible depends on the scene and the
camera, never on the technique used to look for it. Three wildly different
drawn counts agreeing on 1450 is as strong a correctness signal as this harness
has produced.

Two things fall out of the table.

**Geometry time is linear in triangles.** The three points fit
`5.2 ms + 1.37 ms per million triangles` closely. That independently confirms
the pass is bound by geometry rather than by pixels or by driver calls, from
three measurements that were not taken to test it.

**Hi-Z recovers 60% of the available waste and leaves 1727 objects on the
table.** Extrapolating the fit to a perfect culler drawing only the 1450
visible objects, roughly 3.0 M triangles, gives about **9.2 ms against the
13.9 ms it costs now**. That is a prize of the same order as level of detail,
and the two do not overlap: culling removes whole invisible objects, level of
detail thins the visible ones.

Why Hi-Z leaves so much: it tests a conservative bounding box against a coarse
mip of last frame's depth. In a field of overlapping rocks a box can easily
straddle a gap that the mesh inside it does not, and the test has to keep it.
Closing that gap means testing at finer granularity than a whole box.

The heatmap cannot answer any of this, and it is worth saying why, because it
looks like it should. `OverDrawVisualization` draws with
`SetIsDepthTestEnabled(false)` and additive blending, so the green and yellow
are counting **geometric overlap**, not fragments the hardware actually shaded.
Early Z rejects most of those before any shading happens. The heatmap shows
where objects pile up; it does not show where time goes.

### Why Hi-Z leaves invisible objects on screen

The pyramid stores the farthest depth in each cell, which is what makes the
test conservative and safe. It also means a single pixel of visible background
sets its whole cell to the far plane, and nothing behind that cell can ever be
culled again. How much damage that does depends entirely on how big a cell is,
and the cell size came from this, which was a guess:

    while (level + 1 < levelCount && GetTextureWidth(level) > 64) level++;

Sixty four cells across a 1280 wide screen makes each cell 21 by 22 pixels, so
one speck of sky poisons a 21 pixel neighbourhood. Put on **H** and swept, with
the oracle scoring each setting on one frozen frame:

| grid | drawn | wasted | Hi-Z test | triangles | geometry | fps |
| --- | --- | --- | --- | --- | --- | --- |
| 64 | 3174 | 1903 (60%) | 0.213 ms | 6.45 M | 14.43 ms | 40 |
| 128 | 2841 | 1570 (55%) | 0.264 ms | 5.77 M | 12.90 ms | 44 |
| 256 | 2619 | 1348 (52%) | 0.402 ms | 5.34 M | 12.31 ms | 46 |
| 512 | 2477 | 1206 (49%) | 0.808 ms | 5.04 M | 10.95 ms | 46 |

The oracle reports 1271 visible objects at every setting, which is what makes
the drawn column comparable at all. One constant was costing 697 objects and
3.5 ms of geometry time. The default is now 256, the knee of the curve.

**Half the waste survives the finest grid**, so cell size was never the whole
story. What is left is the shape of the test rather than its resolution:

- It tests the screen rectangle of an axis aligned box, which is substantially
  larger than the rock inside it, so it overlaps real background that the rock
  does not.
- It compares against `mAABBMinNDCZ`, the box's nearest corner. For a roughly
  spherical rock that corner sits about 0.7 radii in front of the surface, so
  a rock tucked closely behind an occluder still pokes through it.
- The one cell margin added for readback staleness inflates the rectangle
  further, though far less at 256 than it did at 64.

None of these are fixed by looking harder at the depth buffer. They are fixed
by testing something tighter than a box.

### Testing the silhouette instead of the box

Three occludee shapes, measured on one frozen frame with the oracle scoring
each. The oracle reports the same visible count under all of them, so none is
cheating:

| occludee | waste | Hi-Z test (cpu) | geometry | fps |
| --- | --- | --- | --- | --- |
| bounding box | 48.2% | 0.32 ms | 13.45 ms | 42 |
| convex hull, rectangle around it | 33.0% | 5.73 ms | 12.68 ms | 34 |
| convex hull, rasterised as a polygon | 27.1% | 30.52 ms | 9.08 ms | 18 |

The hull is built once per mesh by incremental insertion, then decimated to a
vertex budget by keeping the furthest vertex per direction bucket and pushing
the kept set outward until it contains the dropped ones. That expansion is what
keeps it correct: keeping a subset alone produces a shape inside the original,
which would cull visible objects. The exact hull of a rock is about 740
vertices, because a rock is a convex blob and nearly every vertex is on its own
hull; 24 is the budget and 17 the average kept.

**The box loses on quality and wins on time.** Every step towards a real
silhouette culls more and costs more than it saves, in this build. The polygon
is the worst of it: six points of waste for twenty five milliseconds, because
the scanline rescans every edge for every row, so the cost falls on precisely
the large objects the polygon exists to help. Walking edges incrementally down
the rows is the obvious fix and has not been tried.

**Where the hull wins, by screen coverage**, in cells, bucketed at 2, 8, 32 and
128: `0 / 0 / 407 / 444 / 45`. Nothing under eight cells is ever culled by the
hull that the box had not already kept, so a size gate is free. It is also
nearly worthless: at a 256 cell grid a cell is four pixels, so only 480 of 3501
objects are that small. The first reading of those buckets was that small
objects gain nothing, which is true, and that skipping them would be a large
saving, which is not.

### Why no test reaches zero waste

Because the test reads a max reduced pyramid, not the depth buffer. Each cell
holds the farthest depth within it, which makes the test sufficient for
occlusion and never necessary: an object is kept whenever one cell it covers
holds a far value contributed by a pixel the object does not overlap.

That survives any cell above one pixel, and a one pixel pyramid is the depth
buffer, tested per object per pixel, which is exactly what the oracle does and
exactly what a whole extra geometry pass costs. **Zero waste is the oracle's
price.** The gap between a cheap conservative test and zero is the technique,
not a defect in it. Taking the grid from 256 to 512 moved waste from 33% to
25.6%, which is that mechanism being measured directly.

## Next

1. **Tests around the culling math**, per phase 5 below. The V flip was caught by
   eye, and the BVH staleness by a screenshot; the audit added for the BVH is the
   first check in the harness that does not need a human looking at it.
2. **Two-phase occlusion culling.** The principled fix for the staleness that the
   current implementation covers with a one cell margin.
3. **Move the Hi-Z test onto the GPU.** Blocked: needs compute dispatch,
   unordered access views and indirect draw, none of which the backend has.
4. **Level of detail for the rock meshes.** 7.85 M triangles for 0.92 M pixels
   is the largest measured cost in the frame by an order of magnitude, and
   nothing else on this list competes with it.
5. **Instancing**, now measured as worth under a millisecond and therefore not
   the priority it looked like before the binds were priced.
6. **`PreRender`**, now 0.16 ms on a still frame after skipping unmoved objects,
   and still around 3 ms while everything moves and 1.0 ms while it
   is paused. The 3.9 ms difference is recomputing a world AABB per moving
   object; the 1.0 ms floor is copying bounds and a matrix into the entity block
   for all 5806 whether or not they moved.
7. **Re-measure everything in Release.** Every number in this document is from a
   Debug build, which is fine for comparing two techniques that are both scalar
   and unfair to the one that is not.

## Deliberately out of scope

Audio, physics, scene serialization, an editor, an asset pipeline, shipping
profilers. All belong to Plan B and none are needed here.
