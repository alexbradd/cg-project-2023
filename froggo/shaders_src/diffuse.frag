#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform GlobalUniformObject {
  vec4 ambientColor;
  vec3 lightDir;
  vec4 lightColor;
  vec3 cameraPosition;
} gubo;

layout(set = 1, binding = 0) uniform sampler2D diffuseTex;

void main() {
  vec3 L = normalize(gubo.lightDir);
  vec3 N = normalize(inNormal);

  vec3 diffuseColor = texture(diffuseTex, inTexCoord).rgb;

  vec3 lambert = gubo.lightColor.rgb * diffuseColor * clamp(dot(L, N), 0.0f, 1.0f);

  float ambientStrength = gubo.ambientColor.a;
  vec3 ambient = ambientStrength * (gubo.ambientColor.rgb * diffuseColor);

  outColor = vec4(clamp(lambert + ambient, 0.0f, 1.0f), 1.0f);
}
