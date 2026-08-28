#include "RenderingDebuggerHelper.h"

#include "RenderingDebuggerModules/Modules/EveryCullingProfilerDebugger.h"
#include "RenderingDebuggerModules/Modules/MaskedOcclusionCullingTester.h"
#include "RenderingDebuggerModules/Modules/OverDrawVisualization.h"
#include "RenderingDebuggerModules/Modules/RendererAABBDebugger.h"

#include <EngineConfigurationData/ConfigData.h>

std::vector<dooms::graphics::RenderingDebuggerModule*> dooms::graphics::renderingDebuggerHelper::CreateDefeaultRenderingDebuggerModules()
{
	std::vector<dooms::graphics::RenderingDebuggerModule*> renderingDebuggerModules{};

	renderingDebuggerModules.emplace_back
	(
		dooms::CreateDObject<EveryCullingProfilerDebugger>()
	);

	renderingDebuggerModules.emplace_back
	(
		dooms::CreateDObject<MaskedOcclusionCullingTester>()
	);

	// Always created, unlike the other modules which used to be gated on
	// OVERDRAW_VISUALIZATION being set at startup. That gate meant the module
	// did not exist at all in a default build, so the runtime toggle had
	// nothing to talk to and silently did nothing.
	//
	// It costs nothing to have around: it holds no resources until overdraw is
	// actually switched on. OVERDRAW_VISUALIZATION now seeds the toggle instead,
	// which is what the other debugger keys already do.
	renderingDebuggerModules.emplace_back
	(
		dooms::CreateDObject<OverDrawVisualization>()
	);

	renderingDebuggerModules.emplace_back
	(
		dooms::CreateDObject<RendererAABBDebugger>()
	);

	return renderingDebuggerModules;
}
