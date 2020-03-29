
//
// Object Transform UBO Layout
//
layout(std140, set = 3, binding = 0) uniform u_Set3
{
  mat4 u_ModelViewProjection;
  mat4 u_Model;
  mat4 u_NormalModel;
};
