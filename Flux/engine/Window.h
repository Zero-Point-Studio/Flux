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

#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include "./gui/viewport/viewport.h"
#include "./gui/viewport/explorer.h"
#include "./gui/viewport/ribbon.h"
#include "./gui/viewport/output.h"
#include "./gui/viewport/properties.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace Flux {
	class Window
	{
	public:
		Window(int width, int height, const std::string& title);
		~Window();

		bool shouldClose() const;
		void update();
		void clear(float r, float g, float b, float a);

		GLFWwindow* getNativeWindow() const { return m_window; };

	private:
		GLFWwindow* m_window;
		int m_width, m_height;
		std::string m_title;
		Viewport m_viewport;
		Explorer m_explorer;
		Ribbon m_ribbon;
		Output m_output;
		Properties m_properties;
	};
}