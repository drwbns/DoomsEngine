#include "OverDrawVisualizationDebugger.h"

#include <Graphics/graphicsSetting.h>


void dooms::OverDrawVisualizationDebugger::InitComponent()
{
	ShowIsSortObjectFrontToBack();
}

void dooms::OverDrawVisualizationDebugger::ShowIsSortObjectFrontToBack()
{
	if (dooms::graphics::graphicsSetting::IsSortObjectFrontToBack == true)
	{
		D_RELEASE_LOG(eLogType::D_LOG, "Sorting Object Front to Back On");
	}
	else
	{
		D_RELEASE_LOG(eLogType::D_LOG, "Sorting Object Front to Back Off");
	}
}

void dooms::OverDrawVisualizationDebugger::UpdateComponent()
{
	// F5 used to toggle front to back sorting from here, and F6 overdraw. Both
	// are checkboxes on the visualisation panel now, and F5 pauses the scene, so
	// leaving these would fight with the keys that own them.
	//
	// This is also why they had to move: a component that only exists in one
	// demo scene is no place to keep engine wide shortcuts.
}
