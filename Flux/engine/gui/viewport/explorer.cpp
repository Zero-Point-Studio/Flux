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

#include "explorer.h"
#include "imgui.h"

namespace Flux {
	void Explorer::renderExplorer(Viewport& viewport) {
		ImGui::Begin("Explorer");

		if (ImGui::IsWindowHovered()) {
			ImGui::SetWindowFocus();
		}

		if (ImGui::TreeNodeEx("Project Files", ImGuiTreeNodeFlags_DefaultOpen)) {

			DrawVirtualNodes(projectRoot);

			ImGui::TreePop();
		}

		if (ImGui::IsWindowFocused() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
			if (ImGui::GetIO().MouseDragMaxDistanceSqr[ImGuiMouseButton_Right] < 10.0f) {
				ImGui::OpenPopup("ContextMenuExplorer");
			}
		}

		if (ImGui::BeginPopup("ContextMenuExplorer")) {
			if (ImGui::BeginMenu("Create a new Project"))
			{
				char* usrProfile = std::getenv("USERPROFILE");
				std::filesystem::path docsPath = std::filesystem::path(usrProfile) / "Documents" / "FluxProjects" / "NewGame";

				std::filesystem::path projTemplatePath = std::filesystem::current_path() / "templates" / "base_game_folder";

				try
				{
					if (!std::filesystem::exists(docsPath))
					{
						std::filesystem::create_directories(docsPath);
						std::filesystem::create_directories(projTemplatePath);
						std::filesystem::copy(projTemplatePath, docsPath, std::filesystem::copy_options::recursive);

						this->activeFolderPath = docsPath;
						std::cout << "Successfully created at " << docsPath << std::endl;
					}
				} catch (const std::filesystem::filesystem_error& e) {
					std::cerr << "Failed to copy template: " << e.what() << std::endl;
				}

				ImGui::EndMenu();
			}

			ImGui::EndPopup();
		}

		ImGui::End();
	}

	void Explorer::DrawVirtualNodes(virtualFile& file) {
		std::string uniqueID = file.name + "##" + std::to_string((uintptr_t)&file);

		if (file.type == fileType::Folder) {
			if (ImGui::TreeNode(uniqueID.c_str())) {
				for (auto& child : file.children) {
					DrawVirtualNodes(child); // The recursive loop
				}
				ImGui::TreePop();
			}
		} else {
			if (ImGui::Selectable(uniqueID.c_str())) {
				// Future logic for file selection
			}

		}
	}
}
