#version 410 core

in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;
in vec3 fPos; // World Space

out vec4 fColor;

// uniforms
uniform vec3 lightDir; 
uniform vec3 lightColor;
uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;
uniform vec3 viewPos; 

// point lights
uniform vec3 pointLightPositions[6];
uniform vec3 pointLightColors[6];
uniform int numberOfLights;

// spot lights
struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
};
uniform SpotLight spotLight[2];

uniform float lightConstant;
uniform float lightLinear;
uniform float lightQuadratic;
uniform int fogInit;

float shininess = 32.0f;

float computeShadow() {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if(projCoords.z > 1.0) return 0.0;
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    float bias = 0.005;
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    return shadow;
}

vec3 computeDirLight(vec3 normal, vec3 viewDir, vec3 color) {
    vec3 lightDirN = normalize(lightDir);
    vec3 ambient = 0.05f * lightColor * color;
    float diff = max(dot(normal, lightDirN), 0.0f);
    vec3 diffuse = diff * lightColor * color;
    vec3 reflectDir = reflect(-lightDirN, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), shininess);
    vec3 specular = spec * lightColor; 
    float shadow = computeShadow();
    return (ambient + (1.0 - shadow) * (diffuse + specular));
}

vec3 computePointLight(int index, vec3 normal, vec3 viewDir, vec3 color) {
    vec3 lightPos = pointLightPositions[index];
    vec3 lightCol = pointLightColors[index];
    vec3 lightDirPt = normalize(lightPos - fPos); 
    float diff = max(dot(normal, lightDirPt), 0.0);
    vec3 reflectDir = reflect(-lightDirPt, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    float distance = length(lightPos - fPos);
    float attenuation = 1.0 / (lightConstant + lightLinear * distance + lightQuadratic * distance * distance);    
    vec3 ambient  = 0.05 * lightCol * color; 
    vec3 diffuse  = diff * lightCol * color;
    vec3 specular = spec * lightCol;
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

vec3 computeSpotLight(SpotLight spot, vec3 normal, vec3 viewDir, vec3 color) {
    vec3 lightDirS = normalize(spot.position - fPos); // World - World
    float theta = dot(lightDirS, normalize(-spot.direction));
    float epsilon = spot.cutOff - spot.outerCutOff;
    float intensity = clamp((theta - spot.outerCutOff) / epsilon, 0.0, 1.0);
    float diff = max(dot(normal, lightDirS), 0.0);
    vec3 reflectDir = reflect(-lightDirS, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    float distance = length(spot.position - fPos);
    float attenuation = 1.0 / (spot.constant + spot.linear * distance + spot.quadratic * distance * distance);    
    vec3 diffuse  = diff * spot.color * color;
    vec3 specular = spec * spot.color;
    diffuse  *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (diffuse + specular);
}

void main() {
    vec3 normal = normalize(fNormal);
    vec3 viewDir = normalize(viewPos - fPos); // Vector spre camera
    vec4 texColorFull = texture(diffuseTexture, fTexCoords);
    if(texColorFull.a < 0.1) discard;
    vec3 texColor = texColorFull.rgb;

    vec3 outputColor = computeDirLight(normal, viewDir, texColor);
    
    for(int i = 0; i < numberOfLights; i++) {
        outputColor += computePointLight(i, normal, viewDir, texColor);
    }
    
    for(int j = 0; j < 2; j++) {
        outputColor += computeSpotLight(spotLight[j], normal, viewDir, texColor);
    }
    
    if (fogInit == 1) {
        float fogDensity = 0.02f; 
        float fragmentDistance = length(fPosEye.xyz);
        float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));
        fogFactor = clamp(fogFactor, 0.0f, 1.0f);
        vec4 fogColor = vec4(lightColor, 1.0f); 
        fColor = mix(fogColor, vec4(outputColor, 1.0f), fogFactor);
    } else {
        fColor = vec4(outputColor, 1.0f);
    }
}