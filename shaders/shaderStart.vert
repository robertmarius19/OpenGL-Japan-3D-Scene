#version 410 core

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;

out vec3 fNormal;
out vec4 fPosEye;
out vec2 fTexCoords;
out vec4 fragPosLightSpace;
out vec3 fPos; // Pozitia in World Space

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceTrMatrix;

void main() 
{
    // world space view poz

    fPos = vec3(model * vec4(vPosition, 1.0f));
    
    // fog world space view poz
    fPosEye = view * model * vec4(vPosition, 1.0f);
    
    // normalize world space
    fNormal = normalize(mat3(transpose(inverse(model))) * vNormal);
    
    fTexCoords = vTexCoords;
    fragPosLightSpace = lightSpaceTrMatrix * model * vec4(vPosition, 1.0f);
    
    gl_Position = projection * view * model * vec4(vPosition, 1.0f);
}