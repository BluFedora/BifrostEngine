
//
// Camera Uniform Layout
//
layout(std140, set = 0, binding = 0) uniform u_Set0
{
  mat4 u_CameraProjection;
  mat4 u_CameraInvViewProjection;
  vec3 u_CameraForward;
  vec3 u_CameraPosition;
};
