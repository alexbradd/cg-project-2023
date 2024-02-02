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
layout(set = 1, binding = 1) uniform sampler2D MRAOTex;

const float PI = 3.14159265358979323846f;
const float F0 = 0.3f;

float ggx_D(float rho2, vec3 N, vec3 H) {
  float dotClamp = clamp(dot(N, H), 0.01, 1.0f);
  float dotClamp2 = dotClamp * dotClamp;

  float denom = dotClamp2 * (rho2 - 1) + 1;
  float denom2 = PI * denom * denom;

  return rho2 / denom2;
}

float ggx_F(vec3 V, vec3 H) {
  float c = 1 - clamp(dot(V, H), 0.0f, 1.0f);
  float c5 = c * c * c * c * c;
  return F0 + ((1 - F0) * c5);
}

float ggx_g(float rho2, vec3 N, vec3 A) {
  float N_A = dot(N, A);
  float dot2 = N_A * N_A;

  float frac = (1 - dot2) / dot2;
  float denom = 1 + sqrt(1 + (rho2 * frac));
  return 2 / denom;
}

void main() {
  vec3 L = normalize(gubo.lightDir);
  vec3 N = normalize(inNormal);
  vec3 V = normalize(gubo.cameraPosition - inPosition);

  vec3 MRAO = texture(MRAOTex, inTexCoord).rgb;

  // Lambert diffuse
  vec3 diffuseColor = texture(diffuseTex, inTexCoord).rgb;
  vec3 lambert = gubo.lightColor.rgb * diffuseColor * clamp(dot(L, N), 0.0f, 1.0f);

  // Cook-torrance reflection
  vec3 Ms = vec3(1.0f);
  vec3 H = normalize(L + V);

  float metallic = MRAO.r;
  float k = 1.0f - metallic;

  float roughness = MRAO.g;
  float roughness2 = roughness * roughness;

  float D = ggx_D(roughness2, N, H);
  float F = ggx_F(V, H);
  float G = ggx_g(roughness2, N, V) * ggx_g(roughness2, N, L);

  vec3 specular = Ms * ((D * F * G) / (4 * clamp(dot(V, N), 0.01f, 1.0f)));

  // Ambient lighting
  float ambientStrength = MRAO.b * gubo.ambientColor.a;
  vec3 ambient = ambientStrength * (gubo.ambientColor.rgb * diffuseColor);

  outColor = vec4(clamp(k * lambert + (1.0f - k) * specular + ambient, 0.0f, 1.0f), 1.0f);
}
