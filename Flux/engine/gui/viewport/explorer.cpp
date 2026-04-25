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
#include <cstring>
#include <iostream>

namespace Flux
{

	std::filesystem::path Explorer::resolveUniqueName(
	    const std::filesystem::path& parentDir,
	    const std::string& baseStem,
	    const std::string& ext) const
	{
		std::filesystem::path candidate = parentDir / (baseStem + ext);
		int counter = 1;
		while (std::filesystem::exists(candidate)) {
			candidate = parentDir /
			    (baseStem + " (" + std::to_string(counter) + ")" + ext);
			++counter;
		}
		return candidate;
	}


	void Explorer::renderExplorer(Viewport& viewport) {
		ImGui::Begin("Explorer");

		if (ImGui::IsWindowHovered()) {
			ImGui::SetWindowFocus();
		}

		if (ImGui::TreeNodeEx("Project Files", ImGuiTreeNodeFlags_DefaultOpen)) {
			DrawVirtualNodes(projectRoot);
			ImGui::TreePop();
		}

		if (ImGui::IsWindowFocused() &&
		    ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
		    ImGui::GetIO().MouseDragMaxDistanceSqr[ImGuiMouseButton_Right] < 10.0f)
		{
			ImGui::OpenPopup("ContextMenuExplorer");
		}

		if (ImGui::BeginPopup("ContextMenuExplorer")) {
			if (ImGui::MenuItem("Create a new Project")) {
				std::filesystem::path searchPath = std::filesystem::current_path();
				bool found = false;
				for (int i = 0; i < 5; ++i) {
					auto candidate = searchPath / "templates" /
					    "Project_Templates" / "base_game_folder_lua";
					if (std::filesystem::exists(candidate)) {
						pendingTemplateRoot = candidate;
						found = true;
						break;
					}
					if (searchPath.has_parent_path())
						searchPath = searchPath.parent_path();
					else
						break;
				}
				if (found) {
					std::strncpy(newProjectNameBuf, "NewGame",
					             sizeof(newProjectNameBuf) - 1);
					newProjectNameBuf[sizeof(newProjectNameBuf) - 1] = '\0';
					showNewProjectModal = true;
					ImGui::CloseCurrentPopup();
				} else {
					std::cerr << "ERROR: template folder not found near "
					          << std::filesystem::current_path() << "\n";
				}
			}
			ImGui::EndPopup();
		}

		if (showNewProjectModal) {
			ImGui::OpenPopup("Name Your Project");
			showNewProjectModal = false;
		}

		float modalW = ImGui::GetMainViewport()->Size.x * 0.30f;
		if (modalW < 320.f) modalW = 320.f;

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(modalW, 0.f), ImGuiCond_Always);

		if (ImGui::BeginPopupModal("Name Your Project", nullptr,
		                           ImGuiWindowFlags_NoResize))
		{
			ImGui::Text("Project name:");
			ImGui::Spacing();

			ImGui::SetNextItemWidth(-1.f);

			if (ImGui::IsWindowAppearing())
				ImGui::SetKeyboardFocusHere();

			bool confirm =
			    ImGui::InputText("##projname", newProjectNameBuf,
			                     sizeof(newProjectNameBuf),
			                     ImGuiInputTextFlags_EnterReturnsTrue);

			ImGui::Spacing();

			bool nameEmpty = (newProjectNameBuf[0] == '\0');
			if (nameEmpty) ImGui::BeginDisabled();

			float btnW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

			if (ImGui::Button("Create", ImVec2(btnW, 0)) || (confirm && !nameEmpty)) {
				char* usrProfile = std::getenv("USERPROFILE");
				if (usrProfile) {
					std::filesystem::path docsBase =
					    std::filesystem::path(usrProfile) /
					    "Documents" / "FluxProjects";

					std::filesystem::path docsPath =
					    resolveUniqueName(docsBase, std::string(newProjectNameBuf), "");

					try {
						std::filesystem::create_directories(docsPath);
						std::filesystem::copy(
						    pendingTemplateRoot, docsPath,
						    std::filesystem::copy_options::recursive);

						activeFolderPath = docsPath;
						projectRoot.name = docsPath.filename().string();
						syncFiles(docsPath, projectRoot);

						std::cout << "Project created at: " << docsPath << "\n";
					} catch (const std::filesystem::filesystem_error& e) {
						std::cerr << "COPY FAILED: " << e.what() << "\n";
					}
				}
				ImGui::CloseCurrentPopup();
			}

			if (nameEmpty) ImGui::EndDisabled();

			ImGui::SameLine();

			if (ImGui::Button("Cancel", ImVec2(btnW, 0)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}

		ImGui::End();

		if (refreshRequested) {
			if (!projectRoot.path.empty() &&
			    std::filesystem::exists(projectRoot.path))
			{
				renamingNode = nullptr;
				syncFiles(projectRoot.path, projectRoot);
			}
			refreshRequested = false;
		}

		if (!pathToDelete.empty()) {
			try {
				if (std::filesystem::exists(pathToDelete))
					std::filesystem::remove_all(pathToDelete);
				if (!projectRoot.path.empty() &&
				    std::filesystem::exists(projectRoot.path))
				{
					renamingNode = nullptr;
					syncFiles(projectRoot.path, projectRoot);
				}
			} catch (const std::filesystem::filesystem_error& e) {
				std::cerr << "Delete failed: " << e.what() << "\n";
			}
			pathToDelete = "";
		}
	}


	void Explorer::DrawVirtualNodes(virtualFile& file) {
		std::string uid = "##" + std::to_string(reinterpret_cast<uintptr_t>(&file));

		if (renamingNode == &file) {
			ImGui::SetNextItemWidth(-1);
			ImGui::SetKeyboardFocusHere();

			bool commit =
			    ImGui::InputText(("##ren" + uid).c_str(),
			                     renameBuffer, sizeof(renameBuffer),
			                     ImGuiInputTextFlags_EnterReturnsTrue |
			                     ImGuiInputTextFlags_AutoSelectAll);

			bool cancelled = ImGui::IsItemDeactivated() &&
			                 !ImGui::IsItemActivated();

			if (commit && renameBuffer[0] != '\0') {
				std::filesystem::path parentDir = file.path.parent_path();
				std::string stem(renameBuffer);
				std::filesystem::path typedPath(stem);
				std::string typedStem = typedPath.stem().string();
				std::string typedExt  = typedPath.extension().string();

				if (file.type == fileType::Folder) {
					typedStem = stem;
					typedExt  = "";
				}
				if (typedExt.empty() && file.type != fileType::Folder)
					typedExt = file.path.extension().string();

				std::filesystem::path newPath =
				    resolveUniqueName(parentDir, typedStem, typedExt);

				if (newPath != file.path) {
					try {
						std::filesystem::rename(file.path, newPath);
					} catch (const std::filesystem::filesystem_error& e) {
						std::cerr << "Rename failed: " << e.what() << "\n";
					}
				}
				renamingNode     = nullptr;
				refreshRequested = true;
			} else if (cancelled) {
				renamingNode = nullptr;
			}
			return;
		}

		if (file.type == fileType::Folder) {
			ImGuiTreeNodeFlags folderFlags =
			    ImGuiTreeNodeFlags_SpanFullWidth |
			    ImGuiTreeNodeFlags_OpenOnArrow;

			bool node_open = ImGui::TreeNodeEx(
			    (file.name + uid).c_str(), folderFlags);

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
				std::string payloadPath = file.path.string();
				ImGui::SetDragDropPayload("EXPLORER_FILE",
				    payloadPath.c_str(), payloadPath.size() + 1);
				ImGui::Text("Moving  %s/", file.name.c_str());
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload =
				        ImGui::AcceptDragDropPayload("EXPLORER_FILE"))
				{
					std::string srcStr(static_cast<const char*>(payload->Data));
					std::filesystem::path src(srcStr);

					bool srcInsideDst = false;
					{
						auto rel = std::filesystem::relative(file.path, src);
						auto it = rel.begin();
						if (it != rel.end() && it->string() != "..")
							srcInsideDst = true;
					}

					if (std::filesystem::exists(src) &&
					    src.parent_path() != file.path &&
					    !srcInsideDst)
					{
						std::string stem, ext;
						if (std::filesystem::is_directory(src)) {
							stem = src.filename().string();
							ext  = "";
						} else {
							stem = src.stem().string();
							ext  = src.extension().string();
						}
						std::filesystem::path dst =
						    resolveUniqueName(file.path, stem, ext);
						try {
							std::filesystem::rename(src, dst);
							renamingNode     = nullptr;
							refreshRequested = true;
						} catch (const std::filesystem::filesystem_error& e) {
							std::cerr << "Move failed: " << e.what() << "\n";
						}
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (ImGui::BeginPopupContextItem()) {
				activeFolderPath = file.path;

				if (ImGui::MenuItem("Rename")) {
					renamingNode = &file;
					std::strncpy(renameBuffer, file.name.c_str(),
					             sizeof(renameBuffer) - 1);
					renameBuffer[sizeof(renameBuffer) - 1] = '\0';
				}
				if (ImGui::MenuItem("Delete Folder"))
					pathToDelete = file.path;

				ImGui::Separator();

				if (ImGui::BeginMenu("Add..")) {
					if (ImGui::MenuItem("Folder"))
						createNewFolder("NewFolder");
					if (ImGui::MenuItem("Script"))
						copyTemplateItem("scripts", "TemplateScript.lua",
						                 "NewScript", ".lua");
					if (ImGui::BeginMenu("Model")) {
						if (ImGui::MenuItem("Add Cube"))
							copyTemplateItem("models", "cube.obj", "Cube", ".obj");
						if (ImGui::MenuItem("Add Sphere"))
							copyTemplateItem("models", "sphere.obj", "Sphere", ".obj");
						ImGui::EndMenu();
					}
					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}

			if (node_open) {
				for (auto& child : file.children)
					DrawVirtualNodes(child);
				ImGui::TreePop();
			}

		} else {
			ImGui::Selectable((file.name + uid).c_str());

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
				std::string payloadPath = file.path.string();
				if (file.type == fileType::Model) {
					ImGui::SetDragDropPayload("MODEL_FILE",
					    payloadPath.c_str(), payloadPath.size() + 1);
				}
				ImGui::SetDragDropPayload("EXPLORER_FILE",
				    payloadPath.c_str(), payloadPath.size() + 1);
				ImGui::Text("Moving  %s", file.name.c_str());
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginPopupContextItem()) {
				if (ImGui::MenuItem("Rename")) {
					renamingNode = &file;
					std::string stemOnly = file.path.stem().string();
					std::strncpy(renameBuffer, stemOnly.c_str(),
					             sizeof(renameBuffer) - 1);
					renameBuffer[sizeof(renameBuffer) - 1] = '\0';
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Delete"))
					pathToDelete = file.path;
				ImGui::EndPopup();
			}
		}
	}


	void Explorer::syncFiles(const std::filesystem::path& path, virtualFile& node)
	{
		node.children.clear();
		node.path = path;

		for (const auto& entry : std::filesystem::directory_iterator(path)) {
			virtualFile child;
			child.name = entry.path().filename().string();
			child.path = entry.path();

			if (entry.is_directory()) {
				child.type = fileType::Folder;
				syncFiles(entry.path(), child);
			} else {
				std::string ext = entry.path().extension().string();
				if      (ext == ".lua")                  child.type = fileType::Script;
				else if (ext == ".txt")                  child.type = fileType::Text;
				else if (ext == ".obj" || ext == ".fbx") child.type = fileType::Model;
			else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") child.type = fileType::Texture;
				else                                     child.type = fileType::Text;
			}
			node.children.push_back(child);
		}
	}

	void Explorer::copyTemplateItem(const std::string& folderType,
	                                const std::string& templateName,
	                                const std::string& targetBaseName,
	                                const std::string& ext)
	{
		std::filesystem::path searchPath = std::filesystem::current_path();
		std::filesystem::path absoluteTemplatePath;
		bool found = false;

		for (int i = 0; i < 5; ++i) {
			auto p = searchPath / "templates" / "File_Templates" /
			         folderType / templateName;
			if (std::filesystem::exists(p)) {
				absoluteTemplatePath = p;
				found = true;
				break;
			}
			if (searchPath.has_parent_path()) searchPath = searchPath.parent_path();
			else break;
		}

		if (!found) {
			std::cerr << "ERROR: Could not find template: " << templateName << "\n";
			return;
		}

		std::filesystem::path targetPath =
		    resolveUniqueName(activeFolderPath, targetBaseName, ext);

		try {
			std::filesystem::copy_file(absoluteTemplatePath, targetPath,
			    std::filesystem::copy_options::overwrite_existing);
			refreshRequested = true;
			refreshPath      = activeFolderPath;
		} catch (const std::filesystem::filesystem_error& e) {
			std::cerr << "File Delivery Failed: " << e.what() << "\n";
		}
	}

	void Explorer::createNewFolder(const std::string& name) {
		std::filesystem::path targetPath =
		    resolveUniqueName(activeFolderPath, name, "");

		try {
			std::filesystem::create_directories(targetPath);
			refreshRequested = true;
			refreshPath      = activeFolderPath;
		} catch (const std::filesystem::filesystem_error& e) {
			std::cerr << "Folder Creation Failed: " << e.what() << "\n";
		}
	}
}