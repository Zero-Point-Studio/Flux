#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosLightSpace;   // BUG 1: needed by fragment shader for shadow map lookup

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;   // BUG 1: set by Renderer3D::DrawScene each frame

void main()
{
    vec4 worldPos   = model * vec4(aPos, 1.0);
    FragPos         = vec3(worldPos);
    Normal          = mat3(transpose(inverse(model))) * aNormal;
    TexCoords       = aTexCoords;

    // BUG 1 FIX: transform the fragment into light clip-space for the PCF lookup
    FragPosLightSpace = lightSpaceMatrix * worldPos;

    gl_Position = projection * view * worldPos;
}
