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
#include "imgui.h"

namespace Flux {
	void Viewport::RenderViewport() {
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration 
            | ImGuiWindowFlags_NoMove 
            | ImGuiWindowFlags_NoResize 
            | ImGuiWindowFlags_NoBringToFrontOnFocus 
            | ImGuiWindowFlags_NoNavFocus;

		ImGuiViewport* main_viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(main_viewport->Pos);
		ImGui::SetNextWindowSize(main_viewport->Size);

		ImGui::Begin("Viewport", nullptr, window_flags);

		if (ImGui::BeginChild("Viewport Header", ImVec2(0, 30), true)) {
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

            ImGui::SameLine();

            if (ImGui::Button("Play")) {
            }

            ImGui::SameLine();

            if (ImGui::Button("Pause")) {
            }

            ImGui::EndChild();
		}

        ImGui::Text("3D Scene Here");

        ImGui::End();

    }
}