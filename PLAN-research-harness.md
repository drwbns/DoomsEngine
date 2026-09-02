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

`unit_tests` contained **zero source files**. For a harness whose output is
comparative numbers, that was the gap undermining the results themselves: a
wrong number and a real number look identical.

The `ConsumeToken` off-by-one found in this codebase had been silently
corrupting memory for years and would have been caught by a five-line test.

Now covered, 25 tests over three files:

- Frustum plane extraction, against a canonical camera whose six planes are
  known exactly rather than against a second copy of the same formulas, plus
  the SIMD extractor's transposed storage layout.
- The end-to-end frustum job: known-inside, known-outside, straddling.
- The tiled depth buffer's tile alignment and the rounding `SetResolution`
  introduces, extracted into `GetTiledResolution` so it is testable without
  constructing a culling system.
- The convex hull's containment property, exact and decimated.
- Level of detail selection boundaries.

Both discoveries are written up in the `Next` section: the frustum test is an
intersection test rather than a containment one, and the hull decimation was
not conservative despite its name.

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

### What level of detail is worth, before building any of it

The geometry pass submits about eight triangles for every pixel on screen, and
its cost is linear in triangles at `5.2 ms + 1.37 ms per million`. So the
question "how much would Nanite style detail selection buy" has an arithmetic
answer, and it can be had without writing a mesh simplifier.

For every drawn object, take the smaller of the triangles it has and the pixels
it covers, at one triangle per pixel, which is the point past which extra
geometry cannot be seen. Summed over the frame that is the ceiling:

    triangles 8.12 M, ideal 4.58 M, geometry 15.65 ms

The model predicts 16.3 ms for 8.12 M against 15.65 ms measured, so it holds.
At 4.58 M it predicts 11.5 ms, which makes level of detail worth **at least
4.8 ms** on this frame.

At least, because the ideal above is an over estimate. Coverage is taken from
each object's screen aligned bounding rectangle, which is larger than the rock
inside it, double counts where objects overlap, and credits fully occluded
objects with the pixels they would have covered. Every one of those pushes the
ideal up, so the true figure is lower and the saving larger.

That is the largest single item measured in this document, it costs almost
nothing per object -- projected screen size is already computed by PreCulling
-- and it compounds with culling rather than competing with it, since the
objects culling fails to remove get cheaper too.

**What of Nanite is reachable.** Not the compute rasteriser for sub pixel
triangles, not gpu driven cluster culling, not the visibility buffer: the
backend has no compute dispatch, no unordered access views and no indirect
draw. What is reachable is the part that matters here, which is selecting
detail by projected screen error rather than by authored distance bands, and
this scene has only 15 distinct meshes to build detail levels for. What is
given up without the cluster hierarchy is seamlessness: discrete levels pop.

## Every technique, re-measured in Release

Debug inflates cpu side driver work about three times and leaves gpu work
alone, which is enough to invert the ranking of almost everything in this
document. Each row below is one frozen frame with the toggle as the only
change, and the Debug column is what this document said before.

| technique | Debug | Release |
| --- | --- | --- |
| level of detail | slower | **2.706 to 1.949 ms, 339 to 458 fps** |
| group draws by state | 14.21 to 16.11 ms, a loss | **2.706 to 2.575 ms, 339 to 355 fps** |
| convex hull occludee | 5.7 ms cpu to save 0.8 ms | **2.042 to 1.724 ms, 440 to 511 fps** |
| depth pre pass | a loss | still a loss, 339 to 251 fps |
| hull outline as polygon | 30.5 ms cpu, a loss | still a loss, 533 to 371 fps |

Three of five inverted. The two that did not are the pre pass, which doubles
geometry submission, and the polygon outline, whose scanline rescans every edge
for every row it covers. Release makes the polygon three times cheaper, 30.5 ms
to 1.9 ms, and it is still more than the whole geometry pass it is trying to
shorten. Both lose for reasons that are structural rather than a matter of
constant factors, which is why the build did not rescue them.

Fitting the level of detail pair: geometry is about **0.15 us per draw plus
0.56 ms per million triangles**, so on a 4.2 ms pass triangles account for 3.7
and draw submission for 0.5. Under Debug the same fit put draw submission in
front, which is why level of detail looked worthless and instancing looked like
the biggest prize on the board. Neither was true.

**What this costs the rest of the document.** Every conclusion above this
section was measured in Debug. The ones about culling *quality* stand, because
counts of objects and waste do not depend on the build: the oracle's 1450
visible objects, the Hi-Z grid sweep taking waste from 60% to 49%, the hull
halving what the box leaves. The ones about *cost* do not stand, and the
per draw figures in particular are roughly three times too large.

### The Hi-Z grid knee, re-measured in Release

The last Debug-era measurement left standing. One frozen dense frame, 5806
objects, the grid the only change:

| grid | drawn | hi-z test cpu | geometry gpu | fps |
| --- | --- | --- | --- | --- |
| 64 | 3603 | 0.327 ms | 3.062 ms | 299 |
| 128 | 3336 | 0.540 ms | 2.840 ms | 320 |
| 256 | 2932 | 0.712 ms | 2.610 ms | 345 |
| 512 | 2553 | 0.845 ms | 2.446 ms | 360 |

Monotone in every column, and all four rows fit the cost model above to
within 3%. The knee moved from 256 to 512: Release halves the cpu price of
the test, so the extra culling a finer grid buys now pays for itself. The
default follows the measurement. Finer than 512 keeps culling, but the test
keeps growing while the geometry it saves shrinks.

The first run of this sweep produced the opposite verdict — finer grids
culling *less* — and it was wrong. The captures were labeled by counting H
key presses, two presses silently never reached the engine, and the
mislabeled columns were only consistent with the gate's cell arithmetic
under exactly one relabeling. The panel now prints the pause state, the
culling mode and the grid width on every frame, so a capture carries its own
conditions, and the key script refuses to send anything unless it has
verified the engine owns the foreground — keystrokes delivered to whatever
window has focus are a hazard to the rest of the desktop as much as to the
measurement.

## What the engine now starts with

The settings that won, verified together rather than only one at a time, on one
frozen frame of a dense view:

| | drawn | binds | triangles | geometry | fps |
| --- | --- | --- | --- | --- | --- |
| what it used to boot with | 3548 | 1327 | 7.21 M | 4.478 ms | 209 |
| what it boots with now | 3053 | 17 | 4.00 M | 2.686 ms | 339 |

40% off the geometry pass and 62% more frames. They stack: Hi-Z and the hull
occludee cull 495 more objects between them, level of detail takes 45% of the
triangles off what remains, and grouping by state collapses 1327 mesh binds to
17.

On by default: level of detail, the convex hull occludee, grouping by state,
skipping unmoved objects, skipping redundant mesh binds, a 512 cell Hi-Z grid,
and the frustum plus Hi-Z culling mode. The grid was 256 when this table was
captured; the sweep above moved it, and strictly in the direction the table
already points.

Off by default, each because it was measured and lost: the depth pre pass, the
polygon outline, and BVH frustum culling.

The culling mode is the one thing here imposed rather than read back. config.ini
boots into frustum plus software occlusion, and everything else the interface
shows is whatever the engine actually has, deliberately. A harness that starts
worse than the best it knows about is not a useful starting point, so this one
setting is applied over the file.

## Next

Ordered by what would change a decision, not by size.

### Correctness

1. ~~**Tests around the culling math.**~~ **Done.** `unit_tests` now holds 25
   tests over three files, all passing, linking the whole EveryCulling library
   standalone: frustum plane extraction against a canonical camera's exact
   planes, the SIMD extractor's transposed storage layout, tile rounding, the
   hull's containment property, and the level of detail selection boundaries.

   Two things came out of writing them. The frustum test is an *intersection*
   test, not a containment one — `CheckInFrustumSIMDWithTwoPoint` compares
   `dot(plane, centre) > -(radius + margin)`, the radius negated by a bitwise
   OR against `-0.0` — so a sphere straddling a plane is kept, which is the
   conservative direction. And `DecimateHullConservatively` was not
   conservative: its radial heuristic let 14 of 60 hull vertices fall outside
   the decimated hull, which would have culled visible objects wherever the
   proxy was used as an occludee. It now scales the kept hull about its own
   centroid by the exact factor its face planes require. That bug had been
   sitting under a function whose name asserted the opposite, and nothing but
   a test was ever going to find it.

2. **Two-phase occlusion culling.** The principled fix for the readback
   staleness that a one cell margin currently papers over. **Now measured, and
   the margin does not paper over it: it is a correctness item first and a
   conservatism one second.**

   The visibility oracle only ever queried objects that *survived* culling, so
   it measured drawing what did not need drawing and was structurally blind to
   the opposite error. It now also re-draws everything the Hi-Z tests culled,
   against the same finished depth buffer: any samples mean the object would
   have been visible and the cull was wrong. `T` runs the sweep unattended and
   writes `hiz_margin_sweep.csv`; `I` steps the margin by hand.

   Release, grid 512, spawn view, camera turning at about 36 degrees a second
   through the same arc on every step, 60 measured frames after 30 settling:

   | margin | drawn | false culls | wasted | hi-z cpu | geometry gpu |
   |--------|-------|-------------|--------|----------|--------------|
   | 0      | 673.5 | 36.8        | 207.6  | 0.703    | 1.457        |
   | 1      | 742.5 | 22.9        | 256.8  | 0.762    | 1.513        |
   | 2      | 777.9 | 14.9        | 280.4  | 0.838    | 1.513        |

   So the shipped default wrongly culls around 23 objects every frame while the
   camera turns, and nothing was counting them.

   **How much that costs the image, measured after the fact: almost nothing.**
   The count above treats a sliver at a screen edge and a hole in a wall
   identically, so the query's sample count is now kept too:

   | margin | false culls | pixels lost | worst single | drawn pixels |
   |--------|-------------|-------------|--------------|--------------|
   | 0      | 38.0        | 2331        | 694          | 1,643,480    |
   | 1      | 23.0        | 1440        | 650          | 1,644,770    |
   | 2      | 14.9        | 904         | 657          | 1,645,700    |

   At the shipped default that is 1440 pixels out of 1.64 million drawn, 0.09%,
   averaging 63 pixels an object; the worst single object across sixty frames
   covered 650, about 25 by 25. These are distant specks and edge slivers, not
   holes. The earlier description of them as holes in the image was wrong, and
   was written before anything measured severity.

   Which resets the conclusion. The margin prevents an error that is real and
   almost invisible, and costs about 70 objects of cull rate to do it -- objects
   small enough that the geometry pass does not measurably notice: 1.407, 1.387
   and 1.488 ms across the three margins is noise. Both sides of this trade are
   small, so leaving the margin at 1 is defensible and so is 2.
   Raising the margin helps and does not converge -- it buys roughly a third
   fewer errors per step while costing about 5% of the objects drawn and 8% of
   the Hi-Z cpu time -- because the error is stale *data*, not a footprint that
   is too narrow. No margin can fix that; only testing against depth from this
   frame can.

   Held still, the same sweep reports zero false culls at every margin
   including zero, which is why this went unnoticed: with nothing moving there
   is no staleness to absorb, and the margin looks like pure waste. That the
   two readings disagree is the point of measuring under motion.

   The counter was validated against a deliberately unconservative
   configuration (`HiZProbeRectangleShrink` at 0.30), where it reports 17.98,
   6.47 and 3.02 false culls for the three margins: it fires, and it falls with
   margin, which is what a staleness margin should do.

   The cheaper thing was tried first, and did not work. The test compares an
   object's *current* screen rectangle against depth from an *older* camera,
   which is a straight misalignment, so the readback now carries the view
   projection matrix that built it and the hull path can project through that
   instead (`IsHiZReprojectionEnabled`, off). Objects reaching outside the
   older frame are left alone, since the part that was off screen has no depth
   to be judged by.

   It cuts false culls by about a third at every margin -- and draws more while
   doing it. Compared at matched points it is not better than widening the
   margin:

   | configuration        | false culls | drawn |
   |----------------------|-------------|-------|
   | margin 1, as shipped | 22.9        | 742.5 |
   | margin 0, reprojected| 24.0        | 761.5 |
   | margin 2, as shipped | 14.9        | 777.9 |
   | margin 1, reprojected| 15.3        | 782.3 |

   So misalignment was not the dominant error. Both knobs buy error reduction
   at the same exchange rate and neither reaches zero, which says the residue
   is genuine disocclusion: things that were hidden when the depth was captured
   and are not hidden now. No amount of widening or realigning a test against
   old depth can see those. Only depth from this frame can, which is what two
   phase occlusion culling is.

   One sweep per configuration, so the small differences in that table are not
   separable from run to run noise; the shape of the result is what matters.

   That leaves the real fix needing a same frame depth buffer to test against,
   and the two ways to get one are a synchronous readback -- a full stall every
   frame, the cost this design exists to avoid -- or testing on the gpu, which
   is item 5. So item 5 is a prerequisite for this item rather than an
   alternative to it.

   **But on this evidence neither is urgent.** The staleness costs 0.09% of the
   drawn pixels and a cull rate difference the geometry pass cannot be measured
   to care about. Two phase occlusion culling is back to being an optimisation
   with a small payoff on this scene, not a correctness fix, and it should be
   ordered against instancing and the rest on that basis. Worth revisiting if a
   scene ever turns up where the same measurement reads differently -- faster
   camera motion, moving occluders, or objects large enough on screen that one
   wrong cull is a hole rather than a speck.

   A note on what item 5 is actually blocked on, since the note that it wants
   D3D12 or Vulkan is only half right. The DX11 backend already creates and
   binds compute shaders (`CreateComputeShader`, `CSSetShader`,
   `CSSetShaderResources`). What it has no entry point for is `Dispatch`,
   unordered access views, and `DrawIndexedInstancedIndirect` -- all three of
   which D3D11 itself supports. So the gpu side test is blocked on three
   functions in the graphics dll rather than on an api port. The fully gpu
   driven form, where the gpu builds its own draw list at scale, is the part
   that wants a newer api.

### Measurement debt

3. **Rewrite the older sections of this document.** The headline tables are
   corrected and the inversions recorded, but Debug era prose survives in
   places and some of it is now known to be wrong.

### Performance, in measured order

4. **Instancing.** Built, correct, measured, and off. Worth about 0.023 ms,
   which is one percent of a frame and twenty five times less than the half
   millisecond this list carried for it.

   Correctness is measured rather than asserted: with it on the oracle counts
   1.6448 million drawn pixels against 1.64492 million with it off, so every
   object lands in the pixels it did before.

   | margin 1          | off    | on     |
   |-------------------|--------|--------|
   | draws issued      | 742.8  | 413.9  |
   | geometry cpu      | 0.196  | 0.173  |
   | submission cpu    | 0.113  | 0.094  |
   | geometry gpu      | 1.447  | 1.505  |

   Off because the result is not clean rather than because it is small: at
   margin 2 the same measurement reverses, 0.112 ms to 0.118, so run to run
   variation is the size of the effect. One sweep per configuration.

   Two things had to be built before this could be judged at all. The **cpu
   timer around the geometry pass**, split into total and submission, because
   what instancing saves is submission cost and the gpu timer structurally
   cannot see it -- the gpu does the same work either way. And the draw group
   count turned out to overstate the ceiling: a run has to share a detail
   level as well as a mesh and a material, since one draw carries one index
   buffer, and that splits each group about eight ways. 743 collapses to 414,
   not to 51.

   The crash that cost most of a session was none of the things it looked
   like. `DefaultGraphcisPipeLine` is a reflected class and this build loads a
   prebuilt reflection database, so adding a member moves every offset that
   database describes and the engine dies during startup, before the first
   Present. Proven by adding one unused `unsigned long long`, touched by
   nothing, and watching it die. **Do not add members to a reflected class
   while the reflection data is prebuilt.** The instance buffer state lives at
   file scope in the .cpp instead.

   One regression shipped and was caught by these numbers rather than by eye:
   the shader reads its matrix from the instance stream unconditionally, so
   gating the path that binds that stream on the toggle drew the whole scene
   at whatever was left in the buffer -- 2371 objects instead of 743, 4.39
   million pixels instead of 1.645 million. The oracle noticed; nobody
   watching the screen had.

5. **Move the Hi-Z test onto the gpu.** Blocked: needs compute dispatch,
   unordered access views and indirect draw, none of which the backend has.
   The fully gpu driven form of this wants D3D12 or Vulkan.

### Build and infrastructure

6. **The clReflect tools are now copied by the build, not by hand.** Half
   done, and the half that is done was the one silently undoing itself.

   The fork's fixed tools live in
   `clReflect_ForDoomsEngine/binary/bin/Release`, and only they handle paths
   containing spaces -- which this checkout has. They were being copied into
   the output directory by hand, so every clean rebuild restored the stock
   pair from `clReflect_automation` over the top and the fix quietly went
   away. A post build step now copies the fork's tools last, in both Debug and
   Release, and echoes a warning if they are missing rather than leaving the
   failure to be discovered at runtime. Verified by deleting all three dlls
   and rebuilding: they come back.

   **A clean clone still cannot regenerate reflection data**, for two reasons
   worth stating separately:

   - The tools are build output and the submodule ignores them (`release/` in
     its .gitignore, added deliberately by `f34389df`). The *source* fix is
     committed -- `80009acb`, `f419b2ef`, `8eb7e184`, `aef5f0c6` -- so a clean
     clone gets correct source and no binaries.
   - `reflection_binary_Release_x64.cppbin` is not committed either, so there
     is nothing to fall back on.

   Two ways out, and the choice is about repository size rather than
   difficulty. Committing the built tools costs about 69 MB and matches how
   assimp, oneTBB and glslcc are already vendored and copied by post build
   steps. Scripting the CMake build costs nothing in the repository and adds a
   build dependency on CMake and clang, plus the time to build a 35 MB scanner
   that embeds clang. Nothing here can decide that; it is a call about the
   repository.

7. **`clexport -map` still crashes** on a Debug map file, parsing 2,875
   character lines into 1,024 byte buffers. Pre-existing clReflect bug, worked
   around by running clexport without `-map`, which loses function call
   addresses.

8. ~~**The startup timing instrumentation is permanent.**~~ **Done.** It now
   writes `startup_timing.txt` only when `DOOMS_STARTUP_TIMING` is set in the
   environment, and deletes a stale file only when it is about to write one.

   An environment variable rather than a config key because the first phase
   being timed is the one that reads the config: by the time a key could be
   consulted, the measurement it would gate has already happened. Verified
   both ways -- no file on a default run, the timings present when asked.


## Deliberately out of scope

Audio, physics, scene serialization, an editor, an asset pipeline, shipping
profilers. All belong to Plan B and none are needed here.
