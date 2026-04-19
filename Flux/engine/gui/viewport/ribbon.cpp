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

#include "ribbon.h"
#include "imgui.h"
#include <iostream>

namespace Flux {
	int currentTool = 0;
	bool showSettings = false;
	void Ribbon::renderRibbon() {
		ImGuiViewport* main_viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(main_viewport->Pos);
		ImGui::SetNextWindowSize(ImVec2(main_viewport->Size.x, 55));

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoScrollbar
			| ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_MenuBar;

		ImGui::Begin("###Ribbon", nullptr, window_flags);
		if (ImGui::IsWindowHovered()) {
			ImGui::SetWindowFocus();
		}

		if (ImGui::BeginMenuBar()) {
			drawFileMenu();
			drawEditMenu();

			ImGui::Separator();

			drawTransformTools();

			ImGui::EndMenuBar();
		}

		ImGui::SetCursorPosX(main_viewport->Size.x * 0.5f - 50.0f);
		drawProjectControls();

		ImGui::End();
	}


	void Ribbon::drawTransformTools() {
		if (ImGui::RadioButton("Select", currentTool == 3)) { currentTool = 3; }
		ImGui::SameLine();
		if (ImGui::RadioButton("Move", currentTool == 0)) { currentTool = 0; }
		ImGui::SameLine();
		if (ImGui::RadioButton("Rotate", currentTool == 1)) { currentTool = 1; }
		ImGui::SameLine();
		if (ImGui::RadioButton("Scale", currentTool == 2)) { currentTool = 2; }
	}

	void Ribbon::drawFileMenu() {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New Project")) {}
			if (ImGui::MenuItem("Open...")) {}
			if (ImGui::MenuItem("Save")) {}
			ImGui::EndMenu();
		}
	}

	void Ribbon::drawEditMenu() {
		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Undo")) {}
			if (ImGui::MenuItem("Redo")) {}
			if (ImGui::MenuItem("Viewport Settings", nullptr, showSettings)) {
				showSettings = !showSettings;
			}
			ImGui::EndMenu();
		}
	}

	void Ribbon::drawProjectControls() {
		if (ImGui::Button("Play")) {}
		ImGui::SameLine();
		if (ImGui::Button("Pause")) {}
	}
}