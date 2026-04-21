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

		ImVec2 mouseInCanvas = ImVec2(ImGui::GetMousePos().x - imagePos.x, ImGui::GetMousePos().y - imagePos.y);

		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver()) {
			HandleObjectSelection(mouseInCanvas, size, proj, view);
		}

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MODEL_FILE")) {
				const char* droppedPath = (const char*)payload->Data;

				GameObject newObj;
				newObj.name = "New Model";
				newObj.modelBlueprint = GetOrLoadModel(droppedPath);
				sceneObjects.push_back(newObj);
			}
			ImGui::EndDragDropTarget();
		}

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

	bool Viewport::CheckSphereHit(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 objCenter, float radius) {
		glm::vec3 oc = rayOrigin - objCenter;
		float b = glm::dot(oc, rayDir);
		float c = glm::dot(oc, oc) - radius * radius;
		float h = b * b - c;
		return h > 0.0f;
	}

	void Viewport::HandleObjectSelection(ImVec2 mousePos, ImVec2 imageSize, glm::mat4 proj, glm::mat4 view) {
		float ndcX = (2.0f * mousePos.x) / imageSize.x - 1.0f;
		float ndcY = 1.0f - (2.0f * mousePos.y) / imageSize.y;

		glm::vec4 rayClip = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
		glm::vec4 rayEye = glm::inverse(proj) * rayClip;
		rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
		glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
		glm::vec3 rayOrigin = camera->Position;

		int bestHitIndex = -1;
		float closestDistance = 999999.0f;

		for (int i = 0; i < sceneObjects.size(); i++) {
			float objRadius = glm::length(sceneObjects[i].scale) * 0.75f;

			if (CheckSphereHit(rayOrigin, rayWorld, sceneObjects[i].position, objRadius)) {
				float dist = glm::distance(rayOrigin, sceneObjects[i].position);
				if (dist < closestDistance) {
					closestDistance = dist;
					bestHitIndex = i;
				}
			}
		}
		selectedObjectIndex = bestHitIndex;
	}
}