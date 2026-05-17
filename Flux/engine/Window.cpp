#include "Window.h"
#include <iostream>

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#endif

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <SDL3/SDL.h>

namespace Flux
{
    Window::Window(int width, int height, const std::string& title)
        : m_width(width), m_height(height), m_title(title)
    {

        if (!glfwInit())
        {
            std::cerr << "ERROR: FAILED TO INITIALIZE GLFW" << std::endl;
        }

        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

        m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), NULL, NULL);

        if (!m_window)
        {
            const char* desc = nullptr;
            glfwGetError(&desc);
            std::cerr << "FATAL: glfwCreateWindow failed: " << (desc ? desc : "unknown") << std::endl;
            glfwTerminate();
            throw std::runtime_error("glfwCreateWindow failed");
        }

#if defined(_WIN32)
        HWND hwnd = glfwGetWin32Window(m_window);
        BOOL useDarkMode = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));

        HICON hIcon = LoadIcon(GetModuleHandle(NULL), "IDI_ICON1");

    if (hIcon) {
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }
#endif

        glfwMakeContextCurrent(m_window);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cerr << "ERROR: FAILED TO INITIALIZE GLAD" << std::endl;
        }

        GLFWimage images[1];
        images[0].pixels = stbi_load("assets/icon.png", &images[0].width, &images[0].height, 0, 4);

        if (images[0].pixels)
        {
            glfwSetWindowIcon(m_window, 1, images);
            stbi_image_free(images[0].pixels);
        }

        m_explorer.textEditor = &m_texteditor;
        m_explorer.ribbonPtr = &m_ribbon;

        m_ribbon.luaEnginePtr = &m_luaEngine;
        m_ribbon.textEditorPtr = &m_texteditor;
        m_ribbon.explorerPtr = &m_explorer;
        m_ribbon.heiarchyPtr = &m_heiarchy;
        m_ribbon.viewportPtr = &m_viewport;
        m_ribbon.LoadPreferences();
        m_texteditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());

        m_viewport.ribbonPtr = &m_ribbon;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        io.Fonts->AddFontDefault();

        (void)io;

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;


        ImGui_ImplGlfw_InitForOpenGL(m_window, true);
        ImGui_ImplOpenGL3_Init("#version 410");

        m_viewport.Init();
        m_heiarchy.setup();
        m_luaEngine.init();

        Flux::SplashConfig splash;
        splash.title    = "Flux Engine";
        splash.subtitle = "Loading project...";
        Flux::RunSplashScreen(m_window, splash);
    }

    Window::~Window()
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }


    bool Window::shouldClose() const
    {
        if (m_window == nullptr) 
        {
            return true; 
        }
        return glfwWindowShouldClose(m_window);
    }

    void Window::update() 
    {
        if (m_window == nullptr) 
        {
            return; 
        }

        if (m_pendingStop) {
            m_pendingStop = false;
            StopRuntimeEngine();
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImVec2 dockPos = viewport->Pos;
        dockPos.y += 55.0f;

        ImVec2 dockSize = viewport->Size;
        dockSize.y -= 55.0f;

        static bool firstTime = true;
        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

        ImGui::SetNextWindowPos(dockPos);
        ImGui::SetNextWindowSize(dockSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags host_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoBackground;

        ImGui::Begin("MainDockHost", nullptr, host_flags);

        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        if (firstTime)
        {
            firstTime = false;

            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, dockSize);

            ImGuiID dock_id_left;
            ImGuiID dock_id_right;
            ImGuiID dock_id_bottom;
            ImGuiID dock_id_bottomRight;
            ImGuiID dock_id_center = dockspace_id;

            dock_id_left = ImGui::DockBuilderSplitNode(dock_id_center, ImGuiDir_Left, 0.20f, nullptr, &dock_id_center);
            dock_id_bottom =
                ImGui::DockBuilderSplitNode(dock_id_center, ImGuiDir_Down, 0.25f, nullptr, &dock_id_center);
            dock_id_right =
                ImGui::DockBuilderSplitNode(dock_id_center, ImGuiDir_Right, 0.30f, nullptr, &dock_id_center);
            dock_id_bottomRight = ImGui::DockBuilderSplitNode(dock_id_right, ImGuiDir_Down, 0.5f, nullptr,
                                                              &dock_id_right);

            ImGui::DockBuilderDockWindow("Viewport", dock_id_center);
            ImGui::DockBuilderDockWindow("###UniqueEditorID", dock_id_center);
            ImGui::DockBuilderDockWindow("Explorer", dock_id_right);
            ImGui::DockBuilderDockWindow("Output", dock_id_bottom);
            ImGui::DockBuilderDockWindow("Properties", dock_id_bottomRight);
            ImGui::DockBuilderDockWindow("Heiarchy", dock_id_left);


            ImGui::DockBuilderFinish(dockspace_id);
        }
        ImGui::End();

        if (m_luaEngine.isRunning) {
            m_luaEngine.step();
        }


        m_ribbon.renderRibbon();
        m_explorer.renderExplorer(m_viewport);

        static std::filesystem::path lastLoadedProject;
        if (m_explorer.activeFolderPath != lastLoadedProject && !m_explorer.activeFolderPath.empty()) {
            lastLoadedProject = m_explorer.activeFolderPath;
            m_ribbon.LoadProjectSettings(m_explorer.activeFolderPath);
        }

        if (!m_explorer.activeFolderPath.empty()) {
            if (m_ribbon.editorLocked) {
                ImGui::BeginDisabled(true);
            }

            m_viewport.RenderViewport(m_heiarchy);
            m_properties.renderProperties(&m_heiarchy);
            m_heiarchy.renderHeiarchy(m_viewport.activeProjectPath);
            if (m_ribbon.editorLocked) {
                ImGui::EndDisabled(); 
            }

            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S) && !m_ribbon.editorLocked) {
                std::string scenePath = m_explorer.activeFolderPath.string() + "/scene.fscn";
                m_sceneSerializer.Save(m_heiarchy, scenePath, m_explorer.activeFolderPath);
                Output::addLog("Scene saved to " + scenePath);
            }
            
        } else {
            ImGui::Begin("Viewport");
            ImGui::SetWindowFontScale(2.0);
            ImGui::Text("No project loaded. Please Open or Create a project in the Explorer.");
            ImGui::SetWindowFontScale(1.0);
            ImGui::End();
        }

        if (m_ribbon.playToggledFrame) {
            if (m_ribbon.editorLocked) {
                StartRuntimeEngine();
            } else {
                StopRuntimeEngine();
            }
            m_ribbon.playToggledFrame = false;
        }

        m_output.renderOutput();
        
        
        if (m_explorer.isEditorVisible) {
            
            std::string editorTitle = "Text Editor - " + m_explorer.activeScriptName;
            if (m_explorer.isEditorUnsaved) {
                editorTitle += " *";
            }
            editorTitle += "###UniqueEditorID";

            ImGui::Begin(editorTitle.c_str(), &m_explorer.isEditorVisible);

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
                ImGuiIO& io = ImGui::GetIO();
                if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
                    if (!m_explorer.activeFilePath.empty()) {
                        std::ofstream outFile(m_explorer.activeFilePath);
                        if (outFile.is_open()) {
                            outFile << m_texteditor.GetText();
                            outFile.close();    

                            m_explorer.isEditorUnsaved = false;
                        }
                    }
                }
            }
            
            m_texteditor.Render("CodeEditorWidget"); 
            
            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwPollEvents();

        m_runtime.SyncCamera(m_viewport.camera->Position, m_viewport.camera->Position + m_viewport.camera->Front);

        m_runtime.Update();

        if (!m_runtime.isRunning && m_ribbon.editorLocked)
            m_pendingStop = true;

        if (m_window != nullptr)
        {
            glfwMakeContextCurrent(m_window);
            glfwSwapBuffers(m_window);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Window::clear(float r, float g, float b, float a)
    {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT);
    }


    void Window::StartRuntimeEngine() {
        Output::addLog("Starting runtime engine...");

        std::string projName = m_explorer.projectRoot.name;
        if (projName == "Project" || projName.empty()) projName = "Flux Game";

        auto& ps = m_ribbon.projectSettings;

        if (ps.useStartupScene) {
            std::filesystem::path scenePath = m_explorer.activeFolderPath / ps.startupScene;
            if (std::filesystem::exists(scenePath)) {
                m_sceneSerializer.Load(m_heiarchy, scenePath, m_explorer.activeFolderPath);
                Output::addLog("Runtime loading startup scene: " + std::string(ps.startupScene));
            } else {
                Output::addLog("WARNING: Startup scene not found: " + scenePath.string() + ", using current hierarchy.");
            }
        }

        m_runtimeNodes = m_heiarchy.nodes;

        m_runtime.Start(projName, m_explorer.activeFolderPath, m_runtimeNodes,
                        ps.runtimeWidth, ps.runtimeHeight);
    }

    void Window::StopRuntimeEngine() {
        m_runtime.Stop();

        if (m_ribbon.luaEnginePtr) {
            m_ribbon.luaEnginePtr->isRunning = false;
        }

        m_ribbon.editorLocked = false;

        m_runtimeNodes.clear();

        Output::addLog("Runtime stopped.");
    }
}
