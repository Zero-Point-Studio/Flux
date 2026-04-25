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
#include "Textureloader.h"
#include <iostream>
#include <filesystem>

namespace Flux {

void Model::Load() {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate | aiProcess_GenNormals |
        aiProcess_FlipUVs     | aiProcess_CalcTangentSpace);

    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        std::cerr << "ASSIMP: Could not read " << path << "\n";
        return;
    }

    std::filesystem::path modelDir = std::filesystem::path(path).parent_path();

    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* aMesh = scene->mMeshes[i];

        std::vector<Vertex>       verts;
        std::vector<unsigned int> indices;

        for (unsigned int j = 0; j < aMesh->mNumVertices; j++) {
            Vertex v;
            v.Position  = { aMesh->mVertices[j].x, aMesh->mVertices[j].y, aMesh->mVertices[j].z };
            v.Normal    = aMesh->HasNormals()
                        ? glm::vec3(aMesh->mNormals[j].x, aMesh->mNormals[j].y, aMesh->mNormals[j].z)
                        : glm::vec3(0.f, 1.f, 0.f);
            v.TexCoords = aMesh->mTextureCoords[0]
                        ? glm::vec2(aMesh->mTextureCoords[0][j].x, aMesh->mTextureCoords[0][j].y)
                        : glm::vec2(0.f);
            verts.push_back(v);
        }

        for (unsigned int j = 0; j < aMesh->mNumFaces; j++) {
            const aiFace& f = aMesh->mFaces[j];
            for (unsigned int k = 0; k < f.mNumIndices; k++)
                indices.push_back(f.mIndices[k]);
        }

        Mesh myMesh;
        myMesh.indexCount = (unsigned int)indices.size();

        if (aMesh->mMaterialIndex < scene->mNumMaterials) {
            aiMaterial* mat = scene->mMaterials[aMesh->mMaterialIndex];
            aiString texPath;
            if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                std::filesystem::path fullTex = modelDir / texPath.C_Str();
                myMesh.textureID = TextureLoader::Load(fullTex.string());
            }
            if (myMesh.textureID == 0) {
                aiColor3D col(0.8f, 0.4f, 0.1f);
                mat->Get(AI_MATKEY_COLOR_DIFFUSE, col);
                myMesh.matColor    = glm::vec3(col.r, col.g, col.b);
                myMesh.hasMtlColor = true;
            }
        }

        glGenVertexArrays(1, &myMesh.VAO);
        glGenBuffers(1, &myMesh.VBO);
        glGenBuffers(1, &myMesh.EBO);
        glBindVertexArray(myMesh.VAO);

        glBindBuffer(GL_ARRAY_BUFFER, myMesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, myMesh.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        glBindVertexArray(0);
        meshes.push_back(myMesh);
    }
}

void Model::Draw(float alphaOverride) {
    for (auto& mesh : meshes) {
        if (mesh.textureID != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.textureID);
        }
        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

void Model::SetTexture(unsigned int texID) {
    for (auto& m : meshes) m.textureID = texID;
}

}