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
#include "explorer.h"

#include <unordered_map>

extern float vertices[];

namespace Flux {
	extern bool showSettings;
	class OpenGLManager;
	class Renderer3D;
	class Model;

	struct GameObject{
		std::string name;
		std::shared_ptr<Model> modelBlueprint;

		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 rotation = glm::vec3(0.0f);
		glm::vec3 scale = glm::vec3(1.0f);

		glm::mat4 GetTransformMatrix() {
			float matrix[16];
			float t[3] = { position.x, position.y, position.z };
			float r[3] = { rotation.x, rotation.y, rotation.z };
			float s[3] = { scale.x, scale.y, scale.z };
			ImGuizmo::RecomposeMatrixFromComponents(t, r, s, matrix);
			return glm::make_mat4(matrix);
		}
	};

	class Viewport {
	public:
		void Init();
		void RenderViewport();

		std::vector<GameObject> sceneObjects;
		int selectedObjectIndex = -1;

	private:
		bool CheckSphereHit(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 objCenter, float radius);
		void HandleObjectSelection(ImVec2 mousePos, ImVec2 imageSize, glm::mat4 proj, glm::mat4 view);
		
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
		std::unordered_map<std::string, std::shared_ptr<Model>> modelRegistry;
		std::shared_ptr<Model> GetOrLoadModel(const std::string& path);
	};
}