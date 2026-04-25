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
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "Model.h"
#include "./mechanics/Scenenode.h"

namespace Flux {

    class Renderer3D {
    public:
        void Init();

        void DrawScene(Model& model, unsigned int overrideTexID,
                       glm::mat4 modelMatrix, glm::mat4 view, glm::mat4 proj,
                       glm::vec3 cameraPos,
                       const std::vector<SceneNode>& lights,
                       float alpha = 1.0f);

        void DrawBillboard(unsigned int texID,
                           glm::vec3 worldPos, float size,
                           glm::mat4 view, glm::mat4 proj);

    private:
        unsigned int shaderProgram     = 0;
        unsigned int billboardProgram  = 0;
        unsigned int billboardVAO      = 0;
        unsigned int billboardVBO      = 0;

        void InitBillboard();
    };

}