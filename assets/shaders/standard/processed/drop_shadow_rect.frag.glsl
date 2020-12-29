// Reference: [http://madebyevan.com/shaders/fast-rounded-rectangle-shadows/]

#version 450

layout(location = 0) in vec2  frag_Pos;
layout(location = 1) in vec4  frag_Color;
layout(location = 2) in vec4  frag_Rect;
layout(location = 3) in float frag_Sigma;
layout(location = 4) in float frag_CornerRadius;

layout(location = 0) out vec4 o_FragColor0;

// Approximates the error function, needed for the gaussian integral.
vec4 erf(vec4 x) 
{
  vec4 s = sign(x), a = abs(x);
  x      = 1.0 + (0.278393 + (0.230389 + 0.078108 * (a * a)) * a) * a;
  x     *= x;

  return s - s / (x * x);
}

float boxShadow(vec2 lower, vec2 upper, vec2 point, float sigma) 
{
  // TODO(SR): Optimizations to make: sqrt(0.5) can be precalced.

  vec4 query    = vec4(point - lower, point - upper);
  vec4 integral = 0.5 + 0.5 * erf(query * (sqrt(0.5) / sigma));

  return (integral.z - integral.x) * (integral.w - integral.y);
}

void main() 
{
  float shadow_factor = boxShadow(frag_Rect.xy, frag_Rect.zw, frag_Pos, frag_Sigma);

  o_FragColor0 = vec4(frag_Color.rgb, shadow_factor * frag_Color.a);
}
