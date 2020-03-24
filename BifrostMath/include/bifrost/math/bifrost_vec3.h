#ifndef vec3f_h
#define vec3f_h

#include "bifrost_math_export.h"

#if _MSC_VER
#define ALIGN_STRUCT(n) __declspec(align(n))
#else  // Clang and GCC
#define ALIGN_STRUCT(n) __attribute__((aligned(n)))
#endif

#if __cplusplus
extern "C" {
#endif

/*
    SSE CHECKING CODE

    TODO: Abstract away Microsoft's trash way of doing things so that it 
    follows more traditional methods.

    // Microsoft
    #ifdef __AVX2__
    //AVX2
    #elif defined ( __AVX__ )
    //AVX
    #elif (defined(_M_AMD64) || defined(_M_X64))
    //SSE2 x64
    #elif _M_IX86_FP == 2
    //SSE2 x32
    #elif _M_IX86_FP == 1
    //SSE x32
    #endif

      // On Every Compiler Except Microsoft
    #ifdef __SSE__
    #ifdef __SSE2__
    #ifdef __SSE3__
    #ifdef __SSE4_1__
    #ifdef __AVX__
  */

#if !defined(VECTOR_SSE)
#if defined(__SSE__)
#define VECTOR_SSE 0
#else
#define VECTOR_SSE 0
#endif
#endif

#if defined(__ARM_NEON__)
#define VECTOR_NEON 1
#else
#define VECTOR_NEON 0
#endif

typedef unsigned int  Color;
typedef unsigned int  uint;
typedef unsigned char uchar;

typedef struct Mat4x4_t Mat4x4;

typedef struct /*ALIGN_STRUCT(16)*/ Vec3f_t
{
  float x, y, z, w;

} /*ALIGN_STRUCT(16)*/ Vec3f;

typedef struct Rectf_t
{
  float min[2];
  float max[2];

} Rectf;

typedef struct Recti_t
{
  int min[2];
  int max[2];

} Recti;

BIFROST_MATH_API void  Vec3f_set(Vec3f* self, float x, float y, float z, float w);
BIFROST_MATH_API void  Vec3f_copy(Vec3f* self, const Vec3f* other);
BIFROST_MATH_API int   Vec3f_isEqual(const Vec3f* self, const Vec3f* other);
BIFROST_MATH_API void  Vec3f_add(Vec3f* self, const Vec3f* other);
BIFROST_MATH_API void  Vec3f_addScaled(Vec3f* self, const Vec3f* other, const float factor);
BIFROST_MATH_API void  Vec3f_sub(Vec3f* self, const Vec3f* other);
BIFROST_MATH_API void  Vec3f_mul(Vec3f* self, const float scalar);
BIFROST_MATH_API void  Vec3f_multV(Vec3f* self, const Vec3f* other); // Component wise mult
BIFROST_MATH_API void  Vec3f_div(Vec3f* self, const float scalar);
BIFROST_MATH_API float Vec3f_lenSq(const Vec3f* self);
BIFROST_MATH_API float Vec3f_len(const Vec3f* self);
BIFROST_MATH_API void  Vec3f_normalize(Vec3f* self);
BIFROST_MATH_API float Vec3f_dot(const Vec3f* self, const Vec3f* other);
BIFROST_MATH_API void  Vec3f_cross(const Vec3f* self, const Vec3f* other, Vec3f* output);
BIFROST_MATH_API void  Vec3f_mulMat(Vec3f* self, const Mat4x4* matrix);
BIFROST_MATH_API Color Vec3f_toColor(const Vec3f* self);

// Color API
enum
{
  COLOR_EMPTY                = 0x00000000,
  COLOR_TRANSPARENT          = 0x00FFFFFF,
  COLOR_ALICEBLUE            = 0xFFFFF8F0,
  COLOR_ANTIQUEWHITE         = 0xFFD7EBFA,
  COLOR_AQUA                 = 0xFFFFFF00,
  COLOR_AQUAMARINE           = 0xFFD4FF7F,
  COLOR_AZURE                = 0xFFFFFFF0,
  COLOR_BEIGE                = 0xFFDCF5F5,
  COLOR_BISQUE               = 0xFFC4E4FF,
  COLOR_BLACK                = 0x00000000,
  COLOR_BLANCHEDALMOND       = 0xFFCDEBFF,
  COLOR_BLUE                 = 0xFFFF0000,
  COLOR_BLUEVIOLET           = 0xFFE22B8A,
  COLOR_BROWN                = 0xFF2A2AA5,
  COLOR_BURLYWOOD            = 0xFF87B8DE,
  COLOR_CADETBLUE            = 0xFFA09E5F,
  COLOR_CHARTREUSE           = 0xFF00FF7F,
  COLOR_CHOCOLATE            = 0xFF1E69D2,
  COLOR_CORAL                = 0xFF507FFF,
  COLOR_CORNFLOWERBLUE       = 0xFFED9564,
  COLOR_CORNSILK             = 0xFFDCF8FF,
  COLOR_CRIMSON              = 0xFF3C14DC,
  COLOR_CYAN                 = 0xFFFFFF00,
  COLOR_DARKBLUE             = 0xFF8B0000,
  COLOR_DARKCYAN             = 0xFF8B8B00,
  COLOR_DARKGOLDENROD        = 0xFF0B86B8,
  COLOR_DARKGRAY             = 0xFFA9A9A9,
  COLOR_DARKGREEN            = 0xFF006400,
  COLOR_DARKKHAKI            = 0xFF6BB7BD,
  COLOR_DARKMAGENTA          = 0xFF8B008B,
  COLOR_DARKOLIVEGREEN       = 0xFF2F6B55,
  COLOR_DARKORANGE           = 0xFF008CFF,
  COLOR_DARKORCHID           = 0xFFCC3299,
  COLOR_DARKRED              = 0xFF00008B,
  COLOR_DARKSALMON           = 0xFF7A96E9,
  COLOR_DARKSEAGREEN         = 0xFF8BBC8F,
  COLOR_DARKSLATEBLUE        = 0xFF8B3D48,
  COLOR_DARKSLATEGRAY        = 0xFF4F4F2F,
  COLOR_DARKTURQUOISE        = 0xFFD1CE00,
  COLOR_DARKVIOLET           = 0xFFD30094,
  COLOR_DEEPPINK             = 0xFF9314FF,
  COLOR_DEEPSKYBLUE          = 0xFFFFBF00,
  COLOR_DIMGRAY              = 0xFF696969,
  COLOR_DODGERBLUE           = 0xFFFF901E,
  COLOR_FIREBRICK            = 0xFF2222B2,
  COLOR_FLORALWHITE          = 0xFFF0FAFF,
  COLOR_FORESTGREEN          = 0xFF228B22,
  COLOR_FUCHSIA              = 0xFFFF00FF,
  COLOR_GAINSBORO            = 0xFFDCDCDC,
  COLOR_GHOSTWHITE           = 0xFFFFF8F8,
  COLOR_GOLD                 = 0xFF00D7FF,
  COLOR_GOLDENROD            = 0xFF20A5DA,
  COLOR_GRAY                 = 0xFF808080,
  COLOR_GREEN                = 0xFF008000,
  COLOR_GREENYELLOW          = 0xFF2FFFAD,
  COLOR_HONEYDEW             = 0xFFF0FFF0,
  COLOR_HOTPINK              = 0xFFB469FF,
  COLOR_INDIANRED            = 0xFF5C5CCD,
  COLOR_INDIGO               = 0xFF82004B,
  COLOR_IVORY                = 0xFFF0FFFF,
  COLOR_KHAKI                = 0xFF8CE6F0,
  COLOR_LAVENDER             = 0xFFFAE6E6,
  COLOR_LAVENDERBLUSH        = 0xFFF5F0FF,
  COLOR_LAWNGREEN            = 0xFF00FC7C,
  COLOR_LEMONCHIFFON         = 0xFFCDFAFF,
  COLOR_LIGHTBLUE            = 0xFFE6D8AD,
  COLOR_LIGHTCORAL           = 0xFF8080F0,
  COLOR_LIGHTCYAN            = 0xFFFFFFE0,
  COLOR_LIGHTGOLDENRODYELLOW = 0xFFD2FAFA,
  COLOR_LIGHTGRAY            = 0xFFD3D3D3,
  COLOR_LIGHTGREEN           = 0xFF90EE90,
  COLOR_LIGHTPINK            = 0xFFC1B6FF,
  COLOR_LIGHTSALMON          = 0xFF7AA0FF,
  COLOR_LIGHTSEAGREEN        = 0xFFAAB220,
  COLOR_LIGHTSKYBLUE         = 0xFFFACE87,
  COLOR_LIGHTSLATEGRAY       = 0xFF998877,
  COLOR_LIGHTSTEELBLUE       = 0xFFDEC4B0,
  COLOR_LIGHTYELLOW          = 0xFFE0FFFF,
  COLOR_LIME                 = 0xFF00FF00,
  COLOR_LIMEGREEN            = 0xFF32CD32,
  COLOR_LINEN                = 0xFFE6F0FA,
  COLOR_MAGENTA              = 0xFFFF00FF,
  COLOR_MAROON               = 0xFF000080,
  COLOR_MEDIUMAQUAMARINE     = 0xFFAACD66,
  COLOR_MEDIUMBLUE           = 0xFFCD0000,
  COLOR_MEDIUMORCHID         = 0xFFD355BA,
  COLOR_MEDIUMPURPLE         = 0xFFDB7093,
  COLOR_MEDIUMSEAGREEN       = 0xFF71B33C,
  COLOR_MEDIUMSLATEBLUE      = 0xFFEE687B,
  COLOR_MEDIUMSPRINGGREEN    = 0xFF9AFA00,
  COLOR_MEDIUMTURQUOISE      = 0xFFCCD148,
  COLOR_MEDIUMVIOLETRED      = 0xFF8515C7,
  COLOR_MIDNIGHTBLUE         = 0xFF701919,
  COLOR_MINTCREAM            = 0xFFFAFFF5,
  COLOR_MISTYROSE            = 0xFFE1E4FF,
  COLOR_MOCCASIN             = 0xFFB5E4FF,
  COLOR_NAVAJOWHITE          = 0xFFADDEFF,
  COLOR_NAVY                 = 0xFF800000,
  COLOR_OLDLACE              = 0xFFE6F5FD,
  COLOR_OLIVE                = 0xFF008080,
  COLOR_OLIVEDRAB            = 0xFF238E6B,
  COLOR_ORANGE               = 0xFF00A5FF,
  COLOR_ORANGERED            = 0xFF0045FF,
  COLOR_ORCHID               = 0xFFD670DA,
  COLOR_PALEGOLDENROD        = 0xFFAAE8EE,
  COLOR_PALEGREEN            = 0xFF98FB98,
  COLOR_PALETURQUOISE        = 0xFFEEEEAF,
  COLOR_PALEVIOLETRED        = 0xFF9370DB,
  COLOR_PAPAYAWHIP           = 0xFFD5EFFF,
  COLOR_PEACHPUFF            = 0xFFB9DAFF,
  COLOR_PERU                 = 0xFF3F85CD,
  COLOR_PINK                 = 0xFFCBC0FF,
  COLOR_PLUM                 = 0xFFDDA0DD,
  COLOR_POWDERBLUE           = 0xFFE6E0B0,
  COLOR_PURPLE               = 0xFF800080,
  COLOR_RED                  = 0xFF0000FF,
  COLOR_ROSYBROWN            = 0xFF8F8FBC,
  COLOR_ROYALBLUE            = 0xFFE16941,
  COLOR_SADDLEBROWN          = 0xFF13458B,
  COLOR_SALMON               = 0xFF7280FA,
  COLOR_SANDYBROWN           = 0xFF60A4F4,
  COLOR_SEAGREEN             = 0xFF578B2E,
  COLOR_SEASHELL             = 0xFFEEF5FF,
  COLOR_SIENNA               = 0xFF2D52A0,
  COLOR_SILVER               = 0xFFC0C0C0,
  COLOR_SKYBLUE              = 0xFFEBCE87,
  COLOR_SLATEBLUE            = 0xFFCD5A6A,
  COLOR_SLATEGRAY            = 0xFF908070,
  COLOR_SNOW                 = 0xFFFAFAFF,
  COLOR_SPRINGGREEN          = 0xFF7FFF00,
  COLOR_STEELBLUE            = 0xFFB48246,
  COLOR_TAN                  = 0xFF8CB4D2,
  COLOR_TEAL                 = 0xFF808000,
  COLOR_THISTLE              = 0xFFD8BFD8,
  COLOR_TOMATO               = 0xFF4763FF,
  COLOR_TURQUOISE            = 0xFFD0E040,
  COLOR_VIOLET               = 0xFFEE82EE,
  COLOR_WHEAT                = 0xFFB3DEF5,
  COLOR_WHITE                = 0xFFFFFFFF,
  COLOR_WHITESMOKE           = 0xFFF5F5F5,
  COLOR_YELLOW               = 0xFF00FFFF,
  COLOR_YELLOWGREEN          = 0xFF32CD9A
};

BIFROST_MATH_API uchar Color_r(Color self);
BIFROST_MATH_API uchar Color_g(Color self);
BIFROST_MATH_API uchar Color_b(Color self);
BIFROST_MATH_API uchar Color_a(Color self);
BIFROST_MATH_API void  Color_setRGBA(Color* self, uchar r, uchar g, uchar b, uchar a);
BIFROST_MATH_API void  Color_setR(Color* self, uint r);
BIFROST_MATH_API void  Color_setG(Color* self, uint g);
BIFROST_MATH_API void  Color_setB(Color* self, uint b);
BIFROST_MATH_API void  Color_setA(Color* self, uint a);

#if __cplusplus
}
#endif

#endif /* vec3f_h */
