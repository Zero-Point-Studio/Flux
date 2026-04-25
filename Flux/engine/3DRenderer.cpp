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

#include "3DRenderer.h"
#include <glad/glad.h>
#include <iostream>
#include <string>

namespace Flux {

static const char* vertSrc = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoords;
out vec3 FragPos; out vec3 Normal; out vec2 TexCoords;
uniform mat4 model,view,projection;
void main(){
    FragPos=vec3(model*vec4(aPos,1.0));
    Normal=mat3(transpose(inverse(model)))*aNormal;
    TexCoords=aTexCoords;
    gl_Position=projection*view*vec4(FragPos,1.0);
}
)";

static const char* fragSrc = R"(
#version 330 core
out vec4 FragColor;
in vec3 FragPos; in vec3 Normal; in vec2 TexCoords;
uniform vec3 viewPos;
uniform bool hasTexture;
uniform sampler2D albedo;
uniform vec3 matColor;
uniform float alpha;
#define MAX_POINT 8
#define MAX_SPOT  4
uniform vec3  dirLightDir; uniform vec3 dirLightColor; uniform float dirLightIntensity; uniform bool hasDirLight;
uniform int   numPointLights;
uniform vec3  pointPos[MAX_POINT]; uniform vec3 pointColor[MAX_POINT];
uniform float pointIntensity[MAX_POINT]; uniform float pointRange[MAX_POINT];
uniform int   numSpotLights;
uniform vec3  spotPos[MAX_SPOT]; uniform vec3 spotDir[MAX_SPOT];
uniform vec3  spotColor[MAX_SPOT]; uniform float spotIntensity[MAX_SPOT];
uniform float spotRange[MAX_SPOT]; uniform float spotInner[MAX_SPOT]; uniform float spotOuter[MAX_SPOT];
uniform vec3 surfaceColor; uniform float surfaceIntensity; uniform bool hasSurface;
vec3 base(){return hasTexture?texture(albedo,TexCoords).rgb:matColor;}
vec3 calcDir(vec3 n,vec3 vd){
    if(!hasDirLight)return vec3(0);
    vec3 ld=normalize(-dirLightDir);
    float d=max(dot(n,ld),0.0);
    vec3 ref=reflect(-ld,n);
    float s=pow(max(dot(vd,ref),0.0),32.0);
    return dirLightColor*dirLightIntensity*(0.2+d+0.5*s);
}
vec3 calcPoint(int i,vec3 n,vec3 vd){
    vec3 ld=pointPos[i]-FragPos; float dist=length(ld);
    if(dist>pointRange[i])return vec3(0);
    ld=normalize(ld);
    float d=max(dot(n,ld),0.0);
    vec3 ref=reflect(-ld,n); float s=pow(max(dot(vd,ref),0.0),32.0);
    float att=1.0/(1.0+0.09*dist+0.032*dist*dist);
    return pointColor[i]*pointIntensity[i]*att*(d+0.5*s);
}
vec3 calcSpot(int i,vec3 n,vec3 vd){
    vec3 ld=spotPos[i]-FragPos; float dist=length(ld);
    if(dist>spotRange[i])return vec3(0);
    ld=normalize(ld);
    float theta=dot(ld,normalize(-spotDir[i]));
    float eps=cos(radians(spotInner[i]))-cos(radians(spotOuter[i]));
    float fact=clamp((theta-cos(radians(spotOuter[i])))/eps,0.0,1.0);
    float d=max(dot(n,ld),0.0);
    vec3 ref=reflect(-ld,n); float s=pow(max(dot(vd,ref),0.0),32.0);
    float att=1.0/(1.0+0.09*dist+0.032*dist*dist);
    return spotColor[i]*spotIntensity[i]*att*fact*(d+0.5*s);
}
void main(){
    vec3 n=normalize(Normal); vec3 vd=normalize(viewPos-FragPos);
    vec3 b=base();
    vec3 light=vec3(0.05);
    light+=calcDir(n,vd);
    for(int i=0;i<numPointLights;i++) light+=calcPoint(i,n,vd);
    for(int i=0;i<numSpotLights;i++)  light+=calcSpot(i,n,vd);
    if(hasSurface) light+=surfaceColor*surfaceIntensity;
    FragColor=vec4(b*light,alpha);
}
)";

static const char* bbVertSrc = R"(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
out vec2 UV;
uniform vec3  worldPos;
uniform float size;
uniform mat4  view;
uniform mat4  projection;
void main(){
    vec3 camRight = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 camUp    = vec3(view[0][1], view[1][1], view[2][1]);
    vec3 vPos = worldPos
        + camRight * aPos.x * size
        + camUp    * aPos.y * size;
    UV = aUV;
    gl_Position = projection * view * vec4(vPos, 1.0);
}
)";

static const char* bbFragSrc = R"(
#version 330 core
in vec2 UV;
out vec4 FragColor;
uniform sampler2D icon;
void main(){
    vec4 c = texture(icon, UV);
    if(c.a < 0.05) discard;
    FragColor = c;
}
)";

static unsigned int compileShader(GLenum type, const char* src) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "Shader error: " << log << "\n";
    }
    return s;
}

static unsigned int linkProgram(const char* vs, const char* fs) {
    unsigned int v = compileShader(GL_VERTEX_SHADER, vs);
    unsigned int f = compileShader(GL_FRAGMENT_SHADER, fs);
    unsigned int p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

static void set1i(unsigned int p, const char* n, int v)         { glUniform1i(glGetUniformLocation(p,n),v); }
static void set1f(unsigned int p, const char* n, float v)       { glUniform1f(glGetUniformLocation(p,n),v); }
static void set3f(unsigned int p, const char* n, glm::vec3 v)   { glUniform3fv(glGetUniformLocation(p,n),1,glm::value_ptr(v)); }
static void setMat4(unsigned int p, const char* n, glm::mat4 m) { glUniformMatrix4fv(glGetUniformLocation(p,n),1,GL_FALSE,glm::value_ptr(m)); }

void Renderer3D::Init() {
    shaderProgram = linkProgram(vertSrc, fragSrc);
    InitBillboard();
}

void Renderer3D::InitBillboard() {
    billboardProgram = linkProgram(bbVertSrc, bbFragSrc);

    float quad[] = {
        -0.5f,  0.5f,  0.f, 1.f,
        -0.5f, -0.5f,  0.f, 0.f,
         0.5f, -0.5f,  1.f, 0.f,
        -0.5f,  0.5f,  0.f, 1.f,
         0.5f, -0.5f,  1.f, 0.f,
         0.5f,  0.5f,  1.f, 1.f,
    };
    glGenVertexArrays(1, &billboardVAO);
    glGenBuffers(1, &billboardVBO);
    glBindVertexArray(billboardVAO);
    glBindBuffer(GL_ARRAY_BUFFER, billboardVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
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

    if (alpha < 1.0f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
    } else {
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
    }

    glEnable(GL_CULL_FACE); glCullFace(GL_BACK); glFrontFace(GL_CCW);

    setMat4(shaderProgram, "model",      modelMatrix);
    setMat4(shaderProgram, "view",       view);
    setMat4(shaderProgram, "projection", proj);
    set3f  (shaderProgram, "viewPos",    cameraPos);
    set1f  (shaderProgram, "alpha",      alpha);

    bool hasDirLight = false;
    int  numPoint = 0, numSpot = 0;
    bool hasSurface = false;
    glm::vec3 surfCol(1.f); float surfInt = 0.f;

    for (const auto& node : lights) {
        if (node.type == NodeType::DirectionalLight && !hasDirLight) {
            hasDirLight = true;
            set3f(shaderProgram, "dirLightDir",       node.light.direction);
            set3f(shaderProgram, "dirLightColor",     node.light.color);
            set1f(shaderProgram, "dirLightIntensity", node.light.intensity);
        } else if (node.type == NodeType::PointLight && numPoint < 8) {
            std::string i = std::to_string(numPoint);
            set3f(shaderProgram, ("pointPos["      + i + "]").c_str(), node.position);
            set3f(shaderProgram, ("pointColor["    + i + "]").c_str(), node.light.color);
            set1f(shaderProgram, ("pointIntensity["+ i + "]").c_str(), node.light.intensity);
            set1f(shaderProgram, ("pointRange["    + i + "]").c_str(), node.light.range);
            numPoint++;
        } else if (node.type == NodeType::SpotLight && numSpot < 4) {
            std::string i = std::to_string(numSpot);
            set3f(shaderProgram, ("spotPos["   + i + "]").c_str(), node.position);
            set3f(shaderProgram, ("spotDir["   + i + "]").c_str(), node.light.direction);
            set3f(shaderProgram, ("spotColor[" + i + "]").c_str(), node.light.color);
            set1f(shaderProgram, ("spotIntensity[" + i + "]").c_str(), node.light.intensity);
            set1f(shaderProgram, ("spotRange[" + i + "]").c_str(), node.light.range);
            set1f(shaderProgram, ("spotInner[" + i + "]").c_str(), node.light.innerCutoff);
            set1f(shaderProgram, ("spotOuter[" + i + "]").c_str(), node.light.outerCutoff);
            numSpot++;
        } else if (node.type == NodeType::SurfaceLight && !hasSurface) {
            hasSurface = true; surfCol = node.light.color; surfInt = node.light.intensity;
        }
    }

    set1i(shaderProgram, "hasDirLight",      hasDirLight ? 1 : 0);
    set1i(shaderProgram, "numPointLights",   numPoint);
    set1i(shaderProgram, "numSpotLights",    numSpot);
    set1i(shaderProgram, "hasSurface",       hasSurface ? 1 : 0);
    set3f(shaderProgram, "surfaceColor",     surfCol);
    set1f(shaderProgram, "surfaceIntensity", surfInt);

    for (auto& mesh : model.meshes) {
        unsigned int texID = (overrideTexID != 0) ? overrideTexID : mesh.textureID;
        bool useTexture = (texID != 0);
        set1i(shaderProgram, "hasTexture", useTexture ? 1 : 0);
        if (useTexture) {
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

    if (alpha < 1.0f) {
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
    glUniform1f (glGetUniformLocation(billboardProgram, "size"),     size);
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

}