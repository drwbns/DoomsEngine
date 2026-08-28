#pragma once

#include <Core.h>
#include <Graphics/Graphics_Core.h>
#include "../RenderingDebuggerModule.h"
#include <SingleTon/Singleton.h>

#include "OverDrawVisualization.reflection.h"
namespace dooms
{
	namespace asset
	{
		class ShaderAsset;
	}

	namespace graphics
	{
		class Material;
		class FrameBuffer;
		class PicktureInPickture;

		class D_CLASS OverDrawVisualization : public RenderingDebuggerModule, public ISingleton<OverDrawVisualization>
		{
			GENERATE_BODY()

		private:

			bool bmIsOverDrawVisualizationInitialized{ false };
			dooms::graphics::Material* mOverDrawVisualizationObjectDrawMaterial{ nullptr };
			dooms::graphics::FrameBuffer* mOverDrawVisualizationFrameBuffer{ nullptr };
			dooms::graphics::PicktureInPickture* OverDrawVisualizationPIP{ nullptr };

			void SetOverDrawVisualizationRenderingState(const bool isSet);
			void ShowOverDrawVisualizationPIP(const bool isPIPDrawed);

		public:

			void Initialize() override;

			/// <summary>
			/// Drops the frame buffer and its picture-in-picture view after the
			/// window has been resized. Both are sized from the screen when they
			/// are created, and Initialize rebuilds them lazily on next use.
			/// </summary>
			void OnResolutionChanged();
			void PreRender() override;
			void Render(dooms::Camera* const targetCamera) override;
			void PostRender() override;
			const char* GetRenderingDebuggerModuleName() override;
		};
	}
}

