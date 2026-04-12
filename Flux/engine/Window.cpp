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

#include "imgui.h"
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

		if (!m_window) {
			std::cerr << "ERROR: FAILED TO CREATE WINDOW" << std::endl;
		}

		glfwMakeContextCurrent(m_window);

		GLFWimage images[1];
		images[0].pixels = stbi_load("assets/icon.png", &images[0].width, &images[0].height, 0, 4);

		if (images[0].pixels) {
			glfwSetWindowIcon(m_window, 1, images);
			stbi_image_free(images[0].pixels);
		}

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= (1 << 14);

		ImGui::StyleColorsDark();
		
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

	void Window::update()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

		m_viewport.RenderViewport();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwPollEvents();
		glfwSwapBuffers(m_window);
	}

	void Window::clear(float r, float g, float b, float a) {
		glClearColor(r, g, b, a);
		glClear(GL_COLOR_BUFFER_BIT);
	}
}
