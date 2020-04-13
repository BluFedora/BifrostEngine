//
// Author:       Shareef Abdoul-Raheem
// Debug Shader: Overlay Lines
//
#version 450

layout(location = 0) in vec3 frag_Color;

layout(location = 0) out vec4 o_FragColor0;

void main()
{
  o_FragColor0 = vec4(frag_Color, 1.0f);
}
