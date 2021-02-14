//
// Author:       Shareef Abdoul-Raheem
// Debug Shader: World Lines
//
#version 450

layout(location = 0) in vec3 frag_Color;

layout(location = 0) out vec4 o_FragColor0;
layout(location = 1) out vec4 o_FragColor1;

void main()
{
  float metallic          = 0.0f;
  float roughness         = 0.0f;
  float ambient_occlusion = 1.0f;
  vec2  normal            = vec2(0.707f, 0.707f); // {0.5f, 0.5f} direction.
  vec3  albedo            = frag_Color;

  o_FragColor0 = vec4(normal, roughness, metallic);
  o_FragColor1 = vec4(albedo.xyz, ambient_occlusion);
}
