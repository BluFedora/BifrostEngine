#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;

layout(std140, binding = 1) uniform u_Set0
{
  mat4 u_Projection;
};

layout(location = 0) out vec4 frag_Color;
layout(location = 1) out vec2 frag_UV;

void main() 
{
  gl_Position = u_Projection * vec4(position, 0.0, 1.0);
  frag_Color  = color;
  frag_UV     = uv;
}
