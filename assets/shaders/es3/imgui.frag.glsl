uniform sampler2D u_Texture;

/*layout(location = 0)*/ in vec4 frag_Color;
/*layout(location = 1)*/ in vec2 frag_UV;

layout(location = 0) out vec4 o_FragColor0;

void main() 
{
  o_FragColor0 = texture(u_Texture, frag_UV) * frag_Color;
}
