//
// Author: Shareef Abdoul-Raheem
// Standard Shader GBuffer
//
#version 450

layout(location = 0) in vec4 in_Position;
layout(location = 1) in vec4 in_Normal;
layout(location = 2) in vec4 in_Color;
layout(location = 3) in vec2 in_UV;

layout(std140, set = 0, binding = 0) uniform u_Set0
{
  mat4 u_ViewProjection;
  vec3 u_CameraPosition;
};

layout(std140, set = 3, binding = 0) uniform u_Set2
{
  mat4 u_ModelTransform;
};

layout(location = 0) out vec4 frag_Position;
layout(location = 1) out vec4 frag_Normal;
layout(location = 2) out vec4 frag_Color;
layout(location = 3) out vec2 frag_UV;

void main() 
{
  vec4 world_position = u_ModelTransform * in_Position;

  gl_Position = world_position;
  
  frag_Position = world_position;
  frag_Normal   = in_Normal;
  frag_Color    = in_Color;
  frag_UV       = in_UV;
}
