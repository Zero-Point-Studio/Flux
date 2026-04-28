#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

void main() {
    vec3 zenithColor = vec3(0.1, 0.3, 0.6);
    vec3 horizonColor = vec3(0.7, 0.8, 0.9);
    FragColor = texture(skybox, TexCoords);

    vec3 viewDir = normalize(TexCoords);
    float height = max(viewDir.y, 0.0);

    vec3 finalColor = mix(horizonColor, zenithColor, height);

    vec3 sunDirection = normalize(vec3(0.5, 0.5, -1.0));
    float sunAlignment = max(dot(viewDir, sunDirection), 0.0);
    float sunGlare = pow(sunAlignment, 256.0);

    FragColor = vec4(finalColor + (vec3(1.0, 0.9, 0.7) * sunGlare), 1.0);
}