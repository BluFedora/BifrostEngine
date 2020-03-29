//
// Author: Shareef Abdoul-Raheem
// Standard Shader GBuffer
//
// The Position and Normal Data Are In World Space.
//
#version 450

#include "assets/shaders/standard/std_vertex_input.def.glsl"

#include "assets/shaders/standard/camera.ubo.glsl"
#include "assets/shaders/standard/object.ubo.glsl"

layout(location = 0) out vec3 frag_WorldNormal;
layout(location = 1) out vec3 frag_Color;
layout(location = 2) out vec2 frag_UV;

void main()
{
  vec4 object_position = vec4(in_Position.xyz, 1.0);
  vec4 clip_position   = u_ModelViewProjection * object_position;

  frag_WorldNormal = mat3(u_NormalModel) * in_Normal.xyz;
  frag_Color       = in_Color.rgb;
  frag_UV          = in_UV;

  gl_Position = clip_position;
}
