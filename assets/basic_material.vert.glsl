#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv;

layout(std140, binding = 1) uniform u_Set0
{
  mat4 u_ModelTransform;
};

layout(location = 0) out vec4 frag_Color;
layout(location = 1) out vec2 frag_UV;

void main() 
{
  gl_Position = u_ModelTransform * position;
  frag_Color  = color;
  frag_UV     = uv;
}
