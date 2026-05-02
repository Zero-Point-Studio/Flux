#version 330 core
out vec4 FragColor;
in vec3 TexCoords; 
uniform vec3 sunDir;

void main() {
    vec3 viewDir = normalize(TexCoords);
    vec3 lightDir = normalize(-sunDir);
    
    vec3 zenithColor = vec3(0.05, 0.15, 0.4);
    vec3 horizonColor = vec3(0.4, 0.6, 0.8);
    
    float haze = pow(1.0 - max(viewDir.y, 0.0), 3.0);
    vec3 sky = mix(zenithColor, horizonColor, haze);

    float sunDot = max(dot(viewDir, lightDir), 0.0);
    float solarGlow = pow(sunDot, 100.0) * 2.0;
    float sunDisk = pow(sunDot, 1500.0) * 5.0;
    
    vec3 sunFinal = (solarGlow * vec3(1.0, 0.8, 0.5)) + (sunDisk * vec3(1.0, 1.0, 0.9));

    vec3 color = sky + sunFinal;
    color = color / (color + vec3(1.0))
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}