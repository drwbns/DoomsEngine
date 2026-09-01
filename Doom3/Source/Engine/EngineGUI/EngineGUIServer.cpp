#include "EngineGUIServer.h"

#include <Graphics/GraphicsAPI/PlatformImgui/PlatformImgui.h>

#include "imguiHelper/imguiWithReflection.h"

#include <Graphics/GraphicsAPI/GraphicsAPI.h>
#include "engineGUIServerHelper.h"
#include "GUIModules/EngineGUIModule.h"

// DockBuilder, used to lay the panels out the first time the dockspace appears.
#include <imgui_internal.h>

#include "EngineGUIPanel.h"

#include <cstring>
#include <cstdarg>
#include <cstdio>

#include <IO/UserInput_Server.h>
#include <Graphics/GraphicsAPI/graphicsAPISetting.h>
#include <Graphics/graphicsSetting.h>

#include <DObject/DObjectGlobals.h>
#include <Rendering/Pipeline/GraphicsPipeLine.h>
#include <Rendering/Pipeline/PipeLines/DefaultGraphcisPipeLine.h>
#include <Game/GameCore.h>

bool dooms::ui::EngineGUIServer::DestroyImgui()
{
    const bool isSuccess = dooms::graphics::PlatformImgui::ShutDownPlatformImgui();
    D_ASSERT(isSuccess == true);
    ImGui::DestroyContext();

    return isSuccess;
}


// ---------------------------------------------------------------------------
// Panel presentation
// ---------------------------------------------------------------------------
namespace
{
    // Starts hidden, so the engine opens on the scene with mouse look already
    // on rather than behind a wall of panels. F1 brings them back.
    dooms::ui::eEngineGUIDisplayMode gDisplayMode = dooms::ui::eEngineGUIDisplayMode::Hidden;
    char gFocusedPanelName[128] = "DrawCall";
    FLOAT32 gOverlayAlpha = 0.35f;

    // Whether the last BeginPanel actually opened a window, so EndPanel knows
    // if it owes ImGui a matching End().
    bool gPanelWindowOpened = false;

    // Set by F4, cleared once the dockspace has rebuilt its layout.
    //
    // True at startup, so the first time the panels appear they are in the
    // default arrangement rather than whatever imgui.ini remembered from a
    // previous run, which is how panels ended up stacked over the scene.
    bool gDockLayoutResetRequested = true;

    // The transient message shown when a mode is changed with an F key.
    //
    // It is drawn outside the panel system on purpose. The function keys are
    // most useful with the interface hidden, which is exactly when no panel is
    // there to say what just happened.
    char gNotificationText[128] = "";
    double gNotificationExpiryTime = 0.0;

    constexpr double NOTIFICATION_DURATION_SECONDS = 2.0;
    constexpr double NOTIFICATION_FADE_SECONDS = 0.6;

    void ShowNotification(const char* const format, ...)
    {
        va_list arguments;
        va_start(arguments, format);
        vsnprintf(gNotificationText, sizeof(gNotificationText), format, arguments);
        va_end(arguments);

        gNotificationExpiryTime = ImGui::GetTime() + NOTIFICATION_DURATION_SECONDS;
    }

    void RenderNotification()
    {
        const double currentTime = ImGui::GetTime();

        if (currentTime >= gNotificationExpiryTime)
        {
            return;
        }

        const double remainingSeconds = gNotificationExpiryTime - currentTime;
        const float alpha = (remainingSeconds < NOTIFICATION_FADE_SECONDS)
            ? static_cast<float>(remainingSeconds / NOTIFICATION_FADE_SECONDS)
            : 1.0f;

        const ImGuiViewport* const viewport = ImGui::GetMainViewport();
        const ImVec2 topCentre(
            viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
            viewport->WorkPos.y + 24.0f);

        ImGui::SetNextWindowPos(topCentre, ImGuiCond_Always, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.65f * alpha);

        // NoInputs matters: this sits over the scene and must never take the
        // mouse, least of all while mouse look is on.
        const ImGuiWindowFlags notificationFlags =
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs;

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

        if (ImGui::Begin("##EngineNotification", nullptr, notificationFlags))
        {
            ImGui::TextUnformatted(gNotificationText);
        }
        ImGui::End();

        ImGui::PopStyleVar();
    }
}

namespace dooms
{
    namespace ui
    {
        namespace enginePanel
        {
            eEngineGUIDisplayMode GetDisplayMode() { return gDisplayMode; }
            void SetDisplayMode(const eEngineGUIDisplayMode displayMode) { gDisplayMode = displayMode; }

            const char* GetFocusedPanelName() { return gFocusedPanelName; }

            void SetFocusedPanelName(const char* const panelName)
            {
                // Bounded copy, always terminated. Avoids strncpy, which MSVC
                // rejects here, and never leaves the buffer unterminated.
                const size_t capacity = sizeof(gFocusedPanelName);
                size_t index = 0;

                if (panelName != nullptr)
                {
                    for (; (index + 1) < capacity && panelName[index] != 0; index++)
                    {
                        gFocusedPanelName[index] = panelName[index];
                    }
                }

                gFocusedPanelName[index] = 0;
            }

            FLOAT32 GetOverlayAlpha() { return gOverlayAlpha; }

            void SetOverlayAlpha(const FLOAT32 alpha)
            {
                gOverlayAlpha = (alpha < 0.0f) ? 0.0f : ((alpha > 1.0f) ? 1.0f : alpha);
            }

            void RequestDockLayoutReset() { gDockLayoutResetRequested = true; }

            bool ConsumeDockLayoutResetRequest()
            {
                const bool wasRequested = gDockLayoutResetRequested;
                gDockLayoutResetRequested = false;
                return wasRequested;
            }

            bool BeginPanel(const char* const panelName)
            {
                gPanelWindowOpened = false;

                if (gDisplayMode == eEngineGUIDisplayMode::Hidden)
                {
                    return false;
                }

                if (gDisplayMode == eEngineGUIDisplayMode::FocusedOverlay)
                {
                    if (std::strcmp(panelName, gFocusedPanelName) != 0)
                    {
                        return false;
                    }

                    // No chrome, translucent, pinned to the top right and sized
                    // to its contents, so it reads as an overlay on the scene
                    // rather than a window.
                    const ImGuiViewport* const viewport = ImGui::GetMainViewport();
                    const ImVec2 corner(
                        viewport->WorkPos.x + viewport->WorkSize.x - 12.0f,
                        viewport->WorkPos.y + 12.0f);

                    ImGui::SetNextWindowPos(corner, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
                    ImGui::SetNextWindowBgAlpha(gOverlayAlpha);

                    const ImGuiWindowFlags overlayFlags =
                        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
                        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                        ImGuiWindowFlags_NoMove;

                    const bool isVisible = ImGui::Begin(panelName, nullptr, overlayFlags);
                    gPanelWindowOpened = true;
                    return isVisible;
                }

                const bool isVisible = ImGui::Begin(panelName);
                gPanelWindowOpened = true;
                return isVisible;
            }

            void EndPanel()
            {
                if (gPanelWindowOpened)
                {
                    ImGui::End();
                    gPanelWindowOpened = false;
                }
            }
        }
    }
}

namespace
{
    // A full-viewport host window that every panel docks into. It draws no
    // background and uses a pass-through central node, so the rendered scene
    // stays visible wherever nothing is docked.
    void BeginEngineDockSpace()
    {
        const ImGuiViewport* const viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        const ImGuiWindowFlags hostFlags =
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("DoomsEngineDockSpaceHost", nullptr, hostFlags);

        ImGui::PopStyleVar(3);

        const ImGuiID dockSpaceID = ImGui::GetID("DoomsEngineDockSpace");

        // Lay the panels out once, when the dockspace has no layout yet. A
        // layout restored from imgui.ini leaves the node populated, so an
        // arrangement the user has set up themselves is never overwritten.
        const bool bIsResetRequested = dooms::ui::enginePanel::ConsumeDockLayoutResetRequest();

        if (bIsResetRequested || ImGui::DockBuilderGetNode(dockSpaceID) == nullptr)
        {
            // Clear any arrangement first, otherwise the panels stay wherever
            // they were dragged and the default split has no effect.
            ImGui::DockBuilderRemoveNode(dockSpaceID);
            ImGui::DockBuilderAddNode(dockSpaceID, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockSpaceID, viewport->WorkSize);

            // Keep the middle free for the scene, and put panels around it.
            ImGuiID centre = dockSpaceID;
            const ImGuiID left = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Left, 0.24f, nullptr, &centre);
            const ImGuiID right = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Right, 0.26f, nullptr, &centre);
            const ImGuiID bottom = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Down, 0.28f, nullptr, &centre);

            // Scene contents on the left, the way an editor usually arranges it.
            ImGui::DockBuilderDockWindow("Entities in scene", left);

            // Performance figures together on the right.
            ImGui::DockBuilderDockWindow("DrawCall", right);
            ImGui::DockBuilderDockWindow("Display", right);
            ImGui::DockBuilderDockWindow("Visualisation", right);
            ImGui::DockBuilderDockWindow("Profiler", right);
            ImGui::DockBuilderDockWindow("Thread Profiler ( QueryThreadCycleTime ( /s ) )", right);

            // Log and the culling debuggers share the bottom as tabs.
            ImGui::DockBuilderDockWindow("Log", bottom);
            ImGui::DockBuilderDockWindow("Masked SW Occlusion Culling Debugger ( Binned Triangle Count of Tile )", bottom);
            ImGui::DockBuilderDockWindow("Masked SW Occlusion Culling Debugger ( L0 Max Depth Value of SubTile )", bottom);

            ImGui::DockBuilderFinish(dockSpaceID);
        }

        ImGui::DockSpace(dockSpaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::End();
    }
}

namespace
{
    void ToggleBorderlessFullscreen()
    {
        using namespace dooms::graphics;

        if (GraphicsAPI::SetBorderlessFullscreen == nullptr || GraphicsAPI::IsBorderlessFullscreen == nullptr)
        {
            return;
        }

        const unsigned int bIsFullscreen = GraphicsAPI::IsBorderlessFullscreen();
        GraphicsAPI::SetBorderlessFullscreen(bIsFullscreen != 0 ? 0u : 1u);
    }

    // A checkbox that explains itself on hover. These were only reachable
    // through a demo component's properties before, where nothing said what any
    // of them did. The description is a tooltip rather than inline text because
    // the panel docks narrow, and inline descriptions truncate to uselessness.
    void VisualisationToggle(const char* const label, const char* const description, bool& value)
    {
        ImGui::Checkbox(label, &value);

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        {
            ImGui::SetTooltip("%s", description);
        }
    }

    // The visualisations F6 steps through, in the order it steps through them.
    //
    // One at a time and nothing else on, because most of these cover the screen
    // and two at once shows you neither. The panel below still allows any
    // combination for when that is what you want.
    struct VisualisationMode
    {
        const char* mName;
        bool* mFlag;
    };

    const VisualisationMode gVisualisationCycle[] =
    {
        { "Off",              nullptr },
        { "Occluder bounds",  &dooms::graphics::graphicsSetting::IsDrawMaskedOcclusionCullingOcculderBoundingBoxDebugger },
        { "Binned triangles", &dooms::graphics::graphicsSetting::IsDrawMaskedOcclusionCullingBinTriangleStageDebugger },
        { "Tile coverage",    &dooms::graphics::graphicsSetting::IsDrawMaskedOcclusionCullingTileCoverageMaskDebugger },
        { "Tile depth",       &dooms::graphics::graphicsSetting::IsDrawMaskedOcclusionCullingTileL0MaxDepthValueDebugger },
        { "Overdraw",         &dooms::graphics::graphicsSetting::IsOverDrawVisualizationEnabled },
        { "Depth buffer",     &dooms::graphics::graphicsSetting::IsDepthBufferVisualizationEnabled },
        { "Hi-Z pyramid",    &dooms::graphics::graphicsSetting::IsHiZVisualizationEnabled },
        { "Renderer bounds",  &dooms::graphics::graphicsSetting::DrawRenderingBoundingBox }
    };

    constexpr INT32 gVisualisationCycleCount
        = static_cast<INT32>(sizeof(gVisualisationCycle) / sizeof(gVisualisationCycle[0]));

    INT32 gVisualisationCycleIndex = 0;

    void ApplyVisualisationCycleIndex(const INT32 index)
    {
        for (INT32 i = 0; i < gVisualisationCycleCount; i++)
        {
            if (gVisualisationCycle[i].mFlag != nullptr)
            {
                *(gVisualisationCycle[i].mFlag) = (i == index);
            }
        }

        gVisualisationCycleIndex = index;
    }

    // The culling configurations F7 steps through.
    //
    // Each one turns off a technique so its contribution can be read off the
    // frame time and the overdraw view: switch to "None" and the overdraw goes
    // red everywhere, switch to "Frustum + Occlusion" and watch what comes back.
    //
    // PreCulling is never in this table. It is not a culling technique -- it
    // updates bounding spheres and the screen space bounds that the real
    // modules read, and occluder selection is meaningless without it -- so it
    // stays on in every mode.
    struct OcclusionMode
    {
        const char* mName;
        bool mIsViewFrustumEnabled;
        bool mIsMaskedSWOcclusionEnabled;
        bool mIsHiZEnabled;
    };

    // Distance culling is deliberately not one of the modes below.
    //
    // It is an independent axis: it answers a different question from which
    // occlusion technique is running, and folding it into the cycle meant some
    // modes had it and others did not, so two of them could never be compared
    // without wondering whether the difference was the technique or the
    // distance cut. Applied on top of whichever mode is selected, so every
    // comparison is like for like by construction.
    bool gIsDistanceCullingEnabled = false;

    const OcclusionMode gOcclusionModes[] =
    {
        { "None",                  false, false, false },
        { "Frustum",               true,  false, false },

        // The two occlusion techniques, listed apart rather than combined:
        // running them together would measure neither.
        { "Frustum + SW occlusion", true, true,  false },
        { "Frustum + Hi-Z",         true, false, true  }
    };

    constexpr INT32 gOcclusionModeCount
        = static_cast<INT32>(sizeof(gOcclusionModes) / sizeof(gOcclusionModes[0]));

    // Corrected from the culling system on the first frame by
    // SyncOcclusionModeFromEngine, because the engine does not boot from this
    // table: config.ini and the demo component decide what is running.
    INT32 gOcclusionModeIndex = 0;

    // Reads back what the culling system is actually running and points the
    // labels at the matching entry.
    //
    // Without this the panel reported "culling: None" while five and a half
    // thousand objects were being culled, because these values were only ever
    // written by the interface and never compared against the engine. A harness
    // whose readouts describe a state nobody selected is worse than no readout.
    // Returns whether the culling system was there to be read, so the caller
    // can keep trying rather than record a sync that never happened.
    bool SyncOcclusionModeFromEngine()
    {
        dooms::graphics::DefaultGraphcisPipeLine* const pipeLine
            = dooms::CastTo<dooms::graphics::DefaultGraphcisPipeLine*>(dooms::graphics::GraphicsPipeLine::GetSingleton());

        if (IsValid(pipeLine) == false)
        {
            return false;
        }

        culling::EveryCulling* const cullingSystem = pipeLine->mRenderingCullingManager.mCullingSystem.get();
        if (cullingSystem == nullptr)
        {
            return false;
        }

        using CullingModuleType = culling::EveryCulling::CullingModuleType;

        gIsDistanceCullingEnabled
            = cullingSystem->GetIsCullingModuleEnabled(CullingModuleType::DistanceCulling);

        const bool bIsFrustumEnabled
            = cullingSystem->GetIsCullingModuleEnabled(CullingModuleType::ViewFrustumCulling);
        const bool bIsMaskedSWEnabled
            = cullingSystem->GetIsCullingModuleEnabled(CullingModuleType::MaskedSWOcclusionCulling);
        const bool bIsHiZEnabled
            = dooms::graphics::graphicsSetting::IsHiZOcclusionCullingEnabled;

        for (INT32 modeIndex = 0; modeIndex < gOcclusionModeCount; modeIndex++)
        {
            const OcclusionMode& occlusionMode = gOcclusionModes[modeIndex];

            if (occlusionMode.mIsViewFrustumEnabled == bIsFrustumEnabled
                && occlusionMode.mIsMaskedSWOcclusionEnabled == bIsMaskedSWEnabled
                && occlusionMode.mIsHiZEnabled == bIsHiZEnabled)
            {
                gOcclusionModeIndex = modeIndex;
                return true;
            }
        }

        // No entry describes it, which is possible because config.ini can enable
        // any combination. Left on whatever it was rather than asserting a mode
        // that is not running, but still a completed read.
        return true;
    }

    void ApplyOcclusionModeIndex(const INT32 index)
    {
        dooms::graphics::DefaultGraphcisPipeLine* const pipeLine
            = dooms::CastTo<dooms::graphics::DefaultGraphcisPipeLine*>(dooms::graphics::GraphicsPipeLine::GetSingleton());

        if (IsValid(pipeLine) == false)
        {
            return;
        }

        culling::EveryCulling* const cullingSystem = pipeLine->mRenderingCullingManager.mCullingSystem.get();
        if (cullingSystem == nullptr)
        {
            return;
        }

        const OcclusionMode& occlusionMode = gOcclusionModes[index];

        // BVH culling replaces the per object frustum module rather than
        // running beside it. Both reject the same objects, so leaving the
        // module on would hide whatever the tree saves.
        const bool bIsModuleFrustumEnabled =
            occlusionMode.mIsViewFrustumEnabled && (dooms::graphics::graphicsSetting::IsBVHFrustumCullingEnabled == false);

        cullingSystem->SetEnabledCullingModule(
            culling::EveryCulling::CullingModuleType::ViewFrustumCulling, bIsModuleFrustumEnabled);
        cullingSystem->SetEnabledCullingModule(
            culling::EveryCulling::CullingModuleType::DistanceCulling, gIsDistanceCullingEnabled);
        cullingSystem->SetEnabledCullingModule(
            culling::EveryCulling::CullingModuleType::MaskedSWOcclusionCulling, occlusionMode.mIsMaskedSWOcclusionEnabled);

        // Not a culling module: it lives in the pipeline, because it reads a
        // depth pyramid the pipeline owns.
        dooms::graphics::graphicsSetting::IsHiZOcclusionCullingEnabled = occlusionMode.mIsHiZEnabled;

        gOcclusionModeIndex = index;
    }

    // How the geometry itself is drawn, stepped through by F8.
    struct RenderModeEntry
    {
        const char* mName;
        dooms::graphics::graphicsSetting::eRenderMode mMode;
    };

    const RenderModeEntry gRenderModes[] =
    {
        { "Shaded",    dooms::graphics::graphicsSetting::eRenderMode::Shaded },
        { "Wireframe", dooms::graphics::graphicsSetting::eRenderMode::Wireframe },
        { "Textured",  dooms::graphics::graphicsSetting::eRenderMode::Textured }
    };

    constexpr INT32 gRenderModeCount
        = static_cast<INT32>(sizeof(gRenderModes) / sizeof(gRenderModes[0]));

    INT32 gRenderModeIndex = 0;

    void ApplyRenderModeIndex(const INT32 index)
    {
        dooms::graphics::graphicsSetting::RenderMode = gRenderModes[index].mMode;
        gRenderModeIndex = index;
    }

    void RenderVisualisationPanel()
    {
        using namespace dooms::graphics;

        if (dooms::ui::enginePanel::BeginPanel("Visualisation"))
        {
            ImGui::TextDisabled("F6 cycles: %s", gVisualisationCycle[gVisualisationCycleIndex].mName);
            ImGui::TextDisabled("F7 culling: %s", gOcclusionModes[gOcclusionModeIndex].mName);

            // What the selected mode is actually worth. Draw calls move for
            // reasons that have nothing to do with culling, so the number that
            // compares two modes is how many objects each one removes.
            {
                const unsigned int entityCount = graphicsSetting::CullStatEntityCount;
                const unsigned int culledCount = graphicsSetting::CullStatCulledCount;
                const unsigned int drawnCount = (entityCount >= culledCount) ? (entityCount - culledCount) : 0u;

                ImGui::TextDisabled("Objects: %u drawn / %u culled / %u total", drawnCount, culledCount, entityCount);
            }

            // Sits with the culling readouts rather than the visualisation
            // toggles, because it changes what is drawn, not how it is shown.
            if (ImGui::Checkbox("BVH frustum culling", &graphicsSetting::IsBVHFrustumCullingEnabled))
            {
                // Re-applied, because this decides whether the per object
                // frustum module runs at all.
                ApplyOcclusionModeIndex(gOcclusionModeIndex);

                // The tree has been ignoring transform updates for however long
                // it was switched off, so it starts from a full refresh rather
                // than from whatever it happened to be holding.
                graphicsSetting::IsBVHFullRefreshRequested = true;
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            {
                ImGui::SetTooltip("%s", "rejects whole subtrees instead of testing every object, replacing the frustum module");
            }

            {
                // Reads the enum rather than mirroring it, so the box cannot
                // drift from what the pipeline is doing.
                bool bIsDepthPrePassEnabled =
                    (dooms::graphics::graphicsAPISetting::DepthPrePassType == dooms::graphics::eDepthPrePassType::AllOpaque);

                if (ImGui::Checkbox("Depth pre-pass", &bIsDepthPrePassEnabled))
                {
                    dooms::graphics::graphicsAPISetting::DepthPrePassType = bIsDepthPrePassEnabled
                        ? dooms::graphics::eDepthPrePassType::AllOpaque
                        : dooms::graphics::eDepthPrePassType::Disable;
                }

                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
                {
                    ImGui::SetTooltip("%s", "lays depth down first so the g-buffer pass shades each pixel once, at the cost of drawing everything twice");
                }
            }

            if (ImGui::Checkbox("Group draws by state", &graphicsSetting::IsGroupDrawsByStateEnabled))
            {
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            {
                ImGui::SetTooltip("%s", "draws objects sharing a mesh and material together, giving up front to back order to stop rebinding the same geometry");
            }

            if (ImGui::Checkbox("Skip unmoved objects", &graphicsSetting::IsSkipUnchangedCullingDataEnabled))
            {
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            {
                ImGui::SetTooltip("%s", "stops rewriting bounds and a matrix into the culling system for objects whose transform did not change");
            }

            if (ImGui::Checkbox("Distance culling", &gIsDistanceCullingEnabled))
            {
                // Re-applied through the mode, which is what actually reaches
                // the culling system.
                ApplyOcclusionModeIndex(gOcclusionModeIndex);
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            {
                ImGui::SetTooltip("%s", "applies to every culling mode, so they stay comparable");
            }
            ImGui::TextDisabled("F8 render: %s", gRenderModes[gRenderModeIndex].mName);

            VisualisationToggle(
                "Debug drawing",
                "master switch for everything below",
                graphicsSetting::IsDrawDebuggersEnabled);

            ImGui::SeparatorText("Occlusion culling");

            VisualisationToggle(
                "Occluder bounds",
                "boxes of the objects chosen to occlude",
                graphicsSetting::IsDrawMaskedOcclusionCullingOcculderBoundingBoxDebugger);

            VisualisationToggle(
                "Binned triangles",
                "per tile, how much geometry was rasterised",
                graphicsSetting::IsDrawMaskedOcclusionCullingBinTriangleStageDebugger);

            VisualisationToggle(
                "Tile coverage",
                "per tile, which pixels the occluders covered",
                graphicsSetting::IsDrawMaskedOcclusionCullingTileCoverageMaskDebugger);

            VisualisationToggle(
                "Tile depth",
                "per tile, the depth occludees are tested against",
                graphicsSetting::IsDrawMaskedOcclusionCullingTileL0MaxDepthValueDebugger);

            ImGui::SeparatorText("Scene");

            VisualisationToggle(
                "Overdraw",
                "how many times each pixel was shaded",
                graphicsSetting::IsOverDrawVisualizationEnabled);

            VisualisationToggle(
                "Depth buffer",
                "the camera depth buffer, linearised so it is readable",
                graphicsSetting::IsDepthBufferVisualizationEnabled);

            VisualisationToggle(
                "Renderer bounds",
                "bounding box of every renderer",
                graphicsSetting::DrawRenderingBoundingBox);

            ImGui::SeparatorText("Rendering");

            VisualisationToggle(
                "Sort front to back",
                "off makes occlusion culling look worse than it is",
                graphicsSetting::IsSortObjectFrontToBack);
        }
        dooms::ui::enginePanel::EndPanel();
    }

    void RenderDisplayPanel()
    {
        using namespace dooms::graphics;

        if (dooms::ui::enginePanel::BeginPanel("Display"))
        {
            ImGui::Text("Resolution   : %d x %d", graphicsAPISetting::GetScreenWidth(), graphicsAPISetting::GetScreenHeight());
            ImGui::Text("Aspect ratio : %.4f", graphicsAPISetting::GetScreenRatio());
            ImGui::Text("Multisample  : %u", graphicsAPISetting::GetMultiSamplingNum());

            const bool bIsSupported =
                (GraphicsAPI::SetBorderlessFullscreen != nullptr) &&
                (GraphicsAPI::IsBorderlessFullscreen != nullptr);

            ImGui::Separator();

            if (bIsSupported == false)
            {
                // Older graphics DLLs do not export these.
                ImGui::Text("Window mode  : unavailable");
            }
            else
            {
                const bool bIsFullscreen = (GraphicsAPI::IsBorderlessFullscreen() != 0);
                ImGui::Text("Window mode  : %s", bIsFullscreen ? "Borderless fullscreen" : "Windowed");

                if (ImGui::Button(bIsFullscreen ? "Switch to windowed" : "Switch to borderless fullscreen"))
                {
                    ToggleBorderlessFullscreen();
                }

                ImGui::SameLine();
                ImGui::TextDisabled("(F11)");
            }
        }
        dooms::ui::enginePanel::EndPanel();
    }
}

void dooms::ui::EngineGUIServer::PreRender()
{
    // Once, as late as the first frame, because the culling system does not
    // exist when this translation unit's globals are initialised.
    static bool bHasSyncedOcclusionMode = false;

    if (bHasSyncedOcclusionMode == false)
    {
        bHasSyncedOcclusionMode = SyncOcclusionModeFromEngine();
    }

    if (bmIsEngineGUIAvaliable == true)
    {
        graphics::PlatformImgui::PreRenderPlatformImgui();
        ImGui::NewFrame();

        // Only host a dockspace when panels are docked; the overlay and hidden
        // modes want the viewport left alone.
        if (enginePanel::GetDisplayMode() == eEngineGUIDisplayMode::All)
        {
            BeginEngineDockSpace();
        }
    }
}



void dooms::ui::EngineGUIServer::Render()
{
    if (bmIsEngineGUIAvaliable == true)
    {
        // The reflection inspector opens a window per inspected object, so it
        // only belongs in the docked view.
        if (enginePanel::GetDisplayMode() == eEngineGUIDisplayMode::All)
        {
            dooms::ui::imguiWithReflection::UpdateGUI_DObjectsVisibleOnGUI();
        }
     
        RenderDisplayPanel();
        RenderVisualisationPanel();

        for(EngineGUIModule* module : mEngineGUIModules)
        {
            D_ASSERT(IsValid(module));
            if(IsValid(module))
            {
                module->Render();
            }
        }

        // Last, and deliberately not gated on the display mode, so that a mode
        // change still announces itself with the interface hidden.
        RenderNotification();

        ImGui::Render();
    }
}

void dooms::ui::EngineGUIServer::PostRender()
{
    if(bmIsEngineGUIAvaliable == true)
    {
        bmIsEngineGUIAvaliable = true;

        dooms::ui::imguiWithReflection::ClearId();
        dooms::graphics::PlatformImgui::PostRenderPlatformImgui();
    }
}

bool dooms::ui::EngineGUIServer::GetIsEngineGUIAvaliable() const
{
    return bmIsEngineGUIAvaliable;
}

bool& dooms::ui::EngineGUIServer::GetIsEngineGUIAvaliableRef()
{
    return bmIsEngineGUIAvaliable;
}

void dooms::ui::EngineGUIServer::AddEngineGUIModule(EngineGUIModule* const engineGUIModule)
{
    D_ASSERT(IsValid(engineGUIModule));
    D_ASSERT(std::find(mEngineGUIModules.begin(), mEngineGUIModules.end(), engineGUIModule) == mEngineGUIModules.end());;

    mEngineGUIModules.push_back(engineGUIModule);
    engineGUIModule->InitIfNotInitialized();
}

void dooms::ui::EngineGUIServer::RemoveEngineGUIModule(EngineGUIModule* const engineGUIModule)
{
    D_ASSERT(IsValid(engineGUIModule));
    for(INT64 i = 0 ; i < mEngineGUIModules.size() ; i++)
    {
	    if(mEngineGUIModules[i] == engineGUIModule)
	    {
            mEngineGUIModules.erase(mEngineGUIModules.begin() + i);
            i = -1;
	    }
    }
}

void dooms::ui::EngineGUIServer::RemoveEngineGUIModule(const reflection::DClass dClass)
{
    for (INT64 i = 0; i < mEngineGUIModules.size(); i++)
    {
        if(IsValid(mEngineGUIModules[i]))
        {
            if (mEngineGUIModules[i]->GetDClass() == dClass)
            {
                mEngineGUIModules.erase(mEngineGUIModules.begin() + i);
                i = -1;
            }
        }       
    }
}


bool dooms::ui::EngineGUIServer::InitializeImgui()
{
	IMGUI_CHECKVERSION();
	ImGuiContext* const imGuiContext = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	bool isSuccess = (imGuiContext != nullptr);

	// TODO : Block dispatch imput to application when mouse hover on gui
	io.WantCaptureMouse = true;
	io.WantCaptureKeyboard = true;


	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Let panels be docked and tabbed instead of floating over one another.
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();
	ImGuiMemAllocFunc p_alloc_func;
	ImGuiMemFreeFunc p_free_func;
	void* p_user_data;
	ImGui::GetAllocatorFunctions(&p_alloc_func, &p_free_func, &p_user_data);
	isSuccess &= static_cast<bool>(dooms::graphics::PlatformImgui::InitializePlatformImgui(dooms::graphics::GraphicsAPI::GetPlatformWindow(), dooms::graphics::GraphicsAPI::GetPlatformVersion(), imGuiContext, *p_alloc_func, *p_free_func, &p_user_data));

    return isSuccess;
}

dooms::ui::EngineGUIServer::EngineGUIServer()
	: bmIsEngineGUIAvaliable(true)
{
}

dooms::ui::EngineGUIServer::~EngineGUIServer()
{
    DestroyImgui();
}

void dooms::ui::EngineGUIServer::InitializeDefaultEngineGUIModules()
{
    std::vector<dooms::ui::EngineGUIModule*> defaultEngineGUIModule = engineGUIServerHelper::CreateDefaultEngineGUIModules();
	for (EngineGUIModule* module : defaultEngineGUIModule)
	{
        AddEngineGUIModule(module);
	}
}

void dooms::ui::EngineGUIServer::Init()
{
	bool isSuccess = InitializeImgui();
    dooms::ui::imguiWithReflection::Initialize();

    InitializeDefaultEngineGUIModules();

    D_ASSERT(isSuccess == true);
}

void dooms::ui::EngineGUIServer::Update()
{
    // Panels that can be focused as an overlay, in cycle order.
    static const char* const focusablePanels[] =
    {
        "DrawCall",
        "Display",
        "Visualisation",
        "Profiler",
        "Thread Profiler ( QueryThreadCycleTime ( /s ) )",
        "Log",
        "Masked SW Occlusion Culling Debugger ( Binned Triangle Count of Tile )",
        "Masked SW Occlusion Culling Debugger ( L0 Max Depth Value of SubTile )"
    };
    constexpr INT32 focusablePanelCount = static_cast<INT32>(sizeof(focusablePanels) / sizeof(focusablePanels[0]));

    using eKEY_CODE = dooms::input::GraphicsAPIInput::eKEY_CODE;

    // F1 hides the interface entirely and brings it back docked.
    if (dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_F1))
    {
        const bool bShouldShow = (enginePanel::GetDisplayMode() == eEngineGUIDisplayMode::Hidden);

        enginePanel::SetDisplayMode(bShouldShow ? eEngineGUIDisplayMode::All : eEngineGUIDisplayMode::Hidden);

        if (bShouldShow)
        {
            // Guaranteed way back. PreRender and Render bail out entirely when
            // this is false, so without it anything that clears the flag leaves
            // the interface unrecoverable.
            bmIsEngineGUIAvaliable = true;
        }

        ShowNotification("Interface: %s", bShouldShow ? "Shown" : "Hidden");
    }

    // F2 swaps between the docked view and a single focused overlay.
    if (dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_F2))
    {
        const bool bWasOverlay = (enginePanel::GetDisplayMode() == eEngineGUIDisplayMode::FocusedOverlay);

        enginePanel::SetDisplayMode(bWasOverlay
            ? eEngineGUIDisplayMode::All
            : eEngineGUIDisplayMode::FocusedOverlay);

        ShowNotification("Layout: %s", bWasOverlay ? "Docked" : "Overlay");
    }

    // F11 toggles borderless fullscreen, the usual binding for it.
    if (dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_F11))
    {
        ToggleBorderlessFullscreen();

        // Read back rather than assumed, because the toggle does nothing at all
        // when the graphics DLL has no fullscreen entry points.
        if (dooms::graphics::GraphicsAPI::IsBorderlessFullscreen != nullptr)
        {
            ShowNotification("Display: %s",
                (dooms::graphics::GraphicsAPI::IsBorderlessFullscreen() != 0) ? "Fullscreen" : "Windowed");
        }
    }

    // F5 holds the scene still, so two culling modes can be measured against
    // the same frame instead of two different ones.
    if (dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_F5))
    {
        dooms::GameCore::bmIsScenePaused = !dooms::GameCore::bmIsScenePaused;

        ShowNotification("Scene: %s", dooms::GameCore::bmIsScenePaused ? "Paused" : "Running");
    }

    // F6 steps through the visualisations one at a time, starting from off.
    if (dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_F6))
    {
        ApplyVisualisationCycleIndex((gVisualisationCycleIndex + 1) % gVisualisationCycleCount);

        D_RELEASE_LOG(eLogType::D_LOG, "Visualisation : %s", gVisualisationCycle[gVisualisationCycleIndex].mName);
        ShowNotification("View: %s", gVisualisationCycle[gVisualisationCycleIndex].mName);
    }

    // F7 steps through the culling configurations, to compare what each
    // technique is actually buying in this scene.
    if (dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_F7))
    {
        ApplyOcclusionModeIndex((gOcclusionModeIndex + 1) % gOcclusionModeCount);

        D_RELEASE_LOG(eLogType::D_LOG, "Culling : %s", gOcclusionModes[gOcclusionModeIndex].mName);
        ShowNotification("Culling: %s", gOcclusionModes[gOcclusionModeIndex].mName);
    }

    // F9 steps through the levels of the Hi-Z pyramid while its view is up.
    //
    // Wraps at 11, which covers a 1920 wide pyramid down to a single texel.
    // Reading the real count would mean reaching into the pipeline for it, and
    // sampling past the last level just clamps.
    if (dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_F9))
    {
        dooms::graphics::graphicsSetting::HiZVisualizationLevel =
            (dooms::graphics::graphicsSetting::HiZVisualizationLevel + 1) % 11;

        ShowNotification("Hi-Z level: %u", dooms::graphics::graphicsSetting::HiZVisualizationLevel);
    }

    // F8 steps through how the geometry is drawn.
    if (dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_F8))
    {
        ApplyRenderModeIndex((gRenderModeIndex + 1) % gRenderModeCount);

        D_RELEASE_LOG(eLogType::D_LOG, "Render mode : %s", gRenderModes[gRenderModeIndex].mName);
        ShowNotification("Render mode: %s", gRenderModes[gRenderModeIndex].mName);
    }

    // B toggles the BVH, so the two frustum implementations can be swapped
    // while flying rather than by finding a checkbox.
    //
    // Guarded on text input, unlike the function keys above: this is a letter,
    // and the inspector has text fields that would otherwise toggle culling
    // every time someone typed a b into one.
    //
    // WantTextInput rather than WantCaptureKeyboard, which is true whenever a
    // docked panel merely has focus and so disabled every letter key for as
    // long as the interface was on screen.
    if (ImGui::GetIO().WantTextInput == false
        && dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_B))
    {
        dooms::graphics::graphicsSetting::IsBVHFrustumCullingEnabled = !dooms::graphics::graphicsSetting::IsBVHFrustumCullingEnabled;

        // Both of these are what the checkbox does, and both matter: the mode
        // decides whether the per object module runs beside the tree, and the
        // refresh stops the tree culling against where objects used to be.
        ApplyOcclusionModeIndex(gOcclusionModeIndex);
        dooms::graphics::graphicsSetting::IsBVHFullRefreshRequested = true;

        ShowNotification("BVH culling: %s",
            dooms::graphics::graphicsSetting::IsBVHFrustumCullingEnabled ? "On" : "Off");
    }

    // P toggles the depth pre pass, the one lever in this engine that attacks
    // overdraw directly rather than by drawing fewer objects.
    if (ImGui::GetIO().WantTextInput == false
        && dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_P))
    {
        const bool bIsEnabled =
            (dooms::graphics::graphicsAPISetting::DepthPrePassType == dooms::graphics::eDepthPrePassType::AllOpaque);

        dooms::graphics::graphicsAPISetting::DepthPrePassType = bIsEnabled
            ? dooms::graphics::eDepthPrePassType::Disable
            : dooms::graphics::eDepthPrePassType::AllOpaque;

        ShowNotification("Depth pre-pass: %s", bIsEnabled ? "Off" : "On");
    }

    // U toggles skipping the per frame culling data write for objects that
    // did not move, so its saving can be read off PreRender rather than argued.
    if (ImGui::GetIO().WantTextInput == false
        && dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_U))
    {
        dooms::graphics::graphicsSetting::IsSkipUnchangedCullingDataEnabled =
            !dooms::graphics::graphicsSetting::IsSkipUnchangedCullingDataEnabled;

        ShowNotification("Skip unmoved: %s",
            dooms::graphics::graphicsSetting::IsSkipUnchangedCullingDataEnabled ? "On" : "Off");
    }

    // G draws in mesh and material order instead of front to back, trading
    // depth rejection for redundant binds.
    if (ImGui::GetIO().WantTextInput == false
        && dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_G))
    {
        dooms::graphics::graphicsSetting::IsGroupDrawsByStateEnabled =
            !dooms::graphics::graphicsSetting::IsGroupDrawsByStateEnabled;

        ShowNotification("Draw order: %s",
            dooms::graphics::graphicsSetting::IsGroupDrawsByStateEnabled ? "Grouped by state" : "Front to back");
    }

    // M toggles skipping redundant mesh binds without touching the draw order,
    // which is the only way to price the binds on their own: grouping changes
    // the ordering at the same time, so its result confounds the two.
    if (ImGui::GetIO().WantTextInput == false
        && dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_M))
    {
        dooms::graphics::graphicsSetting::IsSkipRedundantMeshBindEnabled =
            !dooms::graphics::graphicsSetting::IsSkipRedundantMeshBindEnabled;

        ShowNotification("Redundant mesh binds: %s",
            dooms::graphics::graphicsSetting::IsSkipRedundantMeshBindEnabled ? "Skipped" : "Issued");
    }

    // O measures what a perfect culler would have achieved on this frame.
    // Expensive: it draws the whole scene a second time.
    if (ImGui::GetIO().WantTextInput == false
        && dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_O))
    {
        dooms::graphics::graphicsSetting::IsVisibilityOracleEnabled =
            !dooms::graphics::graphicsSetting::IsVisibilityOracleEnabled;

        ShowNotification("Visibility oracle: %s",
            dooms::graphics::graphicsSetting::IsVisibilityOracleEnabled ? "On" : "Off");
    }

    // H steps the Hi-Z test through finer granularities, so the cost of its
    // coarseness can be read off the oracle rather than assumed.
    if (ImGui::GetIO().WantTextInput == false
        && dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_H))
    {
        unsigned int& targetWidth = dooms::graphics::graphicsSetting::HiZReadbackTargetWidth;

        targetWidth = (targetWidth >= 512u) ? 64u : (targetWidth * 2u);

        ShowNotification("Hi-Z test grid: %u wide", targetWidth);
    }

    // J and K step the two attribution probes, so the cost of the box's area
    // and the cost of its protruding near corner can be separated.
    if (ImGui::GetIO().WantTextInput == false
        && dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_J))
    {
        FLOAT32& shrink = dooms::graphics::graphicsSetting::HiZProbeRectangleShrink;

        shrink = (shrink >= 0.29f) ? 0.0f : (shrink + 0.10f);

        ShowNotification("Probe: rectangle inset %.0f%%", shrink * 100.0f);
    }

    if (ImGui::GetIO().WantTextInput == false
        && dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_K))
    {
        FLOAT32& depthPush = dooms::graphics::graphicsSetting::HiZProbeDepthPush;

        depthPush = (depthPush >= 0.0029f) ? 0.0f : (depthPush + 0.0010f);

        ShowNotification("Probe: depth push %.4f", depthPush);
    }

    // N swaps the occludee shape between the bounding box and the mesh's
    // convex hull, which is the comparison the hull exists to make.
    if (ImGui::GetIO().WantTextInput == false
        && dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_N))
    {
        dooms::graphics::graphicsSetting::IsHiZHullOccludeeEnabled =
            !dooms::graphics::graphicsSetting::IsHiZHullOccludeeEnabled;

        ShowNotification("Occludee shape: %s",
            dooms::graphics::graphicsSetting::IsHiZHullOccludeeEnabled ? "Convex hull" : "Bounding box");
    }

    // L draws each object at a detail level matched to its size on screen.
    if (ImGui::GetIO().WantTextInput == false
        && dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_L))
    {
        dooms::graphics::graphicsSetting::IsMeshLodEnabled =
            !dooms::graphics::graphicsSetting::IsMeshLodEnabled;

        ShowNotification("Mesh detail: %s",
            dooms::graphics::graphicsSetting::IsMeshLodEnabled ? "By screen size" : "Full");
    }

    // Y steps how many triangles a pixel is considered worth, which is what
    // decides how coarse a level each object gets.
    //
    // Driving it to the floor forces every object to its coarsest level, and
    // that is the test that matters: if the geometry pass does not get faster
    // when the triangle count collapses, then triangle count is not what it is
    // spending its time on, and the whole cost model is wrong.
    if (ImGui::GetIO().WantTextInput == false
        && dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_Y))
    {
        FLOAT32& trianglesPerPixel = dooms::graphics::graphicsSetting::MeshLodTrianglesPerPixel;

        trianglesPerPixel = (trianglesPerPixel <= 0.02f) ? 1.0f : (trianglesPerPixel * 0.25f);

        ShowNotification("Detail: %.3f triangles per pixel", trianglesPerPixel);
    }

    // V drops the per draw model matrix write, to price it. The scene
    // collapses into a heap while it is on, which is the point: it is a
    // measurement, not a mode.
    if (ImGui::GetIO().WantTextInput == false
        && dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_V))
    {
        dooms::graphics::graphicsSetting::IsSkipPerDrawUboWriteEnabled =
            !dooms::graphics::graphicsSetting::IsSkipPerDrawUboWriteEnabled;

        ShowNotification("Per-draw UBO write: %s",
            dooms::graphics::graphicsSetting::IsSkipPerDrawUboWriteEnabled ? "SKIPPED (probe)" : "On");
    }

    // X rasterises the projected hull as a polygon rather than testing the
    // rectangle around it. Measured as a heavy loss in Debug and switched off;
    // it is cpu bound, which is the class of technique Release inverted.
    if (ImGui::GetIO().WantTextInput == false
        && dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_X))
    {
        dooms::graphics::graphicsSetting::IsHiZHullPolygonEnabled =
            !dooms::graphics::graphicsSetting::IsHiZHullPolygonEnabled;

        ShowNotification("Occludee outline: %s",
            dooms::graphics::graphicsSetting::IsHiZHullPolygonEnabled ? "Polygon" : "Rectangle");
    }

    // F4 puts the panels back into the default arrangement.
    if (dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_F4))
    {
        enginePanel::SetDisplayMode(eEngineGUIDisplayMode::All);
        enginePanel::RequestDockLayoutReset();

        ShowNotification("Layout: Reset");
    }

    // F3 steps through the panels, switching to the overlay if not already there.
    if (dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_F3))
    {
        static INT32 focusedPanelIndex = 0;

        if (enginePanel::GetDisplayMode() != eEngineGUIDisplayMode::FocusedOverlay)
        {
            enginePanel::SetDisplayMode(eEngineGUIDisplayMode::FocusedOverlay);
        }
        else
        {
            focusedPanelIndex = (focusedPanelIndex + 1) % focusablePanelCount;
        }

        enginePanel::SetFocusedPanelName(focusablePanels[focusedPanelIndex]);

        ShowNotification("Focus: %s", focusablePanels[focusedPanelIndex]);
    }

    // The interface and mouse look want the cursor for opposite reasons: panels
    // up means a free cursor, panels hidden means look around.
    //
    // Derived from the display mode here rather than set inside the F1 branch,
    // because F1 is not the only key that changes that mode. Going out through
    // F1 and back in through F2, F3 or F4 used to leave the cursor hidden and
    // captured with the panels plainly visible, and nothing put it back.
    {
        // The overlay counts as looking around, not as using the interface. It
        // is a heads up display: you read it while flying, and there is nothing
        // on it to click, so taking the cursor back would only stop the camera.
        const eEngineGUIDisplayMode displayMode = enginePanel::GetDisplayMode();
        const bool bShouldMouseLook =
            (displayMode == eEngineGUIDisplayMode::Hidden) ||
            (displayMode == eEngineGUIDisplayMode::FocusedOverlay);

        // Only on a change: the setter reaches through to the graphics DLL to
        // switch the mouse into relative mode.
        if (bShouldMouseLook != dooms::userinput::UserInput_Server::GetIsMouseLookEnabled())
        {
            dooms::userinput::UserInput_Server::SetIsMouseLookEnabled(bShouldMouseLook);
        }
    }
}

void dooms::ui::EngineGUIServer::OnEndOfFrame()
{
}
