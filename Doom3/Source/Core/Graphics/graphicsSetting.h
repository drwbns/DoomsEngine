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

