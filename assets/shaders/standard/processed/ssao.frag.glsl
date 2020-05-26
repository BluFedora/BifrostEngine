//
// Author: Shareef Abdoul-Raheem
// Standard Shader SSAO
//
// References:
//   [http://ogldev.atspace.co.uk/www/tutorial45/tutorial45.html]
//   [https://learnopengl.com/Advanced-Lighting/SSAO]
//
// Pair this module with "fullscreen_quad.vert.glsl"
//
#version 450

const int k_KernelSize = 128;

layout(location = 0) in vec3 frag_ViewRay;
layout(location = 1) in vec2 frag_UV;
layout(location = 2) in vec3 frag_ViewRaySSAO;


//
// Camera Uniform Layout
//
layout(std140, set = 0, binding = 0) uniform u_Set0
{
  mat4  u_CameraProjection;
  mat4  u_CameraInvViewProjection;
  mat4  u_CameraViewProjection;
  mat4  u_CameraView;
  vec3  u_CameraForward;
  float u_Time;
  vec3  u_CameraPosition;
  float u_Pad0;
  vec3  u_CameraAmbient;
};

mat4 Camera_getViewMatrix()
{
  return u_CameraView;
}

layout(set = 2, binding = 0) uniform sampler2D u_DepthTexture;
layout(set = 2, binding = 1) uniform sampler2D u_NormalTexture;
layout(set = 2, binding = 2) uniform sampler2D u_NoiseTexture;

layout(std140, set = 2, binding = 3) uniform u_Set2
{
  vec4  u_Kernel[k_KernelSize];
  float u_SampleRadius;
  float u_SampleBias;
};

layout(location = 0) out float o_FragColor0;

#define SSAO 1
//
// Bad Design Notes:
//   > Assumes Both Camera and u_DepthTexture uniforms...
//   > Assumes frag_ViewRay as one of the fragment inputs...
//

// Assumed depth range of [0.0 - 1.0]
// Depth needs to be '2.0f * depth - 1.0f' if the
// perspective expects [-1.0f - 1.0f] like in OpenGL.
float constructLinearDepth(vec2 uv)
{
  float depth        = texture(u_DepthTexture, uv).x;
  float linear_depth = u_CameraProjection[3][2] / (depth + u_CameraProjection[2][2]);

  return linear_depth;
}

#if SSAO
vec3 constructViewPos(vec2 uv)
{
  return frag_ViewRaySSAO * constructLinearDepth(uv);
}
#endif

vec3 constructWorldPos(vec2 uv)
{
  vec3  view_ray     = normalize(frag_ViewRay);
  float linear_depth = constructLinearDepth(uv);
  float view_z_dist  = dot(u_CameraForward, view_ray);

  return u_CameraPosition + view_ray * (linear_depth / view_z_dist);
}

// [https://aras-p.info/texts/CompactNormalStorage.html]
// [https://mynameismjp.wordpress.com/2009/06/17/storing-normals-using-spherical-coordinates/]

const float k_SimplePI = 3.14159f;

vec2 encodeNormal(vec3 n)
{
  vec2 spherical;

  spherical.x = atan(n.y, n.x) / k_SimplePI;
  spherical.y = n.z;

  return spherical * 0.5f + 0.5f;
}

vec2 sincos(float x)
{
  return vec2(sin(x), cos(x));
}

vec3 decodeNormal(vec2 spherical)
{
  spherical = spherical * 2.0f - 1.0f;

  vec2 sin_cos_theta = sincos(spherical.x * k_SimplePI);
  vec2 sin_cos_phi   = vec2(sqrt(1.0 - spherical.y * spherical.y), spherical.y);

  return vec3(sin_cos_theta.y * sin_cos_phi.x, sin_cos_theta.x * sin_cos_phi.x, sin_cos_phi.y);
}

void main()
{
  ivec2 noise_scale = textureSize(u_NormalTexture, 0) / textureSize(u_NoiseTexture, 0);  // TODO: Check if I should make this a floating point division.
  vec3  view_pos    = (Camera_getViewMatrix() * vec4(constructWorldPos(frag_UV), 1.0)).xyz;
  vec3  view_normal = normalize((u_CameraView * vec4(decodeNormal(texture(u_NormalTexture, frag_UV).xy), 0.0)).xyz);
  vec3  rand_dir    = texture(u_NoiseTexture, frag_UV * noise_scale).xyz;
  vec3  tangent     = normalize(rand_dir - view_normal * dot(rand_dir, view_normal));
  vec3  bitangent   = cross(view_normal, tangent);
  mat3  tbn_mat     = mat3(tangent, bitangent, view_normal);  // Gramm-Schmidt process
  float ao_factor   = 0.0;

  for (int i = 0; i < k_KernelSize; ++i)
  {
    vec3 sampling_pos    = view_pos + (tbn_mat * u_Kernel[i].xyz) * u_SampleRadius;  // The 'tbn_mat' brings the u_Kernel sample from tangent to view space.
    vec4 sampling_offset = u_CameraProjection * vec4(sampling_pos, 1.0);             // View => Clip Space
    sampling_offset.xy   = (sampling_offset.xy / sampling_offset.w) * 0.5 + 0.5;     // Perspective Divide Followed by a Map to the [0, 1] range.

    float sample_depth = (Camera_getViewMatrix() * vec4(constructWorldPos(sampling_offset.xy), 1.0)).z;
    float check_range  = smoothstep(0.0, 1.0, u_SampleRadius / abs(view_pos.z - sample_depth));

    ao_factor += check_range * step(sampling_pos.z + u_SampleBias, sample_depth); // (((sampling_pos.z + u_SampleBias) <= sample_depth) ? 1.0f : 0.0f)
  }

  // 'Invert' the ao factor.
  ao_factor = 1.0 - (ao_factor / float(k_KernelSize));

  // Squaring the value is an aethetic choice. Can be adjusted to other powers (including ao_factor^1.0).
  o_FragColor0 = ao_factor * ao_factor;
}
