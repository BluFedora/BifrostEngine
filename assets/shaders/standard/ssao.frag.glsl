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

#include "assets/shaders/standard/camera.ubo.glsl"

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
#include "assets/shaders/standard/position_encode.glsl"
#include "assets/shaders/standard/normal_encode.glsl"

float zAtSample(vec2 sampling_offset)
{
  vec3  world_pos = constructWorldPos(sampling_offset);

  float mat02 = u_CameraView[0][2];
  float mat12 = u_CameraView[1][2];
  float mat22 = u_CameraView[2][2];
  float mat32 = u_CameraView[3][2];

  return dot(world_pos, vec3(mat02, mat12, mat22)) + mat32;

  // return (u_CameraView * vec4(constructWorldPos(sampling_offset.xy), 1.0)).z
}

void main()
{
  ivec2 noise_scale = textureSize(u_NormalTexture, 0) / textureSize(u_NoiseTexture, 0);  // TODO: Check if I should make this a floating point division.
  // vec2  noise_scale = vec2(1.0);
  vec3  view_pos    = (u_CameraView * vec4(constructWorldPos(frag_UV), 1.0)).xyz;
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

    float sample_depth = zAtSample(sampling_offset.xy);
    float check_range  = smoothstep(0.0, 1.0, u_SampleRadius / abs(view_pos.z - sample_depth));

    ao_factor += check_range * step(sampling_pos.z + u_SampleBias, sample_depth); // (((sampling_pos.z + u_SampleBias) <= sample_depth) ? 1.0f : 0.0f)
  }
  
  // 'Invert' the ao factor.
  ao_factor = 1.0 - (ao_factor / float(k_KernelSize));

  // Squaring the value is an aethetic choice. Can be adjusted to other powers (including ao_factor^1.0).
  o_FragColor0 = ao_factor * ao_factor;
}
