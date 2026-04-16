/*
* Flux is a free, versatile game engine built for developers of all skill levels.
* Copyright (C) 2026  Zero Point Studio (Idkthisguy)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "Window.h"
#include <iostream>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Flux {
	Window::Window(int width, int height, const std::string& title)
		: m_width(width), m_height(height), m_title(title)
	{
		if (!glfwInit()) {
			std::cerr << "ERROR: FAILED TO INITIALIZE GLFW" << std::endl;
		}

		m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), NULL, NULL);

		HWND hwnd = glfwGetWin32Window(m_window);

		BOOL useDarkMode = TRUE;

		DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));

		if (!m_window) {
			std::cerr << "ERROR: FAILED TO CREATE WINDOW" << std::endl;
		}

		glfwMakeContextCurrent(m_window);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			std::cerr << "ERROR: FAILED TO INITIALIZE GLAD" << std::endl;
		}

		GLFWimage images[1];
		images[0].pixels = stbi_load("assets/icon.png", &images[0].width, &images[0].height, 0, 4);

		if (images[0].pixels) {
			glfwSetWindowIcon(m_window, 1, images);
			stbi_image_free(images[0].pixels);
		}

		m_viewport.Init();

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		
		
		ImGui_ImplGlfw_InitForOpenGL(m_window, true);
		ImGui_ImplOpenGL3_Init("#version 460");
	}

	Window::~Window()
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
	}


	bool Window::shouldClose() const
	{
		return glfwWindowShouldClose(m_window);
	}

	void Window::update() // Main render
	{
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

		if (firstTime) {
			firstTime = false;

			ImGui::DockBuilderRemoveNode(dockspace_id);
			ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspace_id, dockSize);

			ImGuiID dock_id_left;
			ImGuiID dock_id_right;
			ImGuiID dock_id_bottom;
			ImGuiID dock_id_bottomRight;
			ImGuiID dock_id_center = dockspace_id;

			dock_id_left = ImGui::DockBuilderSplitNode(dock_id_center, ImGuiDir_Left, 0.2f, nullptr, &dock_id_center);
			dock_id_bottom = ImGui::DockBuilderSplitNode(dock_id_center, ImGuiDir_Down, 0.25f, nullptr, &dock_id_center);
			dock_id_right = ImGui::DockBuilderSplitNode(dock_id_center, ImGuiDir_Right, 0.25f, nullptr, &dock_id_center);
			dock_id_bottomRight = ImGui::DockBuilderSplitNode(dock_id_right, ImGuiDir_Down, 0.5f, nullptr, &dock_id_right);

			ImGui::DockBuilderDockWindow("Viewport", dock_id_center);
			ImGui::DockBuilderDockWindow("Explorer", dock_id_right);
			ImGui::DockBuilderDockWindow("Output", dock_id_bottom);
			ImGui::DockBuilderDockWindow("Properties", dock_id_bottomRight);

			ImGui::DockBuilderFinish(dockspace_id);
		}
		ImGui::End();

		m_viewport.RenderViewport();
		m_explorer.renderExplorer();
		m_ribbon.renderRibbon();
		m_output.renderOutput();
		m_properties.renderProperties();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		glfwPollEvents();
		glfwSwapBuffers(m_window);
	}

	void Window::clear(float r, float g, float b, float a) {
		glClearColor(r, g, b, a);
		glClear(GL_COLOR_BUFFER_BIT);
	}
}
