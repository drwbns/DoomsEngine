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
			bool bmIsFirstPassReported{ false };
			dooms::graphics::Material* mOverDrawVisualizationObjectDrawMaterial{ nullptr };
			dooms::graphics::Material* mOverDrawVisualizationPresentMaterial{ nullptr };
			dooms::graphics::FrameBuffer* mOverDrawVisualizationFrameBuffer{ nullptr };
			dooms::graphics::PicktureInPickture* OverDrawVisualizationPIP{ nullptr };

			void SetOverDrawVisualizationRenderingState(const bool isSet);
			void ShowOverDrawVisualizationPIP(const bool isPIPDrawed);

			/// <summary>
			/// Builds the frame buffer, materials and picture-in-picture on first
			/// use, and again after a resize has dropped them. Cheap to call.
			/// </summary>
			void EnsureResourcesCreated();

		public:

			void Initialize() override;

			/// <summary>
			/// Redirects drawing into the overdraw buffer, with every renderer
			/// forced onto a material that adds a fixed amount per fragment.
			/// Whatever the caller draws between Begin and End is counted.
			///
			/// Depth testing and depth writes are off for the pass: the measure
			/// is how many times a pixel gets shaded, so layers a depth test
			/// would have rejected still have to count. That makes this a second
			/// pass over the scene rather than something that can share the
			/// g-buffer pass.
			/// </summary>
			void BeginOverDrawPass();

			/// <summary>
			/// Restores the previous render state and puts the result on screen.
			/// </summary>
			void EndOverDrawPass();

			/// <summary>
			/// Takes the result back off screen. Safe before anything has been
			/// initialised, which is the usual case since overdraw is off.
			/// </summary>
			void HideOverDrawVisualization();

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

