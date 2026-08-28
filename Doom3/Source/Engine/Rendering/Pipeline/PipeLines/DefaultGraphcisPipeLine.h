#pragma once

#include <Core.h>

#include "../GraphicsPipeLine.h"
#include "Vector3.h"
#include "../RenderingCullingManager.h"
#include "Rendering/RenderingDebugger/RenderingDebugger.h"

#include "DefaultGraphcisPipeLine.reflection.h"
namespace dooms
{
	class Renderer;
	namespace graphics
	{
		class Material;

		class D_CLASS DefaultGraphcisPipeLine : public GraphicsPipeLine
		{
			GENERATE_BODY()

		private:

			D_PROPERTY()
			graphics::Material* DepthOnlyMaterial;

		protected:

			void PreRenderRenderer();
			void PushFrontToBackSortJobToJobSystem(dooms::Camera* const targetCamera, const UINT32 cameraIndex, std::atomic<bool>* bIsFinihsed);
			static void FrontToBackSort(const math::Vector3& CameraPos, const UINT32 cameraIndex);

			void DrawRenderersWithDepthOnly(dooms::Camera* const targetCamera, const size_t cameraIndex) const;
			void DrawRenderers(dooms::Camera* const targetCamera, const size_t cameraIndex) const;
			void ConditionalDrawRenderers
			(
				dooms::Camera* const targetCamera, 
				const size_t cameraIndex, 
				const std::function<bool(const dooms::Renderer* const)> ConditionFunc
			) const;
			void DrawBatchedRenderers() const;
			virtual void CameraRender(dooms::Camera* const targetCamera, const size_t cameraIndex)  = 0;

			graphics::Material* GetDepthOnlyMaterial() const;

		public:

			D_PROPERTY()
			RenderingCullingManager mRenderingCullingManager;

			/// <summary>
			/// Applies a window resize recorded by the graphics backend, if there
			/// is one. Called at the top of PreRender, where the previous frame's
			/// cull jobs have finished and this frame's have not started.
			/// </summary>
			void ApplyPendingResolutionChange();

			D_PROPERTY()
			RenderingDebugger mRenderingDebugger;

			DefaultGraphcisPipeLine(dooms::graphics::Graphics_Server& graphicsServer);

			void Initialize() override;
			void LateInitialize() override;
			void PreRender() override;
			void Render() override;
			void PostRender() override;
		};
	}
}
