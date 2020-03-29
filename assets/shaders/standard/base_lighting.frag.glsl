//
// Author:          Shareef Abdoul-Raheem
// Standard Shader: Base Lighting
//
// #version 450

layout(location = 0) in vec3 frag_ViewRay;
layout(location = 1) in vec2 frag_UV;

#include "assets/shaders/standard/camera.ubo.glsl"

layout(set = 2, binding = 0) uniform sampler2D u_GBufferRT0;
layout(set = 2, binding = 1) uniform sampler2D u_GBufferRT1;
layout(set = 2, binding = 2) uniform sampler2D u_SSAOBlurredBuffer;
layout(set = 2, binding = 3) uniform sampler2D u_DepthTexture;

layout(location = 0) out vec4 o_FragColor0;

#include "assets/shaders/standard/normal_encode.glsl"
#include "assets/shaders/standard/pbr_lighting.glsl"
#include "assets/shaders/standard/position_encode.glsl"

void main()
{
  vec4  gsample0           = texture(u_GBufferRT0, frag_UV);
  vec4  gsample1           = texture(u_GBufferRT1, frag_UV);
  vec4  ssao_sample        = texture(u_SSAOBlurredBuffer, frag_UV);
  vec3  world_normal       = decodeNormal(gsample0.xy);
  float roughness          = gsample0.z;
  float metallic           = gsample0.w;
  vec3  albedo             = gsample1.xyz;
  float ao                 = gsample1.w/* * ssao_sample.r*/;
  vec3  world_position     = constructWorldPos(frag_UV);
  float one_minus_metallic = 1.0f - metallic;
  vec3  v                  = normalize(u_CameraPosition - world_position);
  vec3  f0                 = mix(vec3(0.04), albedo, metallic);  // Reflectance at normal incidence
  float n_dot_v            = dot(world_normal, v);
  vec3  light_out          = vec3(0.0);

  for (int i = 0; i < u_NumLights; ++i)
  {
    Light light        = u_Lights[i];
    vec3  pos_to_light = light.position - world_position;
    vec3  radiance     = calcRadiance(light, pos_to_light);
    vec3  lighting     = mainLighting(
     radiance,
     world_normal,
     albedo,
     roughness,
     one_minus_metallic,
     v,
     normalize(pos_to_light),
     f0,
     n_dot_v);

    light_out += lighting;
  }

  vec3 ambient   = ambientLighting(albedo, ao);
  vec3 lit_color = ambient + light_out;

  o_FragColor0 = vec4(gammaCorrection(tonemapping(lit_color)), 1.0f);
  // o_FragColor0 = vec4(world_normal * 0.5 + 0.5, 1.0f);
}
