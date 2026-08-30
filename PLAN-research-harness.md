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

Implemented behind a toggle and measured on one frozen frame of 5806 objects.
Correct, and slower by a factor of forty.

| | culled | draws | frustum cost | fps |
| --- | --- | --- | --- | --- |
| per object frustum module | 5601 | 217 | 0.050 ms | 141 |
| BVH tree traversal | 5610 | 208 | 1.97 ms | 112 |

Both render the same image, verified at SSIM 1.000000, and the audit described
below reports zero objects rejected by the tree that the frustum accepts. The
tree is in fact slightly *tighter* than the module — those nine objects are
genuinely outside the frustum and contribute no pixels — so this is a real
comparison between two correct implementations, not a bug.

Two reasons it loses, and only one of them is the build:

- **The traversal saves work the implementation then spends anyway.** Rejecting
  a subtree avoids testing its objects, but the results are still applied by
  looping over all 5806 renderers to ask each one whether its node survived.
  The pass is O(objects) regardless of how much the tree pruned, so the
  asymptotic win never arrives. Fixing this means walking surviving leaves
  rather than polling every renderer.
- **It is scalar C++ against SIMD across threads.** `ViewFrustumCulling` is
  vectorised and multithreaded; the traversal is a pointer-chasing loop with a
  `std::vector<bool>` cleared to node capacity every frame. This is a Debug
  build, which punishes that shape far more than the code it is competing with,
  so the forty times figure is an upper bound on the gap, not a measurement of
  it. **Re-measure in Release before treating the margin as real.**

The honest conclusion for this scene is narrower than "BVHs are slow": 5806
objects spread evenly through the view give few subtrees that fall wholly
outside the frustum, which is the only case the hierarchy is paid for. The
technique wants either far more objects or far more spatial clustering than
this scene has.

The maintenance cost is not in the number above. Keeping the tree current runs
in `Renderer::PreRender`, and the frame it was measured on was paused, so
nothing was dirty and it was free. In a moving scene it is not.

## Next

1. **Tests around the culling math**, per phase 5 below. The V flip was caught by
   eye, and the BVH staleness by a screenshot; the audit added for the BVH is the
   first check in the harness that does not need a human looking at it.
2. **Two-phase occlusion culling.** The principled fix for the staleness that the
   current implementation covers with a one cell margin.
3. **Move the Hi-Z test onto the GPU.** Blocked: needs compute dispatch,
   unordered access views and indirect draw, none of which the backend has.
4. **Re-measure everything in Release.** Every number in this document is from a
   Debug build, which is fine for comparing two techniques that are both scalar
   and unfair to the one that is not.

## Deliberately out of scope

Audio, physics, scene serialization, an editor, an asset pipeline, shipping
profilers. All belong to Plan B and none are needed here.
