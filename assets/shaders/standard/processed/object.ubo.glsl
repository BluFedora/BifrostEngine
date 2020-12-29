
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
