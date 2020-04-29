
// [https://aras-p.info/texts/CompactNormalStorage.html]
// [https://mynameismjp.wordpress.com/2009/06/17/storing-normals-using-spherical-coordinates/]

const float k_SimplePI = 3.14159f;

vec2 encodeNormal(vec3 n)
{
  vec2 spherical;

  spherical.x = atan(n.y, n.x) / k_SimplePI;
  spherical.y = n.z;

  return spherical * 0.5f + 0.5f;
}

vec2 sincos(float x)
{
  return vec2(sin(x), cos(x));
}

vec3 decodeNormal(vec2 spherical)
{
  spherical = spherical * 2.0f - 1.0f;

  vec2 sin_cos_theta = sincos(spherical.x * k_SimplePI);
  vec2 sin_cos_phi   = vec2(sqrt(1.0 - spherical.y * spherical.y), spherical.y);

  return vec3(sin_cos_theta.y * sin_cos_phi.x, sin_cos_theta.x * sin_cos_phi.x, sin_cos_phi.y);
}
