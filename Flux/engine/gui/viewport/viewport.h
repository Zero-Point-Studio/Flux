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
#include <string>
#include <memory>
#include <filesystem>
#include "imgui.h"
#include "ImGuizmo.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

#include "../engine/OpenGLManager.h"
#include "../engine/3DRenderer.h"
#include "../engine/Model.h"
#include "../engine/mechanics/camera.h"
#include "heiarchy.h"
#include "Textureloader.h"

namespace Flux {

	extern bool showSettings;
	class OpenGLManager;
	class Renderer3D;

	class Viewport {
	public:
		void Init();
		void RenderViewport(Heiarchy& heiarchy);

		std::filesystem::path activeProjectPath;

	private:
		std::unique_ptr<OpenGLManager> glManager;
		std::unique_ptr<Renderer3D>    renderer;
		std::unique_ptr<Camera>        camera;

		std::shared_ptr<Model> ghostModel;
		std::string            ghostPath;
		glm::vec3              ghostPos  = glm::vec3(0.f);
		bool                   isDraggingModel = false;

		bool CheckSphereHit(glm::vec3 ro, glm::vec3 rd, glm::vec3 center, float radius);
		void HandleObjectSelection(ImVec2 mousePos, ImVec2 sz,
								   glm::mat4 proj, glm::mat4 view,
								   Heiarchy& heiarchy);

		glm::vec3 RaycastToGroundPlane(ImVec2 mousePos, ImVec2 imagePos, ImVec2 size,
									   glm::mat4 proj, glm::mat4 view);

		void DrawLightGizmos(Heiarchy& heiarchy,
							 glm::mat4 view, glm::mat4 proj,
							 ImVec2 imagePos, ImVec2 size);
	};

}