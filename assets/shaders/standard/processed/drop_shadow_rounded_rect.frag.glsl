// Reference: [http://madebyevan.com/shaders/fast-rounded-rectangle-shadows/]

#version 450

layout(location = 0) in vec2  frag_Pos;
layout(location = 1) in vec4  frag_Color;
layout(location = 2) in vec4  frag_Rect;
layout(location = 3) in float frag_Sigma;
layout(location = 4) in float frag_CornerRadius;

layout(location = 0) out vec4 o_FragColor0;

// Approximates the error function, needed for the gaussian integral.
vec2 erf(vec2 x)
{
  vec2 s = sign(x);
  vec2 a = abs(x);

  x = 1.0 + (0.278393 + (0.230389 + 0.078108 * (a * a)) * a) * a;
  x *= x;

  return s - s / (x * x);
}

float gaussian(float x, float sigma)
{
  // TODO(SR): Optimizations to make: 
  //   - sqrt(2.0 * pi) can be precalculated as a constant.

  const float pi = 3.141592653589793;

  return exp(-(x * x) / (2.0 * sigma * sigma)) / (sqrt(2.0 * pi) * sigma);
}

float roundedBoxShadowX(float x, float y, float sigma, float corner, vec2 half_size) 
{
  // TODO(SR): Optimizations to make: 
  //   - sqrt(0.5) can be precalculated as a constant.

  float delta    = min(half_size.y - corner - abs(y), 0.0);
  float curved   = half_size.x - corner + sqrt(max(0.0, corner * corner - delta * delta));
  vec2  integral = 0.5 + 0.5 * erf((x + vec2(-curved, curved)) * (sqrt(0.5) / sigma));

  return integral.y - integral.x;
}

float roundedBoxShadow(vec2 lower, vec2 upper, vec2 point, float sigma, float corner) 
{
  vec2 center    = (lower + upper) * 0.5;
  vec2 half_size = (upper - lower) * 0.5;

  point -= center;

  float low   = point.y - half_size.y;
  float high  = point.y + half_size.y;
  float start = clamp(-3.0 * sigma, low, high);
  float end   = clamp(3.0 * sigma, low, high);

  float step  = (end - start) / 4.0;
  float y     = start + step * 0.5;
  float value = 0.0;

  for (int i = 0; i < 4; i++) 
  {
    value += roundedBoxShadowX(point.x, point.y - y, sigma, corner, half_size) * gaussian(y, sigma) * step;
    
    y += step;
  }

  return value;
}

void main() 
{
  float shadow_factor = roundedBoxShadow(frag_Rect.xy, frag_Rect.zw, frag_Pos, frag_Sigma, frag_CornerRadius);

  o_FragColor0 = vec4(frag_Color.rgb, shadow_factor * frag_Color.a);
}
