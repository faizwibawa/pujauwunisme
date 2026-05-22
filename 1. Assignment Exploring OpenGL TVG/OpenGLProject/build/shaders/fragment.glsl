#version 460 core

in vec3 FragPos;
in vec3 Normal;

uniform vec3 uViewPos;
uniform vec3 uLightPos;

// Material properties
uniform vec3 uObjectColor;
uniform vec3 uAmbient;
uniform vec3 uDiffuse;
uniform vec3 uSpecular;
uniform float uShininess;

out vec4 FragColor;

void main()
{
    // Ambient
    vec3 ambient = uAmbient * uObjectColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(uLightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = uDiffuse * diff * uObjectColor;

    // Specular
    vec3 viewDir = normalize(uViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uShininess);
    vec3 specular = uSpecular * spec;

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}