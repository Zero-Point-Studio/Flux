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
#include <glm/gtx/string_cast.hpp>

namespace Flux {

extern int  currentTool;
extern bool showSettings;

void Viewport::Init() {
    glManager = std::make_unique<OpenGLManager>();
    renderer  = std::make_unique<Renderer3D>();
    camera    = std::make_unique<Camera>(glm::vec3(0.0f, 5.0f, 15.0f));
    glManager->Init(1280, 720);
    renderer->Init();
}

bool Viewport::CheckSphereHit(glm::vec3 ro, glm::vec3 rd, glm::vec3 center, float radius) {
    glm::vec3 oc = ro - center;
    float b = glm::dot(oc, rd);
    float c = glm::dot(oc, oc) - radius * radius;
    return (b * b - c) > 0.0f;
}

glm::vec3 Viewport::RaycastToGroundPlane(ImVec2 mousePos, ImVec2 imagePos, ImVec2 size,
                                          glm::mat4 proj, glm::mat4 view)
{
    float ndcX = ((mousePos.x - imagePos.x) / size.x) * 2.f - 1.f;
    float ndcY = 1.f - ((mousePos.y - imagePos.y) / size.y) * 2.f;

    glm::vec4 rayClip(ndcX, ndcY, -1.f, 1.f);
    glm::vec4 rayEye = glm::inverse(proj) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.f, 0.f);
    glm::vec3 rayDir = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
    glm::vec3 ro = camera->Position;

    float denom = rayDir.y;
    if (std::abs(denom) < 1e-6f) return ro + rayDir * 10.f;

    float t = -ro.y / denom;
    if (t < 0.f) t = 0.f;
    return ro + rayDir * t;
}

void Viewport::HandleObjectSelection(ImVec2 mousePos, ImVec2 sz,
                                     glm::mat4 proj, glm::mat4 view,
                                     Heiarchy& heiarchy)
{
    float ndcX = (2.f * mousePos.x) / sz.x - 1.f;
    float ndcY = 1.f - (2.f * mousePos.y) / sz.y;
    glm::vec4 rayClip(ndcX, ndcY, -1.f, 1.f);
    glm::vec4 rayEye = glm::inverse(proj) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.f, 0.f);
    glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
    glm::vec3 ro = camera->Position;

    int   best = -1;
    float bestDist = 999999.f;
    for (int i = 0; i < (int)heiarchy.nodes.size(); i++) {
        float r = glm::length(heiarchy.nodes[i].scale) * 0.75f;
        if (r < 0.3f) r = 0.3f;
        if (CheckSphereHit(ro, rayWorld, heiarchy.nodes[i].position, r)) {
            float d = glm::distance(ro, heiarchy.nodes[i].position);
            if (d < bestDist) { bestDist = d; best = i; }
        }
    }
    heiarchy.selectedIndex = best;
}

static ImVec2 WorldToScreen(glm::vec3 worldPos,
                             glm::mat4 view, glm::mat4 proj,
                             ImVec2 imagePos, ImVec2 size)
{
    glm::vec4 clip = proj * view * glm::vec4(worldPos, 1.f);
    if (std::abs(clip.w) < 1e-6f) return ImVec2(-1, -1);
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    float sx = (ndc.x * 0.5f + 0.5f) * size.x + imagePos.x;
    float sy = (1.f - (ndc.y * 0.5f + 0.5f)) * size.y + imagePos.y;
    return ImVec2(sx, sy);
}

void Viewport::DrawLightGizmos(Heiarchy& heiarchy,
                                glm::mat4 view, glm::mat4 proj,
                                ImVec2 imagePos, ImVec2 size)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();

    for (int i = 0; i < (int)heiarchy.nodes.size(); i++) {
        SceneNode& node = heiarchy.nodes[i];
        if (node.type == NodeType::Mesh) continue;

        bool selected = (heiarchy.selectedIndex == i);

        renderer->DrawBillboard(node.textureID, node.position, 0.5f, view, proj);

        ImVec2 screenPos = WorldToScreen(node.position, view, proj, imagePos, size);
        if (screenPos.x < imagePos.x || screenPos.x > imagePos.x + size.x) continue;
        if (screenPos.y < imagePos.y || screenPos.y > imagePos.y + size.y) continue;

        ImU32 dotCol   = selected ? IM_COL32(255,220,50,255) : IM_COL32(255,200,50,200);
        ImU32 arrowCol = selected ? IM_COL32(255,220,50,255) : IM_COL32(255,200,50,180);

        dl->AddCircleFilled(screenPos, selected ? 6.f : 4.f, dotCol);

        if (node.type == NodeType::DirectionalLight ||
            node.type == NodeType::SpotLight)
        {
            glm::vec3 arrowEnd = node.position + node.light.direction * 1.5f;
            ImVec2 screenEnd = WorldToScreen(arrowEnd, view, proj, imagePos, size);

            if (screenEnd.x >= imagePos.x && screenEnd.x <= imagePos.x + size.x &&
                screenEnd.y >= imagePos.y && screenEnd.y <= imagePos.y + size.y)
            {
                dl->AddLine(screenPos, screenEnd, arrowCol, 2.f);

                float dx = screenEnd.x - screenPos.x;
                float dy = screenEnd.y - screenPos.y;
                float len = std::sqrt(dx*dx + dy*dy);
                if (len > 1.f) {
                    float ux = dx / len;
                    float uy = dy / len;
                    float headLen = 10.f;
                    float headW   = 5.f;
                    ImVec2 tip  = screenEnd;
                    ImVec2 lp   = ImVec2(tip.x - ux*headLen + uy*headW,
                                         tip.y - uy*headLen - ux*headW);
                    ImVec2 rp   = ImVec2(tip.x - ux*headLen - uy*headW,
                                         tip.y - uy*headLen + ux*headW);
                    dl->AddTriangleFilled(tip, lp, rp, arrowCol);
                }
            }
        }

        if (node.type == NodeType::PointLight) {
            dl->AddCircle(screenPos, selected ? 14.f : 10.f, arrowCol, 16, 1.5f);
        }
    }
}

void Viewport::RenderViewport(Heiarchy& heiarchy)
{
    if (showSettings) {
        if (ImGui::Begin("Viewport Settings", &showSettings)) {
            ImGui::SliderFloat("Camera Sensitivity", &camera->MouseSensitivity, 0.01f, 0.5f);
            ImGui::SliderFloat("Move Speed",         &camera->MovementSpeed,    1.0f,  20.0f);
        }
        ImGui::End();
    }

    ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar);
    if (ImGui::IsWindowHovered()) ImGui::SetWindowFocus();

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

    ImVec2 size = ImGui::GetContentRegionAvail();
    if (size.x < 1.f) size.x = 1.f;
    if (size.y < 1.f) size.y = 1.f;
    float aspect = size.x / size.y;
    float dt = ImGui::GetIO().DeltaTime;

    if (ImGui::IsWindowFocused() && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        if (ImGui::IsKeyDown(ImGuiKey_W)) camera->ProcessKeyboard(FORWARD,  dt);
        if (ImGui::IsKeyDown(ImGuiKey_S)) camera->ProcessKeyboard(BACKWARD, dt);
        if (ImGui::IsKeyDown(ImGuiKey_A)) camera->ProcessKeyboard(LEFT,     dt);
        if (ImGui::IsKeyDown(ImGuiKey_D)) camera->ProcessKeyboard(RIGHT,    dt);
        ImVec2 md = ImGui::GetIO().MouseDelta;
        camera->ProcessMouseMovement(md.x, -md.y);
    }

    glm::mat4 view = camera->GetViewMatrix();
    glm::mat4 proj = glm::perspective(glm::radians(45.f), aspect, 0.1f, 1000.f);

    glManager->Resize((int)size.x, (int)size.y);
    glManager->Bind();
    glViewport(0, 0, (int)size.x, (int)size.y);
    glClearColor(0.1f, 0.1f, 0.1f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto& node : heiarchy.nodes) {
        if (node.model != nullptr) {
            renderer->DrawScene(*node.model, node.textureID,
                                node.GetTransformMatrix(), view, proj,
                                camera->Position, heiarchy.nodes, 1.0f);
        }
    }

    if (isDraggingModel && ghostModel) {
        glm::mat4 ghostTransform = glm::translate(glm::mat4(1.f), ghostPos);
        renderer->DrawScene(*ghostModel, 0,
                            ghostTransform, view, proj,
                            camera->Position, heiarchy.nodes, 0.45f);
    }

    for (auto& node : heiarchy.nodes) {
        if (node.type != NodeType::Mesh && node.textureID != 0) {
            renderer->DrawBillboard(node.textureID, node.position, 0.5f, view, proj);
        }
    }

    glManager->Unbind();

    ImVec2 imagePos = ImGui::GetCursorScreenPos();
    ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(glManager->GetTexture())),
                 size, ImVec2(0,1), ImVec2(1,0));

    ImVec2 mousePos = ImGui::GetIO().MousePos;
    ImVec2 mouseInCanvas(mousePos.x - imagePos.x, mousePos.y - imagePos.y);

    if (ImGui::IsItemHovered() &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !ImGuizmo::IsOver() &&
        !isDraggingModel)
    {
        HandleObjectSelection(mouseInCanvas, size, proj, view, heiarchy);
    }

    bool itemHovered = ImGui::IsItemHovered();

    if (isDraggingModel && itemHovered) {
        ghostPos = RaycastToGroundPlane(mousePos, imagePos, size, proj, view);
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("EXPLORER_FILE",
                ImGuiDragDropFlags_AcceptBeforeDelivery |
                ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
        {
            std::string path(static_cast<const char*>(p->Data));
            std::string ext = std::filesystem::path(path).extension().string();

            if (ext == ".obj" || ext == ".fbx") {
                if (!isDraggingModel || ghostPath != path) {
                    ghostPath  = path;
                    ghostModel = std::make_shared<Model>(path);
                    isDraggingModel = true;
                }
                ghostPos = RaycastToGroundPlane(mousePos, imagePos, size, proj, view);

                if (p->IsDelivery()) {
                    SceneNode n;
                    n.type     = NodeType::Mesh;
                    n.model    = ghostModel;
                    n.name     = std::filesystem::path(path).stem().string();
                    n.position = ghostPos;
                    heiarchy.nodes.push_back(n);
                    heiarchy.selectedIndex = (int)heiarchy.nodes.size() - 1;
                    isDraggingModel = false;
                    ghostModel      = nullptr;
                    ghostPath       = "";
                }
            }

            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
                ext == ".bmp" || ext == ".tga")
            {
                if (p->IsDelivery()) {
                    int sel = heiarchy.selectedIndex;
                    if (sel >= 0 && sel < (int)heiarchy.nodes.size() &&
                        heiarchy.nodes[sel].type == NodeType::Mesh)
                    {
                        std::string texPath(static_cast<const char*>(p->Data));
                        heiarchy.nodes[sel].texturePath = texPath;
                        heiarchy.nodes[sel].textureID =
                            TextureLoader::Load(texPath);
                        if (heiarchy.nodes[sel].model)
                            heiarchy.nodes[sel].model->SetTexture(
                                heiarchy.nodes[sel].textureID);
                    }
                }
            }
        } else {
            if (isDraggingModel) {
                isDraggingModel = false;
                ghostModel      = nullptr;
                ghostPath       = "";
            }
        }
        ImGui::EndDragDropTarget();
    } else if (isDraggingModel && !ImGui::GetDragDropPayload()) {
        isDraggingModel = false;
        ghostModel      = nullptr;
        ghostPath       = "";
    }

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(imagePos.x, imagePos.y, size.x, size.y);

    int sel = heiarchy.selectedIndex;
    if (sel >= 0 && sel < (int)heiarchy.nodes.size() && currentTool != 3) {
        ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
        if (currentTool == 1) op = ImGuizmo::ROTATE;
        if (currentTool == 2) op = ImGuizmo::SCALE;

        auto& target = heiarchy.nodes[sel];
        glm::mat4 mm = target.GetTransformMatrix();

        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj),
                             op, ImGuizmo::LOCAL, glm::value_ptr(mm));

        if (ImGuizmo::IsUsing()) {
            float t[3], r[3], s[3];
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(mm), t, r, s);
            target.position = { t[0], t[1], t[2] };
            target.rotation = { r[0], r[1], r[2] };
            target.scale    = { s[0], s[1], s[2] };

            if (target.type != NodeType::Mesh) {
                glm::mat4 rotMat = glm::eulerAngleYXZ(
                    glm::radians(r[1]), glm::radians(r[0]), glm::radians(r[2]));
                glm::vec4 newDir = rotMat * glm::vec4(0.f, -1.f, 0.f, 0.f);
                target.light.direction = glm::normalize(glm::vec3(newDir));
            }
        }
    }

    DrawLightGizmos(heiarchy, view, proj, imagePos, size);

    if (ImGui::IsWindowFocused() &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
        ImGui::GetIO().MouseDragMaxDistanceSqr[ImGuiMouseButton_Right] < 10.f)
    {
        ImGui::OpenPopup("ViewportCtx");
    }

    if (ImGui::BeginPopup("ViewportCtx")) {
        if (ImGui::BeginMenu("Add")) {
            if (ImGui::BeginMenu("Mesh")) {
                if (ImGui::MenuItem("Cube"))   heiarchy.AddModel("assets/models/cube.obj",   "Cube");
                if (ImGui::MenuItem("Sphere")) heiarchy.AddModel("assets/models/sphere.obj", "Sphere");
                if (ImGui::MenuItem("Monkey")) heiarchy.AddModel("assets/models/monkey.obj", "Monkey");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Light")) {
                if (ImGui::MenuItem("Directional Light")) heiarchy.AddLight(NodeType::DirectionalLight);
                if (ImGui::MenuItem("Point Light"))       heiarchy.AddLight(NodeType::PointLight);
                if (ImGui::MenuItem("Spot Light"))        heiarchy.AddLight(NodeType::SpotLight);
                if (ImGui::MenuItem("Surface Light"))     heiarchy.AddLight(NodeType::SurfaceLight);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

}