#version 330 core
        out vec4 FragColor;

        in vec3 Normal;
        in vec3 FragPos;

        uniform vec3 viewPos;

        void main() {
            vec3 lightColor = vec3(1.0, 1.0, 1.0);
            vec3 lightDir = normalize(vec3(5.0, 10.0, 5.0));
            vec3 objectColor = vec3(0.8, 0.4, 0.1);

            float ambientStrength = 0.2;
            vec3 ambient = ambientStrength * lightColor;
            
            vec3 norm = normalize(Normal);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;

            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);  
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrength * spec * lightColor;  

            vec3 result = (ambient + diffuse + specular) * objectColor;
            FragColor = vec4(result, 1.0);
        }