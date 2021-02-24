//
// Author:          Shareef Abdoul-Raheem
// Standard Shader: Ambient Lighting
//
#version 450

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
  vec4  gsample1    = texture(u_GBufferRT1, frag_UV);
  vec4  ssao_sample = texture(u_SSAOBlurredBuffer, frag_UV);
  vec3  albedo      = gsample1.xyz;
  float ao          = gsample1.w * ssao_sample.r;
  vec3  ambient     = ambientLighting(albedo, ao);

  o_FragColor0 = vec4(ambient, 1.0f);
  // o_FragColor0 = vec4(vec3(0.0f), 1.0f);

  // Debug Outputs
  /*
  vec3 world_position = constructWorldPos(frag_UV);
  vec3 view_position  = (Camera_getViewMatrix() * vec4(world_position, 1.0f)).xyz;
  o_FragColor0        = vec4(vec3(ssao_sample.r), 1.0f);
  
  if (abs(view_position.z + 1000.0f) > 0.05f)
  {
    //o_FragColor0        = vec4(view_position, abs(view_position.z + 1000.0f));

  }
  else
  {
    o_FragColor0 = vec4(vec3(0.4f), abs(view_position.z + 1000.0f));
    discard;
  }
  //*/
}
