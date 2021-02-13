//
// Author:          Shareef Abdoul-Raheem
// Standard Shader: GBuffer Generation
//
#version 450

layout(location = 0) in vec3 frag_WorldNormal;
layout(location = 1) in vec3 frag_Color;
layout(location = 2) in vec2 frag_UV;

layout(set = 2, binding = 0) uniform sampler2D u_AlbedoTexture;
layout(set = 2, binding = 1) uniform sampler2D u_NormalTexture;
layout(set = 2, binding = 2) uniform sampler2D u_MetallicTexture;
layout(set = 2, binding = 3) uniform sampler2D u_RoughnessTexture;
layout(set = 2, binding = 4) uniform sampler2D u_AmbientOcclusionTexture;

layout(location = 0) out vec4 o_FragColor0;
layout(location = 1) out vec4 o_FragColor1;

#include "assets/shaders/standard/camera.ubo.glsl"

#include "assets/shaders/standard/normal_encode.glsl"

void main()
{
  float metallic          = texture(u_MetallicTexture, frag_UV).r;
  float roughness         = texture(u_RoughnessTexture, frag_UV).r;
  float ambient_occlusion = texture(u_AmbientOcclusionTexture, frag_UV).r;
  vec2  normal            = encodeNormal(normalize(frag_WorldNormal));  // TODO: Normal mapping
  vec3  albedo            = vec3(0.7, 0.3, (sin(u_Time * 5.0f) + 1.0f) * 0.5f) * frag_Color;

  o_FragColor0 = vec4(normal, roughness, metallic);
  o_FragColor1 = vec4(albedo.xyz, ambient_occlusion);
}
