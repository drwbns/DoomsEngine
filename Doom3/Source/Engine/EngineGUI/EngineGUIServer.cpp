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

#include <IO/UserInput_Server.h>
#include <Graphics/GraphicsAPI/graphicsAPISetting.h>
#include <Graphics/graphicsSetting.h>

#include <DObject/DObjectGlobals.h>
#include <Rendering/Pipeline/GraphicsPipeLine.h>
#include <Rendering/Pipeline/PipeLines/DefaultGraphcisPipeLine.h>

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
    dooms::ui::eEngineGUIDisplayMode gDisplayMode = dooms::ui::eEngineGUIDisplayMode::All;
    char gFocusedPanelName[128] = "DrawCall";
    FLOAT32 gOverlayAlpha = 0.35f;

    // Whether the last BeginPanel actually opened a window, so EndPanel knows
    // if it owes ImGui a matching End().
    bool gPanelWindowOpened = false;

    // Set by F4, cleared once the dockspace has rebuilt its layout.
    bool gDockLayoutResetRequested = false;
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
        bool mIsDistanceEnabled;
        bool mIsMaskedSWOcclusionEnabled;
    };

    const OcclusionMode gOcclusionModes[] =
    {
        { "All",                 true,  true,  true  },
        { "None",                false, false, false },
        { "Frustum only",        true,  false, false },
        { "Frustum + Distance",  true,  true,  false },
        { "Frustum + Occlusion", true,  false, true  }
    };

    constexpr INT32 gOcclusionModeCount
        = static_cast<INT32>(sizeof(gOcclusionModes) / sizeof(gOcclusionModes[0]));

    // Starts at "All", which is how the engine boots, so the label is honest
    // before F7 is ever pressed.
    INT32 gOcclusionModeIndex = 0;

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

        cullingSystem->SetEnabledCullingModule(
            culling::EveryCulling::CullingModuleType::ViewFrustumCulling, occlusionMode.mIsViewFrustumEnabled);
        cullingSystem->SetEnabledCullingModule(
            culling::EveryCulling::CullingModuleType::DistanceCulling, occlusionMode.mIsDistanceEnabled);
        cullingSystem->SetEnabledCullingModule(
            culling::EveryCulling::CullingModuleType::MaskedSWOcclusionCulling, occlusionMode.mIsMaskedSWOcclusionEnabled);

        gOcclusionModeIndex = index;
    }

    void RenderVisualisationPanel()
    {
        using namespace dooms::graphics;

        if (dooms::ui::enginePanel::BeginPanel("Visualisation"))
        {
            ImGui::TextDisabled("F6 cycles: %s", gVisualisationCycle[gVisualisationCycleIndex].mName);
            ImGui::TextDisabled("F7 culling: %s", gOcclusionModes[gOcclusionModeIndex].mName);

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
    }

    // F2 swaps between the docked view and a single focused overlay.
    if (dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_F2))
    {
        enginePanel::SetDisplayMode(
            (enginePanel::GetDisplayMode() == eEngineGUIDisplayMode::FocusedOverlay)
                ? eEngineGUIDisplayMode::All
                : eEngineGUIDisplayMode::FocusedOverlay);
    }

    // F11 toggles borderless fullscreen, the usual binding for it.
    if (dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_F11))
    {
        ToggleBorderlessFullscreen();
    }

    // F6 steps through the visualisations one at a time, starting from off.
    if (dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_F6))
    {
        ApplyVisualisationCycleIndex((gVisualisationCycleIndex + 1) % gVisualisationCycleCount);

        D_RELEASE_LOG(eLogType::D_LOG, "Visualisation : %s", gVisualisationCycle[gVisualisationCycleIndex].mName);
    }

    // F7 steps through the culling configurations, to compare what each
    // technique is actually buying in this scene.
    if (dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_F7))
    {
        ApplyOcclusionModeIndex((gOcclusionModeIndex + 1) % gOcclusionModeCount);

        D_RELEASE_LOG(eLogType::D_LOG, "Culling : %s", gOcclusionModes[gOcclusionModeIndex].mName);
    }

    // F4 puts the panels back into the default arrangement.
    if (dooms::userinput::UserInput_Server::GetKeyDown(eKEY_CODE::KEY_F4))
    {
        enginePanel::SetDisplayMode(eEngineGUIDisplayMode::All);
        enginePanel::RequestDockLayoutReset();
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
