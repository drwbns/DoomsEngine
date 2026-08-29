#pragma once

#include <Core.h>

#include "../DefaultGraphcisPipeLine.h"
#include "DeferredRenderingDrawer.h"

#include "DeferredRenderingPipeLine.reflection.h"
namespace dooms
{
	namespace graphics
	{
		class D_CLASS DeferredRenderingPipeLine : public DefaultGraphcisPipeLine
		{
			GENERATE_BODY()

		private:

			D_PROPERTY()
			DeferredRenderingDrawer mDeferredRenderingDrawer;

			// The depth buffer view, presented over the frame when it is on.
			//
			// It lives here rather than in a RenderingDebuggerModule of its own
			// because the only thing it needs is the camera's depth attachment,
			// which this class already has in hand at the right moment. A module
			// would mean a new reflected class and a project file entry to reach
			// back for the same pointer.
			class PicktureInPickture* mDepthBufferPIP{ nullptr };
			class Material* mDepthBufferPresentMaterial{ nullptr };

			/// <summary>
			/// Shows or hides the depth view, building it on first use. The depth
			/// texture belongs to the camera, so this cannot be done any earlier.
			/// </summary>
			void UpdateDepthBufferVisualization(dooms::Camera* const targetCamera);

			void CameraRender(dooms::Camera* const targetCamera, const size_t cameraIndex) override;

		public:

			DeferredRenderingPipeLine(dooms::graphics::Graphics_Server& graphicsServer);

			virtual void Initialize() final;
			virtual void LateInitialize() final;

			virtual void PreRender() final;
			virtual void Render() final;
			virtual void PostRender() final;

			virtual eGraphicsPipeLineType GetGraphicsPipeLineType() const override;
			GraphicsPipeLineCamera* CreateGraphicsPipeLineCamera() const override;
		};
	}
}

