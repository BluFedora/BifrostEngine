//
// Author:          Shareef Abdoul-Raheem
// Standard Shader: GBuffer Generation
//
#version 450

layout(location = 0) in vec3 frag_WorldNormal;
layout(location = 1) in vec3 frag_Color;
layout(location = 2) in vec2 frag_UV;

layout(set = 2, binding = 0) uniform sampler2D u_AlbedoTexture;
layout(set = 2, binding = 1) uniform sampler2D u_NormalTexture;
layout(set = 2, binding = 2) uniform sampler2D u_MetallicTexture;
layout(set = 2, binding = 3) uniform sampler2D u_RoughnessTexture;
layout(set = 2, binding = 4) uniform sampler2D u_AmbientOcclusionTexture;

layout(location = 0) out vec4 o_FragColor0;
layout(location = 1) out vec4 o_FragColor1;


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
  float metallic          = texture(u_MetallicTexture, frag_UV).r;
  float roughness         = texture(u_RoughnessTexture, frag_UV).r;
  float ambient_occlusion = texture(u_AmbientOcclusionTexture, frag_UV).r;
  vec2  normal            = encodeNormal(normalize(frag_WorldNormal));  // TODO: Normal mapping
  vec3  albedo            = texture(u_AlbedoTexture, frag_UV).rgb * frag_Color;

  o_FragColor0 = vec4(normal, roughness, metallic);
  o_FragColor1 = vec4(albedo.xyz, ambient_occlusion);
}
