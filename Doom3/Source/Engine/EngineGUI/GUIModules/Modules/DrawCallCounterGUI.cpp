#include "DrawCallCounterGUI.h"

#include <imgui.h>
#include <EngineGUI/EngineGUIPanel.h>
#include <Rendering//RenderingDebugger/RenderingDebugger.h>
#include <Graphics/GraphicsAPI/GraphicsAPI.h>
#include <Graphics/graphicsSetting.h>
#include <DObject/DObjectGlobals.h>
#include <Rendering/Pipeline/GraphicsPipeLine.h>
#include <Rendering/Pipeline/PipeLines/DefaultGraphcisPipeLine.h>

void dooms::ui::DrawCallCounterGUI::Init()
{
	Base::Init();
}

void dooms::ui::DrawCallCounterGUI::Render()
{
	if (dooms::ui::enginePanel::BeginPanel("DrawCall"))
	{
		ImGui::Text("DrawCall : %u", dooms::graphics::GraphicsAPI::GetDrawCall());
		ImGui::Text("FPS : %f", dooms::graphics::RenderingDebugger::GetSingleton()->GetFPS());

		// Beside the draw call rather than only on the visualisation panel,
		// because this is the overlay people actually watch while flying, and
		// the object count is what says whether a culling mode is earning its
		// place. The draw call number moves for reasons that have nothing to do
		// with culling.
		const unsigned int entityCount = dooms::graphics::graphicsSetting::CullStatEntityCount;
		const unsigned int culledCount = dooms::graphics::graphicsSetting::CullStatCulledCount;
		const unsigned int drawnCount = (entityCount >= culledCount) ? (entityCount - culledCount) : 0u;

		ImGui::Text("Objects : %u / %u", drawnCount, entityCount);
		ImGui::Text("Culled : %u", culledCount);

		// Only shown once a result has come back. Zero would read as free, and
		// the honest statement before the first result is that it is not known
		// yet.
		if (dooms::graphics::graphicsSetting::GpuStatHiZBuildMilliseconds > 0.0f)
		{
			ImGui::Text("Hi-Z build : %.3f ms (GPU)", dooms::graphics::graphicsSetting::GpuStatHiZBuildMilliseconds);
		}

		if (dooms::graphics::graphicsSetting::CpuStatHiZTestMilliseconds > 0.0f)
		{
			ImGui::Text("Hi-Z test  : %.3f ms (CPU)", dooms::graphics::graphicsSetting::CpuStatHiZTestMilliseconds);
		}

		// What instancing could collapse the geometry pass to, if it existed.
		if (dooms::graphics::graphicsSetting::CullStatDrawGroupCount > 0)
		{
			ImGui::Text("Mesh binds : %u", dooms::graphics::graphicsSetting::CullStatMeshBindCount);
			ImGui::Text("Triangles  : %.2f M", static_cast<double>(dooms::graphics::graphicsSetting::CullStatIndexCount) / 3000000.0);
			ImGui::Text("Draw groups: %u for %u objects",
				dooms::graphics::graphicsSetting::CullStatDrawGroupCount,
				dooms::graphics::graphicsSetting::CullStatDrawnRendererCount);
		}

		// The two passes that touch pixels. Shown together because a depth
		// pre-pass is a trade between them, and either number alone hides it.
		if (dooms::graphics::graphicsSetting::GpuStatGeometryPassMilliseconds > 0.0f)
		{
			ImGui::Text("Geometry   : %.3f ms (GPU)", dooms::graphics::graphicsSetting::GpuStatGeometryPassMilliseconds);
		}

		if (dooms::graphics::graphicsSetting::GpuStatDepthPrePassMilliseconds > 0.0f)
		{
			ImGui::Text("Depth pre  : %.3f ms (GPU)", dooms::graphics::graphicsSetting::GpuStatDepthPrePassMilliseconds);
		}

		// Always shown, because this is where keeping the BVH current is paid
		// for and it is the largest cost the tree has. Reading it only while
		// the tree is on would hide what turning the tree on actually did.
		ImGui::Text("PreRender  : %.3f ms (CPU)", dooms::graphics::graphicsSetting::CpuStatPreRenderRendererMilliseconds);

		if (dooms::graphics::graphicsSetting::IsBVHFrustumCullingEnabled)
		{
			ImGui::Text("BVH cull   : %.3f ms (CPU)", dooms::graphics::graphicsSetting::CpuStatBVHCullMilliseconds);

			// Zero means the tree agrees with the per object test. Anything else
			// means it is culling objects that are visible, so it is called out
			// rather than left to be noticed on screen.
			const unsigned int bvhDisagreements = dooms::graphics::graphicsSetting::CullStatBVHDisagreementCount;

			if (bvhDisagreements > 0)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "BVH wrong  : %u", bvhDisagreements);
			}
			else
			{
				ImGui::Text("BVH wrong  : 0");
			}
		}

		// Per module cpu time, straight from EveryCulling's own profiler.
		//
		// It has been recording this all along, but the only thing reading it is
		// compiled out behind a macro that is never defined, so the numbers were
		// produced every frame and thrown away. Read here instead: comparing two
		// techniques means seeing what each costs beside what each removes.
		dooms::graphics::DefaultGraphcisPipeLine* const pipeLine
			= dooms::CastTo<dooms::graphics::DefaultGraphcisPipeLine*>(dooms::graphics::GraphicsPipeLine::GetSingleton());

		if (IsValid(pipeLine) && pipeLine->mRenderingCullingManager.mCullingSystem != nullptr)
		{
			ImGui::Separator();

			for (const auto& profilingData : pipeLine->mRenderingCullingManager.mCullingSystem->mEveryCullingProfiler.GetProfilingDatas())
			{
				ImGui::Text("%.*s : %.3f ms",
					static_cast<int>(profilingData.first.size()),
					profilingData.first.data(),
					profilingData.second.mElapsedTime);
			}
		}
	}
	dooms::ui::enginePanel::EndPanel();
}
