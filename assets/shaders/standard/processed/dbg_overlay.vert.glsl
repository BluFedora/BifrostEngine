#version 450

layout(location = 0) in vec4 in_Position;
layout(location = 1) in vec3 in_Offset;
layout(location = 2) in vec4 in_Color;


//
// Camera Uniform Layout
//
layout(std140, set = 0, binding = 0) uniform u_Set0
{
  mat4  u_CameraProjection;
  mat4  u_CameraInvViewProjection;
  mat4  u_CameraViewProjection;
  vec3  u_CameraForward;
  float u_Time;
  vec3  u_CameraPosition;
};

layout(location = 0) out vec3 frag_Color;

void main()
{
  vec4 clip_position = u_CameraViewProjection * in_Position;

  frag_Color = in_Color.rgb;

  gl_Position = clip_position;
}
