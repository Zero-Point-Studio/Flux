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

#include "Model.h"
#include <iostream>

namespace Flux {
    void Model::Load() {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "ASSIMP CHEF ERROR: Could not read from " << path << std::endl;
            return;
        }

        for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
            aiMesh* aMesh = scene->mMeshes[i];

            std::vector<Vertex> vertices;
            std::vector<unsigned int> indices;

            for (unsigned int j = 0; j < aMesh->mNumVertices; j++) {
                Vertex vertex;
                vertex.Position.x = aMesh->mVertices[j].x;
                vertex.Position.y = aMesh->mVertices[j].y;
                vertex.Position.z = aMesh->mVertices[j].z;

                if (aMesh->HasNormals()) {
                    vertex.Normal.x = aMesh->mNormals[j].x;
                    vertex.Normal.y = aMesh->mNormals[j].y;
                    vertex.Normal.z = aMesh->mNormals[j].z;
                }
                vertices.push_back(vertex);
            }

            for (unsigned int j = 0; j < aMesh->mNumFaces; j++) {
                aiFace face = aMesh->mFaces[j];
                for (unsigned int k = 0; k < face.mNumIndices; k++) {
                    indices.push_back(face.mIndices[k]);
                }
            }

            Mesh myMesh;
            myMesh.indexCount = indices.size();

            glGenVertexArrays(1, &myMesh.VAO);
            glGenBuffers(1, &myMesh.VBO);
            glGenBuffers(1, &myMesh.EBO);

            glBindVertexArray(myMesh.VAO);

            glBindBuffer(GL_ARRAY_BUFFER, myMesh.VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, myMesh.EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

            glBindVertexArray(0);

            meshes.push_back(myMesh);
        }
    }

    void Model::Draw() {
        for (unsigned int i = 0; i < meshes.size(); i++) {
            glBindVertexArray(meshes[i].VAO);

            glDrawElements(GL_TRIANGLES, meshes[i].indexCount, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);
    }
}