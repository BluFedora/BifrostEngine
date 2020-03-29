#version 450

#define IS_SPOT_LIGHT 1
//
// Author:          Shareef Abdoul-Raheem
// Standard Shader: Base Lighting
//
// #version 450

layout(location = 0) in vec3 frag_ViewRay;
layout(location = 1) in vec2 frag_UV;


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

layout(set = 2, binding = 0) uniform sampler2D u_GBufferRT0;
layout(set = 2, binding = 1) uniform sampler2D u_GBufferRT1;
layout(set = 2, binding = 2) uniform sampler2D u_SSAOBlurredBuffer;
layout(set = 2, binding = 3) uniform sampler2D u_DepthTexture;

layout(location = 0) out vec4 o_FragColor0;


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
//
// Author:          Shareef Abdoul-Raheem
// Standard Shader: PBR Lighting
//
// References:
//   [https://github.com/google/filament/blob/master/shaders/src/brdf.fs]
//   [https://google.github.io/filament/Filament.md.html]
//   [https://learnopengl.com/PBR/Lighting]
//   [https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf]
//   [http://filmicworlds.com/blog/optimizing-ggx-shaders-with-dotlh/]
//

#ifndef IS_DIRECTIONAL_LIGHT
#define IS_DIRECTIONAL_LIGHT 0
#endif

#ifndef IS_POINT_LIGHT
#define IS_POINT_LIGHT 0
#endif

#ifndef IS_SPOT_LIGHT
#define IS_SPOT_LIGHT 0
#endif

const float k_PI                           = 3.14159265359;
const float k_GammaCorrectionFactor        = 1.0 / 2.2;
const float k_Epsilon                      = 1e-4;
const int   k_MaxPunctualLightsOnScreen    = 512;
const int   k_MaxDirectionalLightsOnScreen = 16;

#if IS_DIRECTIONAL_LIGHT
const int k_MaxLights = k_MaxDirectionalLightsOnScreen;
#else
const int k_MaxLights = k_MaxPunctualLightsOnScreen;
#endif

struct Light
{
  vec4  color;  // [RGB, Intensity]
  vec3  direction;
  float inv_radius_pow2;
  vec3  position;
  float spot_scale;
  float spot_offset;
};

//
// Lighting Uniform Layout
//
layout(std140, set = 1, binding = 0) uniform u_Set1
{
  Light u_Lights[k_MaxLights];
  int   u_NumLights;
};

float pow2(float value);
float pow4(float value);
float pow5(float value);
float distributionGGX(float n_dot_h, float roughnesss);
float geometrySchlickGGX(float n_dot_v, float k, float one_minux_k);
float geometrySmith(float n_dot_l, float n_dot_v, float roughnesss);
vec3  fresnelSchlick(float cosTheta, vec3 F0);
vec3  mainLighting(vec3 surface_normal, vec3 albedo, float roughness, float one_minus_metallic, vec3 pixel_to_cam, vec3 light_dir, float attenuation, vec3 light_color, vec3 f0, float n_dot_v);
vec3  ambientLighting(vec3 albedo, float ao);
vec3  tonemapping(vec3 color);
vec3  gammaCorrection(vec3 color);

//
// calculates value^2.
//
float pow2(float value)
{
  return value * value;
}

//
// calculates value^4.
//
float pow4(float value)
{
  return pow2(pow2(value));
}

//
// calculates value^5 faster than pow(value, 5.0).
//
float pow5(float value)
{
  return pow4(value) * value;
}

float distributionGGX(float n_dot_h, float roughnesss)
{
  float roughness_pow4 = pow4(roughnesss);
  float n_dot_h_pow2   = pow2(n_dot_h);
  float denom          = n_dot_h_pow2 * (roughness_pow4 - 1.0) + 1.0;

  // Potential divide by 0 fix: for roughness @ 0.0 and n_dot_h @ 1.0.
  return roughness_pow4 / max(pow2(denom) * k_PI, k_Epsilon);
}

float geometrySchlickGGX(float n_dot_v, float k, float one_minux_k)
{
  return n_dot_v / (n_dot_v * one_minux_k + k);
}

float geometrySmith(float n_dot_l, float n_dot_v, float roughnesss)
{
  float k           = pow2(roughnesss + 1.0) / 8.0;
  float one_minux_k = 1.0 - k;

  return geometrySchlickGGX(n_dot_v, k, one_minux_k) * geometrySchlickGGX(n_dot_l, k, one_minux_k);
}

/*
  Full Formula:
    vec3 fresnelSchlick(float VoH, vec3 f0, float f90) 
    {
      return f0 + (vec3(f90) - f0) * pow(1.0 - VoH, 5.0);
    }

    We assume f90 is 1.0f since @ 90deg is the angle both
    dielectrics and conductors show specular reflectance.
*/
vec3 fresnelSchlick(float h_dot_v, vec3 f0)
{
  return f0 + (1.0f - f0) * pow5(1.0f - h_dot_v);
}

//
// light_dir       = (light_pos - world_pos) !DO NOT NORMALIZE!
// inv_radius_pow2 = (1.0 / radius)^2
//
float squaredFalloffAtten(vec3 light_dir, float inv_radius_pow2)
{
  float distance_sq = dot(light_dir, light_dir);
  float factor      = distance_sq * inv_radius_pow2;
  float smoothing   = max(1.0 - pow2(factor), 0.0);

  return pow2(smoothing) / max(distance_sq, k_Epsilon);
}

//
// L= light_dir = normalize(light_pos - world_pos)
//
// spot_scale  = 1.0 / max(cos(inner_angle) - cos(outer_angle), k_Epsilon);
// spot_offset = -cos(outer_angle) * spot_scale;
//
float spotAngleAtten(vec3 light_dir, vec3 spot_dir, float spot_scale, float spot_offset)
{
  float cd = dot(-light_dir, spot_dir);

  return pow2(clamp(cd * spot_scale + spot_offset, 0.0, 1.0));
}

//
// light_dir = (light_pos - world_pos) !DO NOT NORMALIZE!
//
vec3 calcRadiancePoint(vec4 light_color, vec3 light_dir, float inv_radius_pow2)
{
  return light_color.xyz * squaredFalloffAtten(light_dir, inv_radius_pow2) * light_color.w;
}

#if IS_DIRECTIONAL_LIGHT == 1

vec3 calcRadiance(Light light, vec3 light_dir)
{
  return light.color.xyz * light.color.w;
}

#elif IS_POINT_LIGHT == 1

vec3 calcRadiance(Light light, vec3 light_dir)
{
  return calcRadiancePoint(light.color, light_dir, light.inv_radius_pow2);
}

#elif IS_SPOT_LIGHT == 1

vec3 calcRadiance(Light light, vec3 light_dir)
{
  return calcRadiancePoint(light.color, light_dir, light.inv_radius_pow2) *
         spotAngleAtten(normalize(light_dir), light.direction, light.spot_scale, light.spot_offset);
}

#endif

//
// in:
//   N       = surface_normal (Expected to be normalized)
//   V       = pixel_to_cam   (Expected to be normalized)
//   L       = light_dir      (Expected to be normalized)
//   n_dot_v = max(dot(n, v), 0.0)
//
// notes:
//   H = half_vector
//
// returns L_out
//
vec3 mainLighting(vec3 radiance, vec3 surface_normal, vec3 albedo, float roughness, float one_minus_metallic, vec3 pixel_to_cam, vec3 light_dir, vec3 f0, float n_dot_v)
{
  // Initial Cached Calculations
  vec3  half_vector = normalize(pixel_to_cam + light_dir);
  float n_dot_h     = clamp(dot(surface_normal, half_vector), 0.0, 1.0);
  float n_dot_l     = max(dot(surface_normal, light_dir), 0.0);
  float h_dot_v     = clamp(dot(half_vector, pixel_to_cam), 0.0, 1.0);

  // Cook-Torrance BRDF
  float ndf     = distributionGGX(n_dot_h, roughness);
  float g       = geometrySmith(n_dot_l, n_dot_v, roughness);
  vec3  fresnel = fresnelSchlick(h_dot_v, f0);  // same as kSpecular.

  // Specular BRDF: Apply Main Light Formula
  vec3  numerator   = ndf * g * fresnel;
  float denominator = 4.0 * n_dot_v * n_dot_l;
  vec3  specular    = numerator / max(denominator, k_Epsilon);  // Potential divide by 0 fix: for n_dot_v @ 0.0 or n_dot_l @ 0.0.

  // Diffuse BRDF + Energy conservation
  vec3 kDiffuse = vec3(1.0) - fresnel;

  // Apply Smoothness
  kDiffuse *= one_minus_metallic;

  return (kDiffuse * albedo / k_PI + specular) * radiance * n_dot_l;
}

vec3 ambientLighting(vec3 albedo, float ao)
{
  return vec3(0.03) * albedo * ao;
}

vec3 tonemapping(vec3 color)
{
  return color / (color + vec3(1.0));
}

vec3 gammaCorrection(vec3 color)
{
  return pow(color, vec3(k_GammaCorrectionFactor));
}
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
