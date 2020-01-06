#ifndef mat4x4_h
#define mat4x4_h

#if _MSC_VER
#define ALIGN_STRUCT(n)   __declspec(align(n))
#else // Clang and GCC
#define ALIGN_STRUCT(n)   __attribute__((aligned(n)))
#endif

#define MATRIX_ROW_MAJOR 0
#define MATRIX_COL_MAJOR !MATRIX_ROW_MAJOR

#if defined(__SSE__)
#define MATRIX_SSE 1
#else
#define MATRIX_SSE 0
#endif

// #ifdef __arm__   // AArch32
// #ifdef __arm64__ // AArch64

#if __cplusplus
extern "C"
{
#endif
  #if MATRIX_ROW_MAJOR
    #define Mat4x4_get(self, x, y)      (&((self)->data[(x) + ((y) << 2)]))
#define Mat4x4_at(self, x, y) ((self)->data[(x) + ((y) << 2)])
  #else
    #define Mat4x4_get(self, x, y)      (&((self)->data[(y) + ((x) << 2)]))
    #define Mat4x4_at(self, x, y) ((self)->data[(y) + ((x) << 2)])
  #endif

  typedef struct Vec3f_t Vec3f;

  typedef struct /*ALIGN_STRUCT(16)*/ Mat4x4_t
  {
    float data[16];

  } /*ALIGN_STRUCT(16)*/ Mat4x4;

  // 'static' functions
  void    Mat4x4_identity(Mat4x4* self);
  void    Mat4x4_initTranslatef(Mat4x4* self, float x, float y, float z);
  void    Mat4x4_initScalef(Mat4x4* self, float x, float y, float z);
  void    Mat4x4_initRotationf(Mat4x4* self, float x, float y, float z);
  void    Mat4x4_ortho(Mat4x4* self, float left, float right, float bottom, float top, float near, float far); // TODO: I don't like the parameter order.
  void    Mat4x4_perspective(Mat4x4* self, float fovDeg, float aspect, float near, float far);
  void    Mat4x4_frustum(Mat4x4* const self, float left, float right, float bottom, float top, float znear, float zfar);
  void    Mat4x4_perspectiveInfinity(Mat4x4* const self, const float fovDeg, const float aspect, const float near);
  void    Mat4x4_initLookAt(Mat4x4* self, const Vec3f* position, const Vec3f* target, const Vec3f* inUp);

  void    Mat4x4_copy(const Mat4x4* self, Mat4x4* outCopy);
  void    Mat4x4_transpose(Mat4x4* self);
  int     Mat4x4_inverse(const Mat4x4* self, Mat4x4* outInverse);
  float   Mat4x4_det(const Mat4x4* self);
  float   Mat4x4_trace(const Mat4x4* self);
  //float*  Mat4x4_get(Mat4x4* self, const uint x, const uint y);
  //float   Mat4x4_constGet(const Mat4x4* self, const uint x, const uint y);

  //      The order is [self * other] which means other happens 'first'.
  void    Mat4x4_mult(const Mat4x4* self, const Mat4x4* other, Mat4x4* out);
  void    Mat4x4_multVec(const Mat4x4* self, const Vec3f *vec, Vec3f* outVec);
#if __cplusplus
}
#endif

#endif /* mat4x4_h */
