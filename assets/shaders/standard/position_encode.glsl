//
// Bad Design Notes:
//   > Assumes Both Camera and u_DepthTexture uniforms...
//   > Assumes frag_ViewRay as one of the fragment inputs...
//

// Assumed depth range of [0.0 - 1.0]
// Depth needs to be '2.0f * depth - 1.0f' if the
// perspective expects [-1.0f - 1.0f] like in OpenGL.
float constructLinearDepth(vec2 uv)
{
  float depth        = texture(u_DepthTexture, uv).x;
  float linear_depth = u_CameraProjection[3][2] / (depth + u_CameraProjection[2][2]);

  return linear_depth;
}

#if SSAO
vec3 constructViewPos(vec2 uv)
{
  return frag_ViewRaySSAO * constructLinearDepth(uv);
}
#endif

vec3 constructWorldPos(vec2 uv)
{
  vec3  view_ray     = normalize(frag_ViewRay);
  float linear_depth = constructLinearDepth(uv);
  float view_z_dist  = dot(u_CameraForward, view_ray);

  return u_CameraPosition + view_ray * (linear_depth / view_z_dist);
}
