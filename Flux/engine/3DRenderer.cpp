#include "3DRenderer.h"
#include <glad/glad.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "utils/PathHelper.h"

namespace Flux
{
    std::string LoadShaderFromFile(const std::string& filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            std::cerr << "Driver Error: Could not open shader file " << filepath << "\n";
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    static unsigned int compileShader(GLenum type, const char* src)
    {
        unsigned int s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        int ok;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            char log[512];
            glGetShaderInfoLog(s, 512, nullptr, log);
            std::cerr << "Shader error: " << log << "\n";
        }
        return s;
    }

    static unsigned int linkProgram(const char* vs, const char* fs)
    {
        unsigned int v = compileShader(GL_VERTEX_SHADER,   vs);
        unsigned int f = compileShader(GL_FRAGMENT_SHADER, fs);
        unsigned int p = glCreateProgram();
        glAttachShader(p, v);
        glAttachShader(p, f);
        glLinkProgram(p);
        int ok;
        glGetProgramiv(p, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetProgramInfoLog(p, 512, nullptr, log);
            std::cerr << "Program link error: " << log << "\n";
        }
        glDeleteShader(v);
        glDeleteShader(f);
        return p;
    }

    static void set1i(unsigned int p, const char* n, int v)
    {
        glUniform1i(glGetUniformLocation(p, n), v);
    }

    static void set1f(unsigned int p, const char* n, float v)
    {
        glUniform1f(glGetUniformLocation(p, n), v);
    }

    static void set3f(unsigned int p, const char* n, glm::vec3 v)
    {
        glUniform3fv(glGetUniformLocation(p, n), 1, glm::value_ptr(v));
    }

    static void setMat4(unsigned int p, const char* n, glm::mat4 m)
    {
        glUniformMatrix4fv(glGetUniformLocation(p, n), 1, GL_FALSE, glm::value_ptr(m));
    }

    void Renderer3D::Init()
    {
        std::string vShaderCode = LoadShaderFromFile(PathHelper::GetAssetPath("shaders/vertSrc.glsl"));
        std::string fShaderCode = LoadShaderFromFile(PathHelper::GetAssetPath("shaders/fragSrc.glsl"));
        shaderProgram = linkProgram(vShaderCode.c_str(), fShaderCode.c_str());
        InitBillboard();
    }
    void Renderer3D::InitShadowMap(int resolution)
    {
        shadowResolution = resolution;

        glGenFramebuffers(1, &shadowFBO);

        glGenTextures(1, &shadowDepthTex);
        glBindTexture(GL_TEXTURE_2D, shadowDepthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                     resolution, resolution, 0,
                     GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderCol[] = { 1.f, 1.f, 1.f, 1.f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderCol);

        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, shadowDepthTex, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "ERROR: Shadow map FBO incomplete!\n";

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
 
        const char* depthVS = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 lightSpaceMatrix;
uniform mat4 model;
void main() {
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}
)";
        const char* depthFS = R"(
#version 330 core
void main() { /* depth written automatically */ }
)";
        depthProgram = linkProgram(depthVS, depthFS);

        shadowReady = true;
    }

    void Renderer3D::DrawDepthPass(const std::vector<SceneNode>& nodes,
                                   glm::vec3 lightDir)
    {
        if (!shadowReady) return;
        glm::vec3 lightPos = -glm::normalize(lightDir) * 30.f;
        glm::mat4 lightView = glm::lookAt(lightPos,
                                          glm::vec3(0.f),
                                          glm::vec3(0.f, 1.f, 0.f));
        glm::mat4 lightProj = glm::ortho(-20.f, 20.f, -20.f, 20.f, 0.1f, 100.f);
        lightSpaceMatrix = lightProj * lightView;

        glViewport(0, 0, shadowResolution, shadowResolution);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(depthProgram);
        setMat4(depthProgram, "lightSpaceMatrix", lightSpaceMatrix);

        glCullFace(GL_FRONT);
        glEnable(GL_DEPTH_TEST);

        for (const auto& node : nodes)
        {
            if (node.type != NodeType::Mesh || !node.model) continue;
            setMat4(depthProgram, "model", node.GetTransformMatrix());
            for (auto& mesh : node.model->meshes)
            {
                glBindVertexArray(mesh.VAO);
                glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
        }

        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Renderer3D::InitGrid()
    {
        std::string vgridShaderCode = LoadShaderFromFile(PathHelper::GetAssetPath("shaders/gridVertSrc.glsl"));
        std::string fgridShaderCode = LoadShaderFromFile(PathHelper::GetAssetPath("shaders/gridFragSrc.glsl"));
        gridProgram = linkProgram(vgridShaderCode.c_str(), fgridShaderCode.c_str());

        float vertices[] = {
            1, 1, 0, -1, -1, 0, -1, 1, 0,
            -1, -1, 0, 1, 1, 0, 1, -1, 0
        };
        glGenVertexArrays(1, &gridVAO);
        unsigned int vbo;
        glGenBuffers(1, &vbo);
        glBindVertexArray(gridVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    void Renderer3D::DrawGrid(glm::mat4 view, glm::mat4 proj, glm::vec3 cameraPos)
    {
        glUseProgram(gridProgram);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        setMat4(gridProgram, "view", view);
        setMat4(gridProgram, "projection", proj);
        glBindVertexArray(gridVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }


    void Renderer3D::InitBillboard()
    {
        std::string bbvShaderCode = LoadShaderFromFile(PathHelper::GetAssetPath("shaders/bbVertSrc.glsl"));
        std::string bbfShaderCode = LoadShaderFromFile(PathHelper::GetAssetPath("shaders/bbFragSrc.glsl"));
        billboardProgram = linkProgram(bbvShaderCode.c_str(), bbfShaderCode.c_str());

        float quad[] = {
            -0.5f,  0.5f, 0.f, 1.f,
            -0.5f, -0.5f, 0.f, 0.f,
             0.5f, -0.5f, 1.f, 0.f,
            -0.5f,  0.5f, 0.f, 1.f,
             0.5f, -0.5f, 1.f, 0.f,
             0.5f,  0.5f, 1.f, 1.f,
        };
        glGenVertexArrays(1, &billboardVAO);
        glGenBuffers(1, &billboardVBO);
        glBindVertexArray(billboardVAO);
        glBindBuffer(GL_ARRAY_BUFFER, billboardVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);
    }


    void Renderer3D::DrawScene(Model& model, unsigned int overrideTexID,
                               glm::mat4 modelMatrix, glm::mat4 view, glm::mat4 proj,
                               glm::vec3 cameraPos,
                               const std::vector<SceneNode>& lights,
                               float alpha)
    {
        glUseProgram(shaderProgram);
        glEnable(GL_DEPTH_TEST);

        if (alpha < 1.0f)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
        }
        else
        {
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
        }

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        setMat4(shaderProgram, "model",      modelMatrix);
        setMat4(shaderProgram, "view",       view);
        setMat4(shaderProgram, "projection", proj);
        set3f (shaderProgram, "viewPos",    cameraPos);
        set1f (shaderProgram, "alpha",      alpha);

        if (shadowReady)
        {
            setMat4(shaderProgram, "lightSpaceMatrix", lightSpaceMatrix);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, shadowDepthTex);
            set1i(shaderProgram, "shadowMap",    1);
            set1i(shaderProgram, "hasShadowMap", 1);
        }
        else
        {
            set1i(shaderProgram, "hasShadowMap", 0);
        }

        bool hasDirLight = false;
        int  numPoint    = 0;
        int  numSpot     = 0;
        bool hasSurface  = false;
        glm::vec3 surfCol(1.f);
        float     surfInt = 0.f;

        for (const auto& node : lights)
        {
            if (node.type == NodeType::DirectionalLight && !hasDirLight)
            {
                hasDirLight = true;
                set3f(shaderProgram, "dirLightDir",       node.light.direction);
                set3f(shaderProgram, "dirLightColor",     node.light.color);
                set1f(shaderProgram, "dirLightIntensity", node.light.intensity);
            }
            else if (node.type == NodeType::PointLight && numPoint < 8)
            {
                std::string i = std::to_string(numPoint);
                set3f(shaderProgram, ("pointPos["       + i + "]").c_str(), node.position);
                set3f(shaderProgram, ("pointColor["     + i + "]").c_str(), node.light.color);
                set1f(shaderProgram, ("pointIntensity[" + i + "]").c_str(), node.light.intensity);
                set1f(shaderProgram, ("pointRange["     + i + "]").c_str(), node.light.range);
                numPoint++;
            }
            else if (node.type == NodeType::SpotLight && numSpot < 4)
            {
                std::string i = std::to_string(numSpot);
                set3f(shaderProgram, ("spotPos["       + i + "]").c_str(), node.position);
                set3f(shaderProgram, ("spotDir["       + i + "]").c_str(), node.light.direction);
                set3f(shaderProgram, ("spotColor["     + i + "]").c_str(), node.light.color);
                set1f(shaderProgram, ("spotIntensity[" + i + "]").c_str(), node.light.intensity);
                set1f(shaderProgram, ("spotRange["     + i + "]").c_str(), node.light.range);
                set1f(shaderProgram, ("spotInner["     + i + "]").c_str(), node.light.innerCutoff);
                set1f(shaderProgram, ("spotOuter["     + i + "]").c_str(), node.light.outerCutoff);
                numSpot++;
            }
            else if (node.type == NodeType::SurfaceLight && !hasSurface)
            {
                hasSurface = true;
                surfCol    = node.light.color;
                surfInt    = node.light.intensity;
            }
        }

        set1i(shaderProgram, "hasDirLight",      hasDirLight ? 1 : 0);
        set1i(shaderProgram, "numPointLights",   numPoint);
        set1i(shaderProgram, "numSpotLights",    numSpot);
        set1i(shaderProgram, "hasSurface",       hasSurface ? 1 : 0);
        set3f(shaderProgram, "surfaceColor",     surfCol);
        set1f(shaderProgram, "surfaceIntensity", surfInt);

        for (auto& mesh : model.meshes)
        {
            unsigned int texID   = (overrideTexID != 0) ? overrideTexID : mesh.textureID;
            bool         useTex  = (texID != 0);
            set1i(shaderProgram, "hasTexture", useTex ? 1 : 0);
            if (useTex)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texID);
                set1i(shaderProgram, "albedo", 0);
            }
            set3f(shaderProgram, "matColor",
                  mesh.hasMtlColor ? mesh.matColor : glm::vec3(0.8f, 0.4f, 0.1f));

            glBindVertexArray(mesh.VAO);
            glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        if (alpha < 1.0f)
        {
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }
    }


    void Renderer3D::DrawBillboard(unsigned int texID,
                                   glm::vec3 worldPos, float size,
                                   glm::mat4 view, glm::mat4 proj)
    {
        if (texID == 0) return;

        glUseProgram(billboardProgram);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);

        glUniform3fv(glGetUniformLocation(billboardProgram, "worldPos"), 1, glm::value_ptr(worldPos));
        glUniform1f(glGetUniformLocation(billboardProgram, "size"), size);
        setMat4(billboardProgram, "view",       view);
        setMat4(billboardProgram, "projection", proj);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);
        set1i(billboardProgram, "icon", 0);

        glBindVertexArray(billboardVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
    }


    void Renderer3D::InitSkybox()
    {
        std::string vCode = LoadShaderFromFile(PathHelper::GetAssetPath("shaders/skyboxVertShader.glsl"));
        std::string fCode = LoadShaderFromFile(PathHelper::GetAssetPath("shaders/skyboxFragShader.glsl"));
        skyboxProgram = linkProgram(vCode.c_str(), fCode.c_str());

        float skyboxVertices[] = {
            -1.f,  1.f, -1.f,  -1.f, -1.f, -1.f,   1.f, -1.f, -1.f,
             1.f, -1.f, -1.f,   1.f,  1.f, -1.f,  -1.f,  1.f, -1.f,
            -1.f, -1.f,  1.f,  -1.f, -1.f, -1.f,  -1.f,  1.f, -1.f,
            -1.f,  1.f, -1.f,  -1.f,  1.f,  1.f,  -1.f, -1.f,  1.f,
             1.f, -1.f, -1.f,   1.f, -1.f,  1.f,   1.f,  1.f,  1.f,
             1.f,  1.f,  1.f,   1.f,  1.f, -1.f,   1.f, -1.f, -1.f,
            -1.f, -1.f,  1.f,  -1.f,  1.f,  1.f,   1.f,  1.f,  1.f,
             1.f,  1.f,  1.f,   1.f, -1.f,  1.f,  -1.f, -1.f,  1.f,
            -1.f,  1.f, -1.f,   1.f,  1.f, -1.f,   1.f,  1.f,  1.f,
             1.f,  1.f,  1.f,  -1.f,  1.f,  1.f,  -1.f,  1.f, -1.f,
            -1.f, -1.f, -1.f,  -1.f, -1.f,  1.f,   1.f, -1.f, -1.f,
             1.f, -1.f, -1.f,  -1.f, -1.f,  1.f,   1.f, -1.f,  1.f
        };

        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    void Renderer3D::DrawSkybox(glm::mat4 view, glm::mat4 proj, glm::vec3 sunDir)
    {
        glDepthFunc(GL_LEQUAL);
        glUseProgram(skyboxProgram);

        glm::mat4 staticView = glm::mat4(glm::mat3(view));
        setMat4(skyboxProgram, "view",       staticView);
        setMat4(skyboxProgram, "projection", proj);
        set3f (skyboxProgram, "sunDir",     sunDir);

        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glDepthFunc(GL_LESS);
    }

}