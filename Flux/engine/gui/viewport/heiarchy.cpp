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

#include "heiarchy.h"
#include "Model.h"
#include "Textureloader.h"
#include <cstring>
#include <iostream>
#include <filesystem>

namespace Flux {

static const char* NodeTypeLabel(NodeType t) {
    switch (t) {
        case NodeType::DirectionalLight: return "[Dir]  ";
        case NodeType::PointLight:       return "[Pt]   ";
        case NodeType::SpotLight:        return "[Spot] ";
        case NodeType::SurfaceLight:     return "[Surf] ";
        default:                         return "[Mesh] ";
    }
}

std::shared_ptr<Model> Heiarchy::GetOrLoadModel(const std::string& path) {
    auto it = modelRegistry.find(path);
    if (it != modelRegistry.end()) return it->second;
    auto m = std::make_shared<Model>(path);
    modelRegistry[path] = m;
    return m;
}

void Heiarchy::AddModel(const std::string& path, const std::string& name) {
    SceneNode n;
    n.type  = NodeType::Mesh;
    n.model = GetOrLoadModel(path);
    n.name  = name.empty() ? std::filesystem::path(path).stem().string() : name;
    nodes.push_back(n);
    selectedIndex = (int)nodes.size() - 1;
}

void Heiarchy::AddLight(NodeType type, const std::string& name) {
    SceneNode n;
    n.type = type;

    auto tryLoadIcon = [&](const char* iconPath) -> unsigned int {
        if (std::filesystem::exists(iconPath))
            return TextureLoader::Load(iconPath);
        return 0;
    };

    switch (type) {
        case NodeType::DirectionalLight:
            n.name      = name.empty() ? "Directional Light" : name;
            n.textureID = tryLoadIcon("assets/icons/l_directional.png");
            break;
        case NodeType::PointLight:
            n.name      = name.empty() ? "Point Light" : name;
            n.textureID = tryLoadIcon("assets/icons/l_point.png");
            break;
        case NodeType::SpotLight:
            n.name      = name.empty() ? "Spot Light" : name;
            n.textureID = tryLoadIcon("assets/icons/l_spot.png");
            break;
        case NodeType::SurfaceLight:
            n.name      = name.empty() ? "Surface Light" : name;
            n.textureID = tryLoadIcon("assets/icons/l_surface.png");
            break;
        default:
            n.name = name;
    }

    nodes.push_back(n);
    selectedIndex = (int)nodes.size() - 1;
}

void Heiarchy::DrawNode(int index) {
    SceneNode& node = nodes[index];
    std::string uid = "##node" + std::to_string(index);

    if (renamingIndex == index) {
        ImGui::SetNextItemWidth(-1);
        ImGui::SetKeyboardFocusHere();
        if (ImGui::InputText(("##ren" + uid).c_str(), renameBuffer, sizeof(renameBuffer),
                ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            if (renameBuffer[0] != '\0') node.name = renameBuffer;
            renamingIndex = -1;
        }
        if (ImGui::IsItemDeactivated()) renamingIndex = -1;
        return;
    }

    bool selected = (selectedIndex == index);
    std::string label = std::string(NodeTypeLabel(node.type)) + node.name + uid;

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_Leaf |
        ImGuiTreeNodeFlags_NoTreePushOnOpen |
        ImGuiTreeNodeFlags_SpanFullWidth;
    if (selected) flags |= ImGuiTreeNodeFlags_Selected;

    if (selected) {
        ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.26f, 0.59f, 0.98f, 0.35f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.26f, 0.59f, 0.98f, 0.55f));
    }
    ImGui::TreeNodeEx(label.c_str(), flags);
    if (selected) ImGui::PopStyleColor(2);

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        selectedIndex = index;

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        renamingIndex = index;
        std::strncpy(renameBuffer, node.name.c_str(), sizeof(renameBuffer) - 1);
        renameBuffer[sizeof(renameBuffer) - 1] = '\0';
    }

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        ImGui::SetDragDropPayload("HIER_NODE", &index, sizeof(int));
        ImGui::Text("Moving  %s", node.name.c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("HIER_NODE")) {
            int srcIdx = *(const int*)p->Data;
            if (srcIdx != index && srcIdx >= 0 && srcIdx < (int)nodes.size()) {
                SceneNode moved = nodes[srcIdx];
                nodes.erase(nodes.begin() + srcIdx);
                int insertAt = (srcIdx < index) ? index : index + 1;
                if (insertAt > (int)nodes.size()) insertAt = (int)nodes.size();
                nodes.insert(nodes.begin() + insertAt, moved);
                selectedIndex = insertAt;
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopupContextItem(("##ctx" + uid).c_str())) {
        if (ImGui::MenuItem("Rename")) {
            renamingIndex = index;
            std::strncpy(renameBuffer, node.name.c_str(), sizeof(renameBuffer) - 1);
            renameBuffer[sizeof(renameBuffer) - 1] = '\0';
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Delete")) {
            nodes.erase(nodes.begin() + index);
            if (selectedIndex >= (int)nodes.size())
                selectedIndex = (int)nodes.size() - 1;
            ImGui::EndPopup();
            return;
        }
        ImGui::EndPopup();
    }
}

void Heiarchy::renderHeiarchy(const std::filesystem::path& activeProjectPath) {
    ImGui::Begin("Heiarchy");
    if (ImGui::IsWindowHovered()) ImGui::SetWindowFocus();

    ImGui::Text("Scene");
    ImGui::Separator();

    for (int i = 0; i < (int)nodes.size(); i++)
        DrawNode(i);

    float emptyH = std::max(ImGui::GetContentRegionAvail().y, 8.0f);
    ImGui::InvisibleButton("##hierEmpty",
        ImVec2(ImGui::GetContentRegionAvail().x, emptyH));

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("EXPLORER_FILE")) {
            std::string path(static_cast<const char*>(p->Data));
            std::string ext = std::filesystem::path(path).extension().string();
            if (ext == ".obj" || ext == ".fbx")
                AddModel(path);
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopupContextItem("##hierEmpty")) {
        if (ImGui::BeginMenu("Add Object")) {
            if (ImGui::BeginMenu("Mesh")) {
                auto tryAdd = [&](const char* rel, const char* addName) {
                    std::string full = "assets/models/" + std::string(rel);
                    if (!activeProjectPath.empty()) {
                        std::filesystem::path candidate = activeProjectPath / "models" / rel;
                        if (std::filesystem::exists(candidate)) full = candidate.string();
                    }
                    AddModel(full, addName);
                };
                if (ImGui::MenuItem("Cube"))   tryAdd("cube.obj",   "Cube");
                if (ImGui::MenuItem("Sphere")) tryAdd("sphere.obj", "Sphere");
                if (ImGui::MenuItem("Monkey")) tryAdd("monkey.obj", "Monkey");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Light")) {
                if (ImGui::MenuItem("Directional Light")) AddLight(NodeType::DirectionalLight);
                if (ImGui::MenuItem("Point Light"))       AddLight(NodeType::PointLight);
                if (ImGui::MenuItem("Spot Light"))        AddLight(NodeType::SpotLight);
                if (ImGui::MenuItem("Surface Light"))     AddLight(NodeType::SurfaceLight);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

}