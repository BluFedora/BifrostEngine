//
// Author: Shareef Abdoul-Raheem
// Standard Shader GBuffer
//
// The Position and Normal Data Are In World Space.
// + Bone animation.
//
#version 450

#include "assets/shaders/standard/std_vertex_input.def.glsl"
layout(location = 4) in uint in_BoneIndices;
layout(location = 5) in vec4 in_BoneWeights;

#include "assets/shaders/standard/camera.ubo.glsl"
#include "assets/shaders/standard/object.ubo.glsl"

layout(location = 0) out vec3 frag_WorldNormal;
layout(location = 1) out vec3 frag_Color;
layout(location = 2) out vec2 frag_UV;

void main()
{
  uint bone_index0 = (in_BoneIndices >> 0)  & 0xFF;
  uint bone_index1 = (in_BoneIndices >> 8)  & 0xFF;
  uint bone_index2 = (in_BoneIndices >> 16) & 0xFF;
  uint bone_index3 = (in_BoneIndices >> 24) & 0xFF;
  
  mat4 bone_transform0 = u_Bones[bone_index0] * in_BoneWeights[0];
  mat4 bone_transform1 = u_Bones[bone_index1] * in_BoneWeights[1];
  mat4 bone_transform2 = u_Bones[bone_index2] * in_BoneWeights[2];
  mat4 bone_transform3 = u_Bones[bone_index3] * in_BoneWeights[3];
  
  mat4 final_bone_transform = bone_transform0 + bone_transform1 +
                              bone_transform2 + bone_transform3;
  
  vec4 object_position = final_bone_transform * vec4(in_Position.xyz, 1.0);
  vec4 clip_position   = u_ModelViewProjection * object_position;

  frag_WorldNormal = mat3(u_NormalModel) * in_Normal.xyz;
  frag_Color       = in_Color.rgb;
  frag_UV          = in_UV;

  gl_Position = clip_position;
}
