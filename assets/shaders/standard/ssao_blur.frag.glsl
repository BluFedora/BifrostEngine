//
// Author: Shareef Abdoul-Raheem
// Standard Shader: SSAO Blur
//
// References:
//   [https://learnopengl.com/Advanced-Lighting/SSAO]
//
// Pair this module with "fullscreen_quad.vert.glsl"
// A basic box blur since we already have consistent noise.
//
#version 450

const int k_NoiseSize     = 4;
const int k_HalfNoiseSize = k_NoiseSize / 2;
const int k_NoiseSizePow2 = k_NoiseSize * k_NoiseSize;

layout(location = 0) in vec3 frag_ViewRay;
layout(location = 1) in vec2 frag_UV;

layout(set = 2, binding = 0) uniform sampler2D u_SSAOTexture;

layout(location = 0) out float o_FragColor0;

void main()
{
  vec2  noise_texel_size = 1.0 / vec2(/*textureSize(u_SSAOTexture, 0)*/ 1280.0, 720.0);
  float accumulation     = 0.0;

  for (int y = -k_HalfNoiseSize; y < k_HalfNoiseSize; ++y)
  {
    for (int x = -k_HalfNoiseSize; x < k_HalfNoiseSize; ++x)
    {
      accumulation += texture(u_SSAOTexture, frag_UV + vec2(float(x), float(y)) * noise_texel_size).r;
    }
  }

  o_FragColor0 = accumulation / float(k_NoiseSizePow2);
}
