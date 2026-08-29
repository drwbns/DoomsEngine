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
			extern inline bool DrawRenderingBoundingBox{ false };
			extern inline float DefaultClearColor[4]{ 0.0f, 0.0f, 0.0f, 1.0f };

			// How the scene geometry is rasterised.
			enum class eRenderMode : unsigned int
			{
				Shaded,
				Wireframe
			};

			extern inline eRenderMode RenderMode{ eRenderMode::Shaded };

			void LoadData();
		}
	}
};

