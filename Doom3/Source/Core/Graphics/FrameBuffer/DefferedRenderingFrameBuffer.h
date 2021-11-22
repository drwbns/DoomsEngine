#pragma once

#include "FrameBuffer.h"

#include "../Graphics_Setting.h"
#include <Vector4.h>

#include "DefferedRenderingFrameBuffer.reflection.h"
namespace dooms
{
	class Camera;
	namespace graphics
	{

		class DOOM_API D_CLASS DefferedRenderingFrameBuffer : public graphics::FrameBuffer
		{
			GENERATE_BODY()
			
			

		private:

		public:

			math::Vector4 mAlbedoClearColor{1.0f};

			DefferedRenderingFrameBuffer();

			void BlitDepthBufferToScreenBuffer() const;
			void BindGBufferTextures();

			virtual void ClearFrameBuffer(const Camera* const camera) final;
		};

	}
}

