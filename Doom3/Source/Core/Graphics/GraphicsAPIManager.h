#pragma once

#include "Graphics_Core.h"
#include "GraphicsAPI.h"

namespace doom
{
	namespace graphics
	{
		namespace GraphicsAPIManager
		{
			void Initialize();
			void DEBUG_CALLBACK(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* msg, const void* data);
			void SwapBuffer();
			void SetWindowTitle(const char* const title);
		};
	}
}

