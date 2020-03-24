//
// Author: Shareef Abdoul-Raheem
// Standard Shader GBuffer
//
#version 450

layout(set = 2, binding = 0) uniform sampler2D u_AlbedoTexture;
layout(set = 2, binding = 1) uniform sampler2D u_NormalTexture;
layout(set = 2, binding = 2) uniform sampler2D u_MetallicTexture;
layout(set = 2, binding = 3) uniform sampler2D u_RoughnessTexture;
layout(set = 2, binding = 4) uniform sampler2D u_AmbientOcclusionTexture;

layout(std140, set = 1, binding = 0) uniform u_Set1
{
  vec3 u_LightColor;
  vec3 u_LightPosition;
  vec3 u_LightDirection;
};

layout(location = 0) in vec4 frag_Position;
layout(location = 1) in vec4 frag_Normal;
layout(location = 2) in vec4 frag_Color;
layout(location = 3) in vec2 frag_UV;

layout(location = 0) out vec4 o_FragColor0;
layout(location = 1) out vec4 o_FragColor1;
layout(location = 2) out vec4 o_FragColor2;

void main()
{
  float metalic           = 0.5;
  float ambient_occlusion = 1.0;
  float roughness         = 1.0;

  o_FragColor0 = vec4(frag_Position.xyz, roughness);
  o_FragColor1 = vec4(frag_Normal.xyz, ambient_occlusion);
  o_FragColor2 = vec4((texture(u_AlbedoTexture, frag_UV) * frag_Color).xyz, metalic);
}
