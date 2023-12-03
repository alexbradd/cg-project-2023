#version 450

layout(location = 0) in vec3 in_position;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = vec4(in_position.r + 0.5, in_position.g + 0.5, in_position.b + 0.5, 1.0);
}
