#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outColor;
layout(location = 3) out vec2 outTexCoord;
layout(location = 4) out vec3 outTangent;

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

  // Pass stuff to fragment
  mat4 nMat = inverse(transpose(pushConstants.model));

  outPosition = (pushConstants.model * vec4(inPosition, 1.0)).xyz;
  outNormal = (nMat * vec4(inNormal, 0.0)).xyz;
  outColor = inColor;
  outTexCoord = inTexCoord;
  outTangent = (pushConstants.model * vec4(inTangent, 0.0)).xyz;
}
