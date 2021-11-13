#pragma once

#include "FrameBuffer.h"

#include "../Graphics_Setting.h"
#include <Vector4.h>

#include "DefferedRenderingFrameBuffer.reflection.h"
namespace dooms
{
	namespace graphics
	{

		class DOOM_API D_CLASS DefferedRenderingFrameBuffer : public graphics::FrameBuffer
		{
			GENERATE_BODY()
			
			

		private:

		public:

			math::Vector4 mAlbedoClearColor;

			DefferedRenderingFrameBuffer();

			void BlitDepthBufferToScreenBuffer() const;
			void BindGBufferTextures();

			FORCE_INLINE virtual void ClearFrameBuffer() final
			{
				graphics::FrameBuffer::BindFrameBuffer();
			
				static const GraphicsAPI::eBufferType BUFFER_TYPES[3] =
				{
					GraphicsAPI::eBufferType::COLOR,
					GraphicsAPI::eBufferType::COLOR,
					GraphicsAPI::eBufferType::COLOR
				};

				static const GraphicsAPI::eBufferMode BUFFER_MODES[3] =
				{ 
					GraphicsAPI::eBufferMode::COLOR_ATTACHMENT0, 
					GraphicsAPI::eBufferMode::COLOR_ATTACHMENT1,
					GraphicsAPI::eBufferMode::COLOR_ATTACHMENT2
				};

				static const math::Vector4 ZERO_ZERO_ZERO_ONE{ 0.0f, 0.0f, 0.0f, 1.0f };
			

				static math::Vector4 TARGET_COLORS[3] =
				{
					ZERO_ZERO_ZERO_ONE,
					ZERO_ZERO_ZERO_ONE,
					math::Vector4(0, 0, 0, 0)
				};

				TARGET_COLORS[2] = Graphics_Setting::DefaultClearColor;

				GraphicsAPI::ClearSpecificBuffer(3, BUFFER_TYPES, BUFFER_MODES, TARGET_COLORS);
				GraphicsAPI::Clear(GraphicsAPI::eClearMask::DEPTH_BUFFER_BIT);

				SetTargetDrawBuffer();
			}
		};

	}
}

