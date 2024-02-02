#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform GlobalUniformObject {
  vec4 ambientColor;
  vec3 lightDir;
  vec4 lightColor;
  vec3 cameraPosition;
} gubo;

layout(set = 1, binding = 0) uniform sampler2D diffuseTex;
layout(set = 1, binding = 1) uniform sampler2D specularTex;
layout(set = 1, binding = 2) uniform sampler1D diffuseMap;
layout(set = 1, binding = 3) uniform sampler1D specularMap;

void main() {
  vec3 L = normalize(gubo.lightDir);
  vec3 N = normalize(inNormal);
  vec3 V = normalize(gubo.cameraPosition - inPosition);

  // Colors
  vec3 diffuseColor = texture(diffuseTex, inTexCoord).rgb;
  vec3 specularColor = texture(specularTex, inTexCoord).rgb;

  // Lambert diffuse. The shadow strength is mapped to the diffuseMap.
  float lightIntensity = clamp(dot(L, N), 0.01f, 0.99f);
  float mappedIntensity = texture(diffuseMap, lightIntensity).r * lightIntensity;
  vec3 lambert = (gubo.lightColor.rgb * mappedIntensity) * diffuseColor ;

  // Specular with Phong with gamma = 5. Specular color is chosen from the
  // specular texture, while the light intensity is mapped to the one in specularMap
  vec3 r = -reflect(L, N);
  float specIntensity = pow(clamp(dot(V, r), 0.01f, 0.99f), 1.0f);
  float mappedSpecIntensity = texture(specularMap, specIntensity).r * specIntensity;
  vec3 specular = specularColor * mappedSpecIntensity;

  // Ambient lighting
  float ambientStrength = gubo.ambientColor.a;
  vec3 ambient = ambientStrength * (gubo.ambientColor.rgb * diffuseColor);

  outColor = vec4(clamp(lambert + specular + ambient, 0.0f, 1.0f), 1.0f);
}
