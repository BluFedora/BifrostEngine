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

layout(location = 0) out vec3 frag_ViewRay;
layout(location = 1) out vec2 frag_UV;

void main()
{
  vec2 uv             = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  vec4 clip_position  = vec4(uv * 2.0f - 1.0f, k_ZDepth, 1.0f);
  vec4 world_position = u_CameraInvViewProjection * clip_position;

  world_position.xyz /= world_position.w;

  // Light Volume : float3 positionWS = mul(input.PositionOS, WorldMatrix);

  frag_ViewRay = vec3(world_position) - u_CameraPosition;
  frag_UV      = uv;

  gl_Position = clip_position;
}
