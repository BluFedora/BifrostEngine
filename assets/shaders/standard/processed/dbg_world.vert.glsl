//
// Author: Shareef Abdoul-Raheem
// Debug Shader Screen Space Lines
//
// References:
//   [https://mattdesl.svbtle.com/drawing-lines-is-hard]
//   [https://github.com/mattdesl/three-line-2d/blob/master/test/shader-dash.js]
//
#version 450

const float k_Aspect      = 1600.0f / 900.0f;
const bool  k_DoMiterJoin = true;

layout(location = 0) in vec4 in_CurrPosition;
layout(location = 1) in vec4 in_NextPosition;
layout(location = 2) in vec4 in_PrevPosition;
layout(location = 3) in vec4 in_Color;
layout(location = 4) in float in_Direction;
layout(location = 5) in float in_Thickness;


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
  vec2 aspect    = vec2(k_Aspect, 1.0f);
  vec4 clip_curr = u_CameraViewProjection * vec4(in_CurrPosition.xyz, 1.0f);
  vec4 clip_next = u_CameraViewProjection * vec4(in_NextPosition.xyz, 1.0f);
  vec4 clip_prev = u_CameraViewProjection * vec4(in_PrevPosition.xyz, 1.0f);

  vec2 screen_curr = clip_curr.xy / clip_curr.w * aspect;
  vec2 screen_next = clip_next.xy / clip_next.w * aspect;
  vec2 screen_prev = clip_prev.xy / clip_prev.w * aspect;

  vec2  direction = vec2(0.0f);
  float thickness = in_Thickness;

  if (screen_curr == screen_prev)  // First Vertex in Chain
  {
    direction = normalize(screen_next - screen_curr);
  }
  else if (screen_curr == screen_next)  // Last Vertex in Chain
  {
    direction = normalize(screen_curr - screen_prev);
  }
  else  // Middle of the Chain
  {
    direction = normalize(screen_curr - screen_prev);

    if (k_DoMiterJoin)
    {
      vec2 direction_b = normalize(screen_next - screen_curr);
      vec2 tangent     = normalize(direction + direction_b);
      vec2 perp        = vec2(-direction.y, direction.x);
      vec2 miter       = vec2(-tangent.y, tangent.x);

      direction = tangent;
      thickness = in_Thickness / dot(miter, perp);
    }
  }

  vec2 normal = vec2(-direction.y, direction.x) * thickness * 0.5f;
  normal.x /= k_Aspect;

  frag_Color = in_Color.rgb;

  gl_Position = clip_curr + vec4(normal * in_Direction, 0.0f, 0.0f);
}
