#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform GlobalUniformObject {
  mat4 projection;
  mat4 view;
} gubo;

// Faster than using the uniform. Downside is we have only 128 bytes
// in total (guaranteed by the vulkan spec)
layout(push_constant) uniform push_constant {
    mat4 model; // 64 bytes
    mat4 _unused; // 64 bytes
} pushConstants;

void main() {
  gl_Position = gubo.projection * gubo.view * pushConstants.model * vec4(inPosition, 1.0);
}
