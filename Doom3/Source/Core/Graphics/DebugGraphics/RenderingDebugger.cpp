#include "RenderingDebugger.h"

#ifdef DEBUG_DRAWER

#include "../Graphics_Core.h"
#include <Rendering/Renderer/RendererStaticIterator.h>
#include <Rendering/Renderer/Renderer.h>
#include <EngineGUI/PrintText.h>
#include "IO/UserInput_Server.h"



void dooms::graphics::RenderingDebugger::DrawRenderingBoundingBox()
{
	if (Graphics_Setting::DrawRenderingBoundingBox == true)
	{
		for (UINT32 layerIndex = 0; layerIndex < MAX_LAYER_COUNT; layerIndex++)
		{
			const std::vector<Renderer*>& renderersInLayer = RendererComponentStaticIterator::GetSingleton()->GetWorkingRendererInLayer(0, layerIndex);
			for (size_t rendererIndex = 0; rendererIndex < renderersInLayer.size(); rendererIndex++)
			{
				renderersInLayer[rendererIndex]->ColliderUpdater<dooms::physics::AABB3D>::DrawWorldColliderCache();
			}
		}
	}
}

void dooms::graphics::RenderingDebugger::PrintDrawCallCounter()
{
	dooms::ui::PrintText("Draw Call : %d", RenderingDebugger::PreviousFrameDrawCallCounter);
}

void dooms::graphics::RenderingDebugger::UpdateInputForPrintDrawCallCounter()
{

#ifdef DEBUG_MODE

	if (dooms::userinput::UserInput_Server::GetKeyUp(eKEY_CODE::KEY_F5))
	{
		PrintDrawCallCounter();
	}

#endif

}

#endif