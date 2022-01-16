#include "../Input/GraphicsAPIInput.h"

namespace dooms
{
	namespace input
	{
		namespace GraphicsAPIInput
		{
			DOOMS_ENGINE_GRAPHICS_API GRAPHICS_INITIALIZEGRAPHICSAPIINPUT InitializeGraphisAPIInput{nullptr};
			DOOMS_ENGINE_GRAPHICS_API GRAPHICS_DEINITIALIZEGRAPHICSAPIINPUT DeInitializeGraphisAPIInput{nullptr};
			DOOMS_ENGINE_GRAPHICS_API GRAPHICS_POLLEVENTS PollEvents{nullptr};
			DOOMS_ENGINE_GRAPHICS_API GRAPHICS_SETCURSORMODE SetCursorMode{nullptr};
			DOOMS_ENGINE_GRAPHICS_API GRAPHICS_SETCURSORENTERCALLBACK SetCursorEnterCallback{nullptr};
			DOOMS_ENGINE_GRAPHICS_API GRAPHICS_SETCURSORPOSITIONCALLBACK SetCursorPosition_Callback{nullptr};
			DOOMS_ENGINE_GRAPHICS_API GRAPHICS_SETSCROLLCALLBACK SetScroll_Callback{nullptr};
			DOOMS_ENGINE_GRAPHICS_API GRAPHICS_SETKEYCALLBACK SetKey_Callback{nullptr};
			DOOMS_ENGINE_GRAPHICS_API GRAPHICS_SETMOUSEBUTTONCALLBACK SetMouseButton_Callback{nullptr};
			DOOMS_ENGINE_GRAPHICS_API GRAPHICS_GETKEYCURRENTACTION GetKeyCurrentAction{nullptr};
		}
	}
}