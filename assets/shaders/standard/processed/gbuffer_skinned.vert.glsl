//
// Author: Shareef Abdoul-Raheem
// Standard Shader GBuffer
//
// The Position and Normal Data Are In World Space.
// + Bone animation.
//
#version 450

layout(location = 0) in vec4 in_Position;
layout(location = 1) in vec4 in_Normal;
layout(location = 2) in vec4 in_Color;
layout(location = 3) in vec2 in_UV;
layout(location = 4) in uint in_BoneIndices;
layout(location = 5) in vec4 in_BoneWeights;


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
  float u_CameraAspect;
  vec3  u_CameraAmbient;
};

mat4 Camera_getViewMatrix()
{
  return u_CameraView;
}

#define k_MaxBones 128

//
// Object Transform UBO Layout
//
layout(std140, set = 3, binding = 0) uniform u_Set3Binding0
{
  mat4 u_ModelViewProjection;
  mat4 u_Model;
  mat4 u_NormalModel;
};

layout(std140, set = 3, binding = 1) uniform u_Set3Binding1
{
  mat4 u_Bones[k_MaxBones];
};

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
