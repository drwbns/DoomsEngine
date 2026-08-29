#pragma once

#include <Core.h>

#include "../DefaultGraphcisPipeLine.h"
#include "DeferredRenderingDrawer.h"

#include "DeferredRenderingPipeLine.reflection.h"
namespace dooms
{
	namespace asset
	{
		class TextureAsset;
	}

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
			class PicktureInPickture* mAlbedoPIP{ nullptr };

			/// <summary>
			/// A picture-in-picture covering the whole viewport, showing one
			/// texture. Returns null if the texture is not available yet.
			/// </summary>
			class PicktureInPickture* CreateFullscreenPIP(class TextureView* const textureView);

			/// <summary>
			/// Shows or hides the depth view, building it on first use. The depth
			/// texture belongs to the camera, so this cannot be done any earlier.
			/// </summary>
			void UpdateDepthBufferVisualization(dooms::Camera* const targetCamera);

			/// <summary>
			/// Shows or hides the unlit albedo view for the textured render mode.
			/// </summary>
			void UpdateAlbedoVisualization(dooms::Camera* const targetCamera);

			// The hierarchical depth pyramid: one R32F texture whose top level is
			// a copy of the camera depth buffer and whose every lower level is
			// the maximum of the four texels above it.
			//
			// One frame buffer per level, all pointing at the same texture at a
			// different mip, because that is the only way to render a chain.
			dooms::asset::TextureAsset* mHiZTexture{ nullptr };
			std::vector<class FrameBuffer*> mHiZFrameBuffers{};
			class Material* mHiZCopyMaterial{ nullptr };
			class Material* mHiZDownsampleMaterial{ nullptr };
			class PicktureInPickture* mHiZPIP{ nullptr };
			class Mesh* mHiZQuadMesh{ nullptr };
			class Material* mHiZPresentMaterial{ nullptr };
			UINT32 mHiZLevelCount{ 0 };

			/// <summary>
			/// Draws a quad covering the target with whatever material is bound.
			/// The deferred drawer's own quad cannot be borrowed for this: it
			/// binds its own material before drawing.
			/// </summary>
			void DrawHiZQuad();

			/// <summary>
			/// Builds the pyramid for this frame, creating it on first use.
			/// Must run after the geometry pass has filled the depth buffer and
			/// before anything that wants to read the pyramid.
			/// </summary>
			void BuildHiZPyramid(dooms::Camera* const targetCamera);

			/// <summary>
			/// Shows or hides the pyramid, so it can be seen rather than trusted.
			/// </summary>
			void UpdateHiZVisualization();

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

