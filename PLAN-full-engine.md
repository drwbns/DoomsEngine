# Plan B — From Engine Core to Shippable Engine

You made a fair correction: most heavy subsystems already exist as good
libraries, so this is mostly **integration framework**, not building from
scratch. That materially changes the cost — my earlier "years of work" estimate
assumed writing physics and audio by hand, which nobody should do.

This plan is what remains once you take the libraries as given.

## The head start most engines don't have

**The reflection system is the asset here.** Scene serialization, an editor,
undo/redo, and property inspection are normally the expensive, unglamorous half
of an engine. clReflect already gives you typed access to any `D_PROPERTY` at
runtime — which the inspector already proves by editing live components.

That means serialization is largely *"walk the reflection data and write it
out"* rather than hand-authoring a schema per type. Most hobby engines never get
this far, and it is the single strongest argument that Plan B is realistic.

## What to take off the shelf

| Subsystem | Library | Status |
| --- | --- | --- |
| Model import | **assimp** | already integrated |
| Texture / DDS | **DirectXTex** | already integrated |
| Immediate-mode UI | **Dear ImGui 1.92.9-docking** | already integrated |
| Window / input | **GLFW** (GL path) / Win32 (D3D11 path) | partly integrated |
| Math + SIMD | **JINMATH** | already yours |
| Physics (3D) | **Jolt** | to add |
| Audio | **miniaudio** | to add — importers exist, playback does not |
| Serialization format | **JSON** for scenes, binary for cooked assets | to add |
| Scripting *(optional)* | **Lua + sol2** | to add |

Deliberately **not** SDL: you already have two working platform paths. Swapping
to SDL would be a rewrite of working code to gain portability you may not need.
Revisit only if you want Linux or Mac.

## The work that is actually yours

Libraries do not give you these — this is the engine.

### 1. Finish the platform layer

A recurring pattern in this codebase is that the shell was never exercised as an
*application*. Found and fixed during one session: `WM_SIZE` commented out,
window close not wired to the game loop, `GetKeyDown` not edge-triggered,
`ClipCursor` instead of relative mouse, `EngineGUIServer::Update()` never
called, the engine-wide GUI flag used as a window close button.

Individually shallow, collectively substantial, and all of it is the difference
between a demo that runs and a tool people use for hours. Expect more of the
same class.

### 2. Scene representation and serialization

The centre of gravity for this plan.

- No save/load exists anywhere today; scenes are constructed in C++.
- Build it **on the reflection data**, not by hand.
- Needs stable object identity across a save/load round trip, and a story for
  references between objects.
- Cooked binary for runtime, readable text for source control.

### 3. Editor

You are closer than it looks: ImGui with docking is in, and the reflection
inspector already edits live objects. Missing are scene hierarchy editing,
selection, gizmos, undo/redo, and play-in-editor.

Undo/redo built on reflection is far easier than the usual command-pattern
slog — record the property, old value, new value.

### 4. Asset pipeline

Import → cook → runtime load, with dependency tracking and hot reload.
`glslcc` already handles GLSL→HLSL. What is missing is the pipeline *around*
it: knowing what is stale and rebuilding only that.

### 5. Bake the reflection data

`GENERATE_REFLECTION_DATA = TRUE` regenerates at **startup**, taking around two
minutes for a full scan. Shipping builds must bake it (`FALSE`) and load the
prebuilt `.cppbin`. The mechanism already works; it just has to become part of
the build rather than a runtime step.

### 6. Tests

~~`unit_tests` currently contains zero source files.~~ **Partly done.** There are
38 tests across seven suites now -- frustum extraction, the SIMD plane layout,
tile rounding, hull containment, level of detail boundaries, Hi-Z cell
arithmetic, sweep frame arithmetic -- passing in Debug and Release. Writing them
found a real bug: a hull decimation that claimed to be conservative and was not.

What is still uncovered is the part this plan cares about most. **Serialization
does not exist yet, so neither does its round trip test**, and a save/load bug
is silent and destroys user data. Write the test with the feature, not after.

### 7. Profiling that survives Release

~~There is no GPU timing at all.~~ **Done.** Gpu timestamp rings, per module cpu
timers, the geometry pass split into total and submission, and a frame budget
that shows cpu and gpu against frame time separately. All of it works in
Release, which is the only build worth measuring.

`D_START_PROFILING` still compiles out unless `PROFILING_RELEASE_MODE`; the
timers above sit beside it rather than replacing it.

The budget line is what makes the rest of this plan schedulable: the frame is
about 76% gpu busy, so cpu side work has roughly 0.65 ms to bid for, and any
future proposal can be checked against that before it is built rather than
after.

## Suggested order

1. **Platform layer** — cheapest, and everything else is nicer to work on after.
2. **Serialization on reflection** — unlocks the editor and content workflow.
3. **Editor basics** — hierarchy, selection, gizmos, undo/redo.
4. **Physics (Jolt)** — clean boundary, well-understood integration.
5. **Audio (miniaudio)** — smallest subsystem, importers already exist.
6. **Asset pipeline** — once there is content worth cooking.
7. **Tests and Release profiling** — alongside, not at the end.

## Risks worth naming

**clReflect is the dependency to watch** — dormant rather than deleted, which
is better than it first appears but still your problem.

- `SungJJinKang/clReflect_ForDoomsEngine` is **alive and not archived**, default
  branch `doom_engine_version`, with full history. Last actual push was
  **February 2023**; later timestamps on the repo are metadata, not commits.
- `SungJJinKang/clReflect_automation`, the C# harness that drives it, **is
  deleted**. Only local copies and the drwbns mirror survive.

So the C++ side can still be diffed or rebased against upstream if needed, but
nothing is being maintained, and the tool that runs it is gone. The prebuilt
`clscan` was built against clang 12 and had to be rebuilt from source, including
fixing a stack-corrupting off-by-one in `ConsumeToken`, quote handling for paths
with spaces, and a string reader that desynchronised the whole database on any
name longer than 1024 characters.

**One risk this plan does not name, and should.** The reflection database
describes a class layout, and nothing checks that the layout still matches. Add
a member to a reflected base class and the engine dies during startup, before
the first frame, with nothing in the log to say why -- proven by adding a single
unused `unsigned long long` and watching it happen. Every item in this plan
changes reflected types constantly, serialization most of all. A version or
layout check that fails loudly is close to a prerequisite for the rest of it.

In practice you are the maintainer. Everything in this plan leans on reflection,
so budget for that ownership rather than discovering it later.

**The fixed 1920×1080 G-buffer** is independent of screen resolution today.
Fine for a demo, wrong for a shipping renderer.

**Scope discipline.** The gap between "engine that runs my project" and "engine
other people ship on" is mostly documentation, stability, and API commitment —
none of it fun, all of it required. Consider staying at the former deliberately.

## The honest comparison

Plan A plays to what is already strong and is roughly 80% built.

Plan B is genuinely achievable given the libraries and the reflection head
start — but the remaining work is largely tooling and content workflow, not
rendering. Worth doing if building the engine *is* the goal; not worth it if the
goal is to ship a specific game, where an existing engine wins on time to
playable.

They are not exclusive: Plan A's work — visualisation, measurement, tests —
survives intact into Plan B.
