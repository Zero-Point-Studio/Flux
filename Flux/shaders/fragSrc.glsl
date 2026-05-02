#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;

uniform vec3  viewPos;
uniform bool  hasTexture;
uniform sampler2D albedo;
uniform vec3  matColor;
uniform float alpha;

uniform sampler2D shadowMap;
uniform bool      hasShadowMap;

#define MAX_POINT 8
#define MAX_SPOT  4

uniform vec3  dirLightDir;
uniform vec3  dirLightColor;
uniform float dirLightIntensity;
uniform bool  hasDirLight;

uniform int   numPointLights;
uniform vec3  pointPos[MAX_POINT];
uniform vec3  pointColor[MAX_POINT];
uniform float pointIntensity[MAX_POINT];
uniform float pointRange[MAX_POINT];

// Helper: Industry-standard inverse-square attenuation
float getAttenuation(float dist, float range) {
    float atten = 1.0 / (dist * dist); 
    float window = pow(clamp(1.0 - pow(dist/range, 4.0), 0.0, 1.0), 2.0);
    return atten * window;
}

float ShadowFactor(vec4 fragPosLS, vec3 n, vec3 ld) {
    if (!hasShadowMap) return 1.0;
    vec3 proj = fragPosLS.xyz / fragPosLS.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0) return 1.0;

    float cosTheta = clamp(dot(n, ld), 0.0, 1.0);
    float bias = max(0.002 * (1.0 - cosTheta), 0.0005);

    // 5x5 PCF for soft edges
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float shadow = 0.0;
    for(int x = -2; x <= 2; ++x) {
        for(int y = -2; y <= 2; ++y) {
            float pcfDepth = texture(shadowMap, proj.xy + vec2(x, y) * texelSize).r; 
            shadow += proj.z - bias > pcfDepth  ? 0.0 : 1.0;
        }
    }
    return shadow / 25.0;
}

vec3 calcDir(vec3 n, vec3 vd) {
    if (!hasDirLight) return vec3(0.0);
    vec3 ld = normalize(-dirLightDir);
    float diff = max(dot(n, ld), 0.0);
    vec3 halfwayDir = normalize(ld + vd);  
    float spec = pow(max(dot(n, halfwayDir), 0.0), 128.0);
    float shadow = ShadowFactor(FragPosLightSpace, n, ld);
    return dirLightColor * dirLightIntensity * shadow * (diff + spec);
}

vec3 calcPoint(int i, vec3 n, vec3 vd) {
    vec3 ld = pointPos[i] - FragPos;
    float dist = length(ld);
    if (dist > pointRange[i]) return vec3(0.0);
    ld = normalize(ld);
    float diff = max(dot(n, ld), 0.0);
    vec3 halfwayDir = normalize(ld + vd);
    float spec = pow(max(dot(n, halfwayDir), 0.0), 128.0);
    return pointColor[i] * pointIntensity[i] * getAttenuation(dist, pointRange[i]) * (diff + spec);
}

void main() {
    vec3 n = normalize(Normal);
    vec3 vd = normalize(viewPos - FragPos);
    
    // 1. Get Base Color and convert to Linear Space (Industry Standard)
    vec3 b = hasTexture ? texture(albedo, TexCoords).rgb : matColor;
    b = pow(b, vec3(2.2)); 

    // 2. Additive Lighting (Point lights hit shadows!)
    vec3 ambient = vec3(0.03) * b; // Low ambient for depth
    vec3 direct = calcDir(n, vd) * b;
    
    vec3 points = vec3(0.0);
    for (int i = 0; i < numPointLights; i++) {
        points += calcPoint(i, n, vd) * b;
    }

    vec3 result = ambient + direct + points;

    // 3. Reinhard Tone Mapping (Prevents "nuclear" white highlights)
    result = result / (result + vec3(1.0));

    // 4. Gamma Correction (The "Final Polish" for realistic monitor output)
    result = pow(result, vec3(1.0/2.2)); 

    FragColor = vec4(result, alpha);
}