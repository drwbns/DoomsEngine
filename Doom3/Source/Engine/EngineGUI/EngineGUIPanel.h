#pragma once

#include <Core.h>

namespace dooms
{
	namespace ui
	{
		// How the GUI panels are presented.
		enum class eEngineGUIDisplayMode
		{
			// Every enabled panel draws, docked into the engine dockspace.
			All,

			// Only the focused panel draws, as a translucent overlay with no
			// chrome, pinned to a corner of the viewport - the arrangement
			// Unreal uses for its stat displays.
			FocusedOverlay,

			// Nothing draws.
			Hidden
		};

		namespace enginePanel
		{
			eEngineGUIDisplayMode GetDisplayMode();
			void SetDisplayMode(const eEngineGUIDisplayMode displayMode);

			// Which panel is shown in FocusedOverlay mode, matched against the
			// name passed to BeginPanel.
			const char* GetFocusedPanelName();
			void SetFocusedPanelName(const char* const panelName);

			// 0 is fully transparent, 1 fully opaque.
			FLOAT32 GetOverlayAlpha();
			void SetOverlayAlpha(const FLOAT32 alpha);

			// Asks for the docked arrangement to be rebuilt from the default on
			// the next frame, discarding whatever the panels were dragged into.
			void RequestDockLayoutReset();

			// True once per request, for the dockspace to act on.
			bool ConsumeDockLayoutResetRequest();

			/// <summary>
			/// Opens a panel window on behalf of a GUI module, applying whatever
			/// the current display mode requires.
			///
			/// Returns true when the caller should draw its contents. EndPanel
			/// must be called afterwards either way:
			///
			///     if (enginePanel::BeginPanel("DrawCall")) { ...contents... }
			///     enginePanel::EndPanel();
			/// </summary>
			bool BeginPanel(const char* const panelName);

			void EndPanel();
		}
	}
}
