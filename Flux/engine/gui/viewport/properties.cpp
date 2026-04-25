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

#include "properties.h"
#include "heiarchy.h"
#include "Textureloader.h"
#include "Model.h"
#include <cstring>
#include <filesystem>

namespace Flux {

static bool DragVec3Row(const char* label, glm::vec3& v, float speed = 0.1f) {
    float arr[3] = { v.x, v.y, v.z };
    ImGui::PushID(label);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    bool changed = ImGui::DragFloat3("##v", arr, speed, -FLT_MAX, FLT_MAX, "%.3f");
    if (changed) v = { arr[0], arr[1], arr[2] };
    ImGui::PopID();
    return changed;
}

static bool ColorRow(const char* label, glm::vec3& c) {
    float col[3] = { c.r, c.g, c.b };
    ImGui::PushID(label);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    bool changed = ImGui::ColorEdit3("##c", col);
    if (changed) c = { col[0], col[1], col[2] };
    ImGui::PopID();
    return changed;
}

static bool FloatRow(const char* label, float& f, float spd = 0.01f, float mn = 0.f, float mx = FLT_MAX) {
    ImGui::PushID(label);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1);
    bool changed = ImGui::DragFloat("##f", &f, spd, mn, mx, "%.3f");
    ImGui::PopID();
    return changed;
}

static void BeginTable2Col() {
    ImGui::BeginTable("##t", 2,
        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings);
    ImGui::TableSetupColumn("L", ImGuiTableColumnFlags_WidthFixed, 90.f);
    ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_WidthStretch);
}

void Properties::renderProperties(Heiarchy* h) {
    ImGui::Begin("Properties");
    if (ImGui::IsWindowHovered()) ImGui::SetWindowFocus();

    if (!h || h->selectedIndex < 0 || h->selectedIndex >= (int)h->nodes.size()) {
        ImGui::TextDisabled("No object selected.");
        ImGui::End();
        return;
    }

    SceneNode& node = h->nodes[h->selectedIndex];

    char nameBuf[128];
    std::strncpy(nameBuf, node.name.c_str(), sizeof(nameBuf) - 1);
    nameBuf[sizeof(nameBuf) - 1] = '\0';
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##name", nameBuf, sizeof(nameBuf)))
        node.name = nameBuf;

    ImGui::Separator();
    ImGui::Text("Transform");
    ImGui::Spacing();

    BeginTable2Col();
    DragVec3Row("Position", node.position, 0.1f);
    DragVec3Row("Rotation", node.rotation, 0.5f);
    DragVec3Row("Scale",    node.scale,    0.01f);
    ImGui::EndTable();

    if (node.type == NodeType::Mesh) {
        ImGui::Separator();
        ImGui::Text("Mesh");
        ImGui::Spacing();

        if (node.model)
            ImGui::TextDisabled("%s", node.model->path.c_str());
        else
            ImGui::TextDisabled("(no model)");

        ImGui::Spacing();
        ImGui::Text("Texture:");
        ImGui::SameLine();
        if (node.texturePath.empty())
            ImGui::TextDisabled("none  (drop image here)");
        else
            ImGui::TextDisabled("%s",
                std::filesystem::path(node.texturePath).filename().string().c_str());

        float dropW = ImGui::GetContentRegionAvail().x;
        ImGui::InvisibleButton("##texDrop", ImVec2(dropW, 28));

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("EXPLORER_FILE")) {
                std::string droppedPath(static_cast<const char*>(p->Data));
                std::string ext = std::filesystem::path(droppedPath).extension().string();
                if (ext==".png"||ext==".jpg"||ext==".jpeg"||ext==".bmp"||ext==".tga") {
                    node.texturePath = droppedPath;
                    node.textureID   = TextureLoader::Load(droppedPath);
                    if (node.model) node.model->SetTexture(node.textureID);
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (!node.texturePath.empty()) {
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear")) {
                node.texturePath = "";
                node.textureID   = 0;
                if (node.model) node.model->SetTexture(0);
            }
        }

    } else {
        ImGui::Separator();
        const char* lightLabel =
            node.type == NodeType::DirectionalLight ? "Directional Light" :
            node.type == NodeType::PointLight       ? "Point Light"       :
            node.type == NodeType::SpotLight        ? "Spot Light"        : "Surface Light";
        ImGui::Text("%s", lightLabel);
        ImGui::Spacing();

        BeginTable2Col();
        ColorRow("Color",     node.light.color);
        FloatRow("Intensity", node.light.intensity, 0.01f, 0.f, 100.f);

        if (node.type == NodeType::DirectionalLight)
            DragVec3Row("Direction", node.light.direction, 0.01f);

        if (node.type == NodeType::PointLight)
            FloatRow("Range", node.light.range, 0.1f, 0.f, 1000.f);

        if (node.type == NodeType::SpotLight) {
            DragVec3Row("Direction",  node.light.direction,   0.01f);
            FloatRow("Range",         node.light.range,       0.1f, 0.f, 1000.f);
            FloatRow("Inner Cutoff",  node.light.innerCutoff, 0.1f, 0.f, 90.f);
            FloatRow("Outer Cutoff",  node.light.outerCutoff, 0.1f, 0.f, 90.f);
        }

        if (node.type == NodeType::SurfaceLight) {
            FloatRow("Area Width",  node.light.areaWidth,  0.01f, 0.f, 100.f);
            FloatRow("Area Height", node.light.areaHeight, 0.01f, 0.f, 100.f);
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

}