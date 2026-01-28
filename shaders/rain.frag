#version 410 core

out vec4 fColor;
uniform vec3 color; // Culoarea ploii

void main() {
   
    fColor = vec4(color, 0.6f); 
}