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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <memory>
#include "imgui.h"

#include "../engine/OpenGLManager.h"
#include "../engine/3DRenderer.h"
#include "../engine/Model.h"
#include "../engine/mechanics/camera.h"

extern float vertices[];

namespace Flux {
	extern bool showSettings;
	class OpenGLManager;
	class Renderer3D;
	class Model;

	class Viewport {
	public:
		void Init();
		void RenderViewport();

		std::string modelPath = "assets/sphere.obj";
	private:
		unsigned int fbo;
		unsigned int textureColorBuffer;
		unsigned int rbo;
		unsigned int VAO, VBO;
		unsigned int shaderProgram;
		unsigned int indexCount;
		std::unique_ptr<OpenGLManager> glManager;
		std::unique_ptr<Renderer3D> renderer;
		std::unique_ptr<Model> currentModel;
		std::unique_ptr<Camera> camera;
	};
}