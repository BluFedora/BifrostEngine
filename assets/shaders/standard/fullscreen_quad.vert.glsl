//
// Author: Shareef Abdoul-Raheem
// Standard Shader Fullscreen Quad
//
// References:
//   [https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/]
//
// Diagram:
//   (gl_VertexIndex = 0)                                (gl_VertexIndex = 1)
//            ------------------------------------------------------
//            |                          |                       /
//            |                          |                     /
//            |                          |                   /
//            |                          |                 /
//            |                          |               /
//            |                          |             /
//            |       (Scene Rect)       |           /
//            |                          |         /
//            |                          |       /
//            |                          |     /
//            |                          |   /
//            |                          | /
//            |--------------------------/
//            |                        /
//            |                      /
//            |                    /
//            |                  /
//            |                /
//            |              /
//            |            /
//            |          /
//            |        /
//            |      /
//            |    /
//            |  /
//            |/
//   (gl_VertexIndex = 2)
//
// Requires:
//   Front face of CCW.
//   An Empty Vertex Input Assembly
//   vkCmdDraw(commandBuffer, 3, 1, 0, 0);
//
// Bonus Notes:
//   gl_VertexIndex (Vulkan) is the same as gl_VertexID (OpenGL).
//
#version 450

const float k_ZDepth = 1.0f;

#include "assets/shaders/standard/camera.ubo.glsl"

layout(location = 0) out vec3 frag_ViewRay;
layout(location = 1) out vec2 frag_UV;
layout(location = 2) out vec3 frag_ViewRaySSAO;

void main()
{
  vec2 uv             = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  vec4 clip_position  = vec4(uv * 2.0f - 1.0f, k_ZDepth, 1.0f);
  // vec4 world_position = u_CameraInvViewProjection * clip_position;

  // world_position.xyz /= world_position.w;

  vec4 world_position = inverse(u_CameraProjection) * clip_position;
  world_position /= world_position.w;
  world_position = inverse(u_CameraView) * world_position;

  vec4 view_position = Camera_getViewMatrix() * world_position;

  // Light Volume : float3 positionWS = mul(input.PositionOS, WorldMatrix);

  frag_ViewRay     = vec3(world_position) - u_CameraPosition;
  frag_UV          = uv;
  frag_ViewRaySSAO = vec3(view_position.xy / view_position.z, 1.0f);

  // TEST
  //vec4 vp = inverse(u_CameraProjection) * clip_position;

  //vp.xyz /= vp.w;

  //frag_ViewRay = vec3(vp.xy / vp.z, 1.0f);
  // TEST

  gl_Position = clip_position;
}
