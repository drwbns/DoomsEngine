#include "DrawCallCounterGUI.h"

#include <imgui.h>
#include <EngineGUI/EngineGUIPanel.h>
#include <Rendering//RenderingDebugger/RenderingDebugger.h>
#include <Graphics/GraphicsAPI/GraphicsAPI.h>
#include <Graphics/graphicsSetting.h>
#include <DObject/DObjectGlobals.h>
#include <Rendering/Pipeline/GraphicsPipeLine.h>
#include <Rendering/Pipeline/PipeLines/DefaultGraphcisPipeLine.h>
#include <Rendering/Culling/EveryCulling/EveryCulling.h>
#include <Game/GameCore.h>

#include <cstring>

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

		// What every other number on this panel was measured under. A
		// screenshot of this overlay is often the only record of a
		// measurement's conditions, and a key press that never reached the
		// engine looks identical to one that did unless the state itself is
		// on screen.
		ImGui::Text("Scene : %s", dooms::GameCore::bmIsScenePaused ? "Paused" : "Running");

		const dooms::graphics::DefaultGraphcisPipeLine* const statPipeLine
			= dooms::CastTo<dooms::graphics::DefaultGraphcisPipeLine*>(dooms::graphics::GraphicsPipeLine::GetSingleton());

		if (IsValid(statPipeLine))
		{
			const culling::EveryCulling* const cullingSystem
				= statPipeLine->mRenderingCullingManager.mCullingSystem.get();

			if (cullingSystem != nullptr)
			{
				using CullingModuleType = culling::EveryCulling::CullingModuleType;

				// Assembled from the module flags rather than matched against
				// a mode table, so any combination config.ini can produce
				// still reads as what it is. Read back live for the same
				// reason: the interface is not the only thing that decides
				// what is running.
				static char cullingModeLabel[96];
				cullingModeLabel[0] = '\0';

				const bool bIsViewFrustumEnabled
					= cullingSystem->GetIsCullingModuleEnabled(CullingModuleType::ViewFrustumCulling);

				if (dooms::graphics::graphicsSetting::IsBVHFrustumCullingEnabled)
				{
					strcpy_s(cullingModeLabel, "BVH");
				}
				else if (bIsViewFrustumEnabled)
				{
					strcpy_s(cullingModeLabel, "Frustum");
				}

				if (cullingSystem->GetIsCullingModuleEnabled(CullingModuleType::MaskedSWOcclusionCulling))
				{
					strcat_s(cullingModeLabel, "+SW occlusion");
				}

				if (dooms::graphics::graphicsSetting::IsHiZOcclusionCullingEnabled)
				{
					strcat_s(cullingModeLabel, "+Hi-Z");
				}

				if (cullingSystem->GetIsCullingModuleEnabled(CullingModuleType::DistanceCulling))
				{
					strcat_s(cullingModeLabel, "+Distance");
				}

				if (cullingModeLabel[0] == '\0')
				{
					strcpy_s(cullingModeLabel, "None");
				}

				ImGui::Text("Culling : %s", cullingModeLabel);
			}
		}

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

			// Which grid the cost above was measured on. The H key's toast is
			// gone before a screenshot lands, and a sweep labeled by press count
			// is one lost keypress away from comparing the wrong grids against
			// each other. Which happened.
			ImGui::Text("Hi-Z grid  : %u wide", dooms::graphics::graphicsSetting::HiZReadbackTargetWidth);

			// Beside the grid, because the two together decide how much screen
			// a margin cell actually covers: one cell at 512 wide is a quarter
			// of what it is at 128.
			ImGui::Text("Hi-Z margin: %u cells", dooms::graphics::graphicsSetting::HiZStalenessMarginCells);
		}

		// What instancing could collapse the geometry pass to, if it existed.
		if (dooms::graphics::graphicsSetting::CullStatDrawGroupCount > 0)
		{
			ImGui::Text("Mesh binds : %u  (index %u)",
				dooms::graphics::graphicsSetting::CullStatMeshBindCount,
				dooms::graphics::graphicsSetting::CullStatIndexBindCount);
			ImGui::Text("Triangles  : %.2f M  (ideal %.2f M)",
				static_cast<double>(dooms::graphics::graphicsSetting::CullStatIndexCount) / 3000000.0,
				static_cast<double>(dooms::graphics::graphicsSetting::CullStatIdealIndexCount) / 3000000.0);
			ImGui::Text("Draw groups: %u for %u objects",
				dooms::graphics::graphicsSetting::CullStatDrawGroupCount,
				dooms::graphics::graphicsSetting::CullStatDrawnRendererCount);
		}

		if (dooms::graphics::graphicsSetting::IsHiZHullOccludeeEnabled)
		{
			ImGui::Text("Hulls      : %u meshes, %u verts",
				dooms::graphics::graphicsSetting::CullStatHullMeshCount,
				dooms::graphics::graphicsSetting::CullStatHullVertexCount);

			// Where the hull is winning, by how much of the screen the object
			// covers. Culling a distant rock saves as many triangles as culling a
			// near one, so this is what says whether a size threshold would keep
			// the benefit or throw it away with the cost.
			ImGui::Text("Hull tested: %u, skipped %u",
				dooms::graphics::graphicsSetting::CullStatHullTestedCount,
				dooms::graphics::graphicsSetting::CullStatHullSkippedCount);
			ImGui::Text("Hull culls : %u/%u/%u/%u/%u by cells",
				dooms::graphics::graphicsSetting::CullStatHullCullsBySize[0],
				dooms::graphics::graphicsSetting::CullStatHullCullsBySize[1],
				dooms::graphics::graphicsSetting::CullStatHullCullsBySize[2],
				dooms::graphics::graphicsSetting::CullStatHullCullsBySize[3],
				dooms::graphics::graphicsSetting::CullStatHullCullsBySize[4]);
		}

		// What a perfect culler would have managed, beside what this one did.
		// The gap is the headroom every culling technique is competing for.
		if (dooms::graphics::graphicsSetting::CullStatOracleTestedCount > 0)
		{
			const unsigned int wastedCount = dooms::graphics::graphicsSetting::CullStatOracleInvisibleCount;
			const unsigned int testedCount = dooms::graphics::graphicsSetting::CullStatOracleTestedCount;

			ImGui::TextColored(
				(wastedCount * 10u > testedCount) ? ImVec4(1.0f, 0.8f, 0.4f, 1.0f) : ImVec4(0.6f, 1.0f, 0.6f, 1.0f),
				"Wasted     : %u of %u drawn (%.1f%%)",
				wastedCount, testedCount,
				(testedCount > 0) ? (100.0f * static_cast<float>(wastedCount) / static_cast<float>(testedCount)) : 0.0f);
		}

		// The error in the other direction, which nothing else here can show:
		// objects the Hi-Z tests removed that would have drawn pixels. Any
		// value above zero is a hole in the image, so it is red at one rather
		// than at some percentage.
		if (dooms::graphics::graphicsSetting::CullStatOracleFalseCullTestedCount > 0 ||
			dooms::graphics::graphicsSetting::CullStatOracleFalseCullCount > 0)
		{
			const unsigned int falseCullCount = dooms::graphics::graphicsSetting::CullStatOracleFalseCullCount;
			const unsigned int falseCullTestedCount = dooms::graphics::graphicsSetting::CullStatOracleFalseCullTestedCount;

			ImGui::TextColored(
				(falseCullCount > 0) ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f) : ImVec4(0.6f, 1.0f, 0.6f, 1.0f),
				"False culls: %u of %u culled",
				falseCullCount, falseCullTestedCount);
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
