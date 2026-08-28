#include "DrawCallCounterGUI.h"

#include <imgui.h>
#include <EngineGUI/EngineGUIPanel.h>
#include <Rendering//RenderingDebugger/RenderingDebugger.h>
#include <Graphics/GraphicsAPI/GraphicsAPI.h>

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
	}
	dooms::ui::enginePanel::EndPanel();
}
