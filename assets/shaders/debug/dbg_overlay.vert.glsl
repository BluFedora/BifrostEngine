#version 450

layout(location = 0) in vec4 in_Position;
layout(location = 1) in vec3 in_Offset;
layout(location = 2) in vec4 in_Color;

#include "assets/shaders/standard/camera.ubo.glsl"

layout(location = 0) out vec3 frag_Color;

void main()
{
  vec4 clip_position = u_CameraViewProjection * in_Position;

  frag_Color = in_Color.rgb;

  gl_Position = clip_position;
}
