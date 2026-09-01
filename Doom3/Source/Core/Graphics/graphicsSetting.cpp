#include "GraphicsSetting.h"
#include "EngineConfigurationData/ConfigData.h"

void dooms::graphics::graphicsSetting::LoadData()
{
	graphicsSetting::IsDrawMaskedOcclusionCullingBinTriangleStageDebugger = ConfigData::GetSingleton()->GetConfigData().GetValue<bool>("Graphics", "DRAW_MASKED_OCCLUSION_CULLING_BIN_TRIANGLE_STAGE_DEBUGGER");
	graphicsSetting::IsDrawMaskedOcclusionCullingTileCoverageMaskDebugger = ConfigData::GetSingleton()->GetConfigData().GetValue<bool>("Graphics", "DRAW_MASKED_OCCLUSION_CULLING_TILE_COVERAGE_MASK_DEBUGGER");
	graphicsSetting::IsDrawMaskedOcclusionCullingTileL0MaxDepthValueDebugger = ConfigData::GetSingleton()->GetConfigData().GetValue<bool>("Graphics", "DRAW_MASKED_OCCLUSION_CULLING_TILE_L0_MAX_DEPTH_VALUE_DEBUGGER");
	graphicsSetting::IsOverDrawVisualizationEnabled = ConfigData::GetSingleton()->GetConfigData().GetValue<bool>("Graphics", "OVERDRAW_VISUALIZATION");

	// So the margin sweep can be run without a hand on the keyboard, which is
	// what makes it scriptable: launch, wait, read hiz_margin_sweep.csv. The
	// key is optional, and absent it stays off.
	graphicsSetting::IsHiZMarginSweepRequested = ConfigData::GetSingleton()->GetConfigData().GetValue<bool>("Graphics", "HIZ_MARGIN_SWEEP_ON_START");
}
