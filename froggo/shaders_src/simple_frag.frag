#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform GlobalUniformObject {
  vec3 ambientColor;
  vec3 lightDir;
  vec4 lightColor;
  vec3 cameraPosition;
} gubo;

void main() {
    outColor = vec4(normalize(inNormal), 1.0);
}
