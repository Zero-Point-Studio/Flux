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
#include <glad/glad.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>

namespace Flux {

    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
    };

    struct Mesh {
        unsigned int VAO        = 0;
        unsigned int VBO        = 0;
        unsigned int EBO        = 0;
        unsigned int indexCount = 0;
        unsigned int textureID  = 0;
        glm::vec3    matColor   = glm::vec3(0.8f, 0.4f, 0.1f);
        bool         hasMtlColor = false;
    };

    class Model {
    public:
        std::string       path;
        std::vector<Mesh> meshes;

        Model(const std::string& modelPath) : path(modelPath) { Load(); }

        void Load();
        void Draw(float alphaOverride = 1.0f);
        void SetTexture(unsigned int texID);
    };

}