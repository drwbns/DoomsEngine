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

