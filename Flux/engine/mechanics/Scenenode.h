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

#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>
#include <memory>
#include "imgui.h"
#include "imgui.h"
#include "ImGuizmo.h"

namespace Flux {

    class Model;

    enum class NodeType {
        Mesh,
        DirectionalLight,
        PointLight,
        SpotLight,
        SurfaceLight
    };

    struct LightData {
        glm::vec3 color     = glm::vec3(1.0f);
        float     intensity = 1.0f;

        float range         = 10.0f;
        float innerCutoff   = 12.5f;
        float outerCutoff   = 17.5f;

        glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);

        float areaWidth     = 1.0f;
        float areaHeight    = 1.0f;
    };

    struct SceneNode {
        std::string              name;
        NodeType                 type        = NodeType::Mesh;
        std::shared_ptr<Model>   model;
        std::string              texturePath;
        unsigned int             textureID   = 0;

        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        glm::vec3 scale    = glm::vec3(1.0f);

        int              parentIndex = -1;
        std::vector<int> children;

        LightData light;

        glm::mat4 GetTransformMatrix() const {
            float m[16];
            float t[3] = { position.x, position.y, position.z };
            float r[3] = { rotation.x, rotation.y, rotation.z };
            float s[3] = { scale.x,    scale.y,    scale.z    };
            ImGuizmo::RecomposeMatrixFromComponents(t, r, s, m);
            return glm::make_mat4(m);
        }
    };

}