#pragma once

#include "GraphicsAPI.h"
#include "GraphicsAPI.h"

namespace dooms
{
	namespace graphics
	{
		enum class eDepthPrePassType
		{
			Disable,
			AllOpaque
		};

		namespace graphicsAPISetting
		{
			extern int ScreenSize[2];
			/// <summary>
			/// ScreenSize Width / ScreenSize Height
			/// </summary>
			extern float ScreenRatio;
			extern unsigned int MultiSamplingNum;
			
			

			extern bool DefaultIsAlphaTestOn;

			extern bool DefaultIsBlendOn;

			extern graphics::GraphicsAPI::eBlendFactor DefaultBlendSourceFactor;
			extern graphics::GraphicsAPI::eBlendFactor DefaultBlendDestinationFactor;
			extern graphics::eDepthPrePassType DepthPrePassType;
			
			extern void LoadData();
			
			/// <summary>
			/// Updates the cached screen size after the window has been resized.
			/// Everything sized from the resolution must be rebuilt separately.
			/// </summary>
			inline void SetScreenSize(const int width, const int height)
			{
				graphicsAPISetting::ScreenSize[0] = width;
				graphicsAPISetting::ScreenSize[1] = height;
				graphicsAPISetting::ScreenRatio =
					static_cast<float>(width) / static_cast<float>(height);
			}

			inline int GetScreenWidth()
			{
				return graphicsAPISetting::ScreenSize[0];
			}

			inline int GetScreenHeight()
			{
				return graphicsAPISetting::ScreenSize[1];
			}
			
			
			inline float GetScreenRatio()
			{
				return graphicsAPISetting::ScreenRatio;
			}

			inline unsigned int GetMultiSamplingNum()
			{
				return graphicsAPISetting::MultiSamplingNum;
			}

		};

	}
}

