#include "DrawCallCounterGUI.h"

#include <imgui.h>
#include <EngineGUI/EngineGUIPanel.h>
#include <Rendering//RenderingDebugger/RenderingDebugger.h>
#include <Graphics/GraphicsAPI/GraphicsAPI.h>
#include <Graphics/graphicsSetting.h>

void dooms::ui::DrawCallCounterGUI::Init()
{
	Base::Init();
}

void dooms::ui::DrawCallCounterGUI::Render()
{
	if (dooms::ui::enginePanel::BeginPanel("DrawCall"))
	{
		ImGui::Text("DrawCall : %u", dooms::graphics::GraphicsAPI::GetDrawCall());
		ImGui::Text("FPS : %f", dooms::graphics::RenderingDebugger::GetSingleton()->GetFPS());

		// Beside the draw call rather than only on the visualisation panel,
		// because this is the overlay people actually watch while flying, and
		// the object count is what says whether a culling mode is earning its
		// place. The draw call number moves for reasons that have nothing to do
		// with culling.
		const unsigned int entityCount = dooms::graphics::graphicsSetting::CullStatEntityCount;
		const unsigned int culledCount = dooms::graphics::graphicsSetting::CullStatCulledCount;
		const unsigned int drawnCount = (entityCount >= culledCount) ? (entityCount - culledCount) : 0u;

		ImGui::Text("Objects : %u / %u", drawnCount, entityCount);
		ImGui::Text("Culled : %u", culledCount);

		// Only shown once a result has come back. Zero would read as free, and
		// the honest statement before the first result is that it is not known
		// yet.
		if (dooms::graphics::graphicsSetting::GpuStatHiZBuildMilliseconds > 0.0f)
		{
			ImGui::Text("Hi-Z build : %.3f ms (GPU)", dooms::graphics::graphicsSetting::GpuStatHiZBuildMilliseconds);
		}
	}
	dooms::ui::enginePanel::EndPanel();
}
