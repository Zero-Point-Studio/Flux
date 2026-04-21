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

#include "viewport.h"
#include "OpenGLManager.h"
#include "3DRenderer.h"
#include "Model.h"
#include <iostream>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include "ImGuizmo.h"

namespace Flux {
	extern int currentTool;
	extern bool showSettings;

	std::shared_ptr<Model> Viewport::GetOrLoadModel(const std::string& path) {
		if (modelRegistry.find(path) == modelRegistry.end()) {
			// We don't have it yet, so we load it
			modelRegistry[path] = std::make_shared<Model>(path);
		}
		return modelRegistry[path]; // Return the shared pointer
	}

	void Viewport::Init() {
		glManager = std::make_unique<OpenGLManager>();
		renderer = std::make_unique<Renderer3D>();
		camera = std::make_unique<Camera>(glm::vec3(0.0f, 5.0f, 15.0f));

		glManager->Init(1280, 720);
		renderer->Init();

		GameObject defaultCube;
		defaultCube.name = "Cube 1";
		defaultCube.modelBlueprint = GetOrLoadModel("assets/models/cube.obj");
		sceneObjects.push_back(defaultCube);
		selectedObjectIndex = 0;
	}

	void Viewport::RenderViewport() {

		if (showSettings) {
			if (ImGui::Begin("Viewport Settings", &showSettings)) {
				ImGui::SliderFloat("Camera Sensitivity", &camera->MouseSensitivity, 0.01f, 0.5f);
				ImGui::SliderFloat("Move Speed", &camera->MovementSpeed, 1.0f, 20.0f);
			}
			ImGui::End();
		}

		ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar);
		if (ImGui::IsWindowHovered()) {
			ImGui::SetWindowFocus();
		}
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

		ImVec2 size = ImGui::GetContentRegionAvail();
		float aspectRatio = size.x / size.y;

		float deltaTime = ImGui::GetIO().DeltaTime;
		if (ImGui::IsWindowFocused() && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
			if (ImGui::IsKeyDown(ImGuiKey_W)) camera->ProcessKeyboard(FORWARD, deltaTime);
			if (ImGui::IsKeyDown(ImGuiKey_S)) camera->ProcessKeyboard(BACKWARD, deltaTime);
			if (ImGui::IsKeyDown(ImGuiKey_A)) camera->ProcessKeyboard(LEFT, deltaTime);
			if (ImGui::IsKeyDown(ImGuiKey_D)) camera->ProcessKeyboard(RIGHT, deltaTime);

			ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
			camera->ProcessMouseMovement(mouseDelta.x, -mouseDelta.y);
		}

		glm::mat4 view = camera->GetViewMatrix();
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

		glManager->Resize((int)size.x, (int)size.y);
		glManager->Bind();
		glViewport(0, 0, (int)size.x, (int)size.y);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		for (auto& object : sceneObjects) {
			if (object.modelBlueprint) {
				glm::mat4 transform = object.GetTransformMatrix();

				renderer->DrawScene(*object.modelBlueprint, transform, view, proj, 0.0f);
			}
		}

		glManager->Unbind();

		ImVec2 imagePos = ImGui::GetCursorScreenPos();

		ImGui::Image((void*)(intptr_t)glManager->GetTexture(), size, ImVec2(0, 1), ImVec2(1, 0));

		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();

		ImGuizmo::SetRect(imagePos.x, imagePos.y, size.x, size.y);

		ImGuizmo::OPERATION currentOp = ImGuizmo::TRANSLATE;
		if (currentTool == 1) currentOp = ImGuizmo::ROTATE;
		if (currentTool == 2) currentOp = ImGuizmo::SCALE;

		if (selectedObjectIndex != -1 && selectedObjectIndex < sceneObjects.size()) {
			auto& target = sceneObjects[selectedObjectIndex];

			glm::mat4 modelMatrix = target.GetTransformMatrix();

			ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj),
				currentOp, ImGuizmo::LOCAL, glm::value_ptr(modelMatrix));

			if (ImGuizmo::IsUsing()) {
				float t[3], r[3], s[3];
				ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(modelMatrix), t, r, s);

				target.position = glm::vec3(t[0], t[1], t[2]);
				target.rotation = glm::vec3(r[0], r[1], r[2]);
				target.scale = glm::vec3(s[0], s[1], s[2]);
			}
		}

		if (ImGui::IsWindowFocused() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
			if (ImGui::GetIO().MouseDragMaxDistanceSqr[ImGuiMouseButton_Right] < 10.0f) {
				ImGui::OpenPopup("ContextMenu");
			}
		}

		if (ImGui::BeginPopup("ContextMenu")) {
			if (ImGui::BeginMenu("Add Model")) {
				if (ImGui::MenuItem("Add Cube")) {
					GameObject newCube;
					newCube.name = "Cube " + std::to_string(sceneObjects.size() + 1);
					newCube.modelBlueprint = GetOrLoadModel("assets/models/cube.obj");
					sceneObjects.push_back(newCube);
					selectedObjectIndex = sceneObjects.size() - 1;
				}

				if (ImGui::MenuItem("Add Sphere")) {
					GameObject newSphere;
					newSphere.name = "Sphere " + std::to_string(sceneObjects.size() + 1);
					newSphere.modelBlueprint = GetOrLoadModel("assets/models/sphere.obj");
					sceneObjects.push_back(newSphere);
					selectedObjectIndex = sceneObjects.size() - 1;
				}

				if (ImGui::MenuItem("Add Monkey")) {
					GameObject newMonkey;
					newMonkey.name = "Monkey " + std::to_string(sceneObjects.size() + 1);
					newMonkey.modelBlueprint = GetOrLoadModel("assets/models/monkey.obj");
					sceneObjects.push_back(newMonkey);
					selectedObjectIndex = sceneObjects.size() - 1;
				}
				ImGui::EndMenu();
			}
			ImGui::EndPopup();
		}

		ImGui::End();
	}
}