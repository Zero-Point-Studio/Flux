#include "heiarchy.h"
#include "Model.h"
#include "Textureloader.h"
#include <cstring>
#include <iostream>
#include <filesystem>
#include "../../utils/PathHelper.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

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

void Heiarchy::setup() {
    AddLight(NodeType::DirectionalLight, "Sun");
    SceneNode& sunNode = nodes.back();
    sunNode.position = glm::vec3(0.0f, 10.0f, 0.0f);

    float pitch = -75.0f, yaw = 20.0f, roll = 0.0f;
    sunNode.rotation = glm::vec3(pitch, yaw, roll);

    glm::quat q = glm::angleAxis(glm::radians(yaw),   glm::vec3(0,1,0))
                * glm::angleAxis(glm::radians(pitch),  glm::vec3(1,0,0))
                * glm::angleAxis(glm::radians(roll),   glm::vec3(0,0,1));
    sunNode.light.direction = glm::normalize(q * glm::vec3(0.f, -1.f, 0.f));

    AddModel(PathHelper::GetAssetPath(std::string("assets/models/cube.obj")));
}

std::string Heiarchy::GetUniqueName(const std::string& baseName) {
    std::string newName = baseName;
    int counter = 1;
    bool nameExists = true;
    while (nameExists) {
        nameExists = false;
        for (const auto& node : nodes) {
            if (node.name == newName) {
                nameExists = true;
                newName = baseName + " (" + std::to_string(counter) + ")";
                counter++;
                break;
            }
        }
    }
    return newName;
}

void Heiarchy::AddModel(const std::string& path, const std::string& name) {
    SceneNode n;
    n.type  = NodeType::Mesh;
    n.model = GetOrLoadModel(path);
    std::string desiredName = name.empty() ? std::filesystem::path(path).stem().string() : name;
    n.name  = GetUniqueName(desiredName);
    nodes.push_back(n);
    selectedIndex = (int)nodes.size() - 1;
}

void Heiarchy::AddLight(NodeType type, const std::string& name) {
    SceneNode n;
    n.type = type;

    auto tryLoadIcon = [&](const char* iconRelPath) -> unsigned int {
        std::string absolutePath = PathHelper::GetAssetPath(iconRelPath);
        if (std::filesystem::exists(absolutePath))
            return TextureLoader::Load(absolutePath);
        else
            std::cerr << "Warning: Could not find icon at " << absolutePath << "\n";
        return 0;
    };

    std::string baseName = name;
    switch (type) {
        case NodeType::DirectionalLight:
            n.name      = name.empty() ? "Directional Light" : name;
            n.textureID = tryLoadIcon("assets/icons/l_dir.png");
            break;
        case NodeType::PointLight:
            n.name      = name.empty() ? "Point Light" : name;
            n.textureID = tryLoadIcon("assets/icons/l_point.png");
            break;
        case NodeType::SpotLight:
            n.name      = name.empty() ? "Spot Light" : name;
            n.textureID = tryLoadIcon("assets/icons/l_spot.png");
            break;
        default:
            n.name = GetUniqueName(baseName);
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
        ImGui::Separator();
        if (ImGui::MenuItem("Duplicate")) {
            SceneNode copyNode = nodes[index];

            copyNode.name = GetUniqueName(copyNode.name);

            nodes.push_back(copyNode);

            selectedIndex = (int)nodes.size() - 1;
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
                    std::string full = PathHelper::GetAssetPath(std::string("assets/models/") + rel);

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