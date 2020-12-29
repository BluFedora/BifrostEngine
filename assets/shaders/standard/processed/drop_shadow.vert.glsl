// Reference: [http://madebyevan.com/shaders/fast-rounded-rectangle-shadows/]

#version 450

layout(location = 0) in vec2  in_Position;
layout(location = 1) in float in_ShadowSigma;
layout(location = 2) in float in_CornerRadius;
layout(location = 3) in vec4  in_Rect; // {min-x, min-y, max-x, max-y}
layout(location = 4) in vec4  in_Color;

layout(std140, binding = 0) uniform u_Set0
{
  mat4 u_Projection;
};

layout(location = 0) out vec2  frag_Pos;
layout(location = 1) out vec4  frag_Color;
layout(location = 2) out vec4  frag_Rect;
layout(location = 3) out float frag_Sigma;
layout(location = 4) out float frag_CornerRadius;

void main() 
{
  gl_Position = u_Projection * vec4(in_Position, 0.0, 1.0);

  frag_Pos          = in_Position;
  frag_Color        = in_Color;
  frag_Rect         = in_Rect;
  frag_Sigma        = in_ShadowSigma;
  frag_CornerRadius = in_CornerRadius;
}
