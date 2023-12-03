#version 450

layout(location = 0) in vec3 in_position;
layout(location = 0) out vec3 frag_color;

void main() {
    gl_Position = vec4(in_position, 1.0);
    frag_color = vec3(0);
}
