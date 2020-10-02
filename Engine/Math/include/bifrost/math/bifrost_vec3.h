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

BF_MATH_API void  Vec3f_set(Vec3f* self, float x, float y, float z, float w);
BF_MATH_API void  Vec3f_copy(Vec3f* self, const Vec3f* other);
BF_MATH_API int   Vec3f_isEqual(const Vec3f* self, const Vec3f* other);
BF_MATH_API void  Vec3f_add(Vec3f* self, const Vec3f* other);
BF_MATH_API void  Vec3f_addScaled(Vec3f* self, const Vec3f* other, const float factor);
BF_MATH_API void  Vec3f_sub(Vec3f* self, const Vec3f* other);
BF_MATH_API void  Vec3f_mul(Vec3f* self, const float scalar);
BF_MATH_API void  Vec3f_multV(Vec3f* self, const Vec3f* other);  // Component wise mult
BF_MATH_API void  Vec3f_div(Vec3f* self, const float scalar);
BF_MATH_API float Vec3f_lenSq(const Vec3f* self);
BF_MATH_API float Vec3f_len(const Vec3f* self);
BF_MATH_API void  Vec3f_normalize(Vec3f* self);
BF_MATH_API float Vec3f_dot(const Vec3f* self, const Vec3f* other);
BF_MATH_API void  Vec3f_cross(const Vec3f* self, const Vec3f* other, Vec3f* output);
BF_MATH_API void  Vec3f_mulMat(Vec3f* self, const Mat4x4* matrix);
BF_MATH_API Color Vec3f_toColor(Vec3f self);

// Color API

static uint BIFROST_COLOR_EMPTY                = 0x00000000;
static uint BIFROST_COLOR_TRANSPARENT          = 0x00FFFFFF;
static uint BIFROST_COLOR_ALICEBLUE            = 0xFFFFF8F0;
static uint BIFROST_COLOR_ANTIQUEWHITE         = 0xFFD7EBFA;
static uint BIFROST_COLOR_AQUA                 = 0xFFFFFF00;
static uint BIFROST_COLOR_AQUAMARINE           = 0xFFD4FF7F;
static uint BIFROST_COLOR_AZURE                = 0xFFFFFFF0;
static uint BIFROST_COLOR_BEIGE                = 0xFFDCF5F5;
static uint BIFROST_COLOR_BISQUE               = 0xFFC4E4FF;
static uint BIFROST_COLOR_BLACK                = 0xFF000000;
static uint BIFROST_COLOR_BLANCHEDALMOND       = 0xFFCDEBFF;
static uint BIFROST_COLOR_BLUE                 = 0xFFFF0000;
static uint BIFROST_COLOR_BLUEVIOLET           = 0xFFE22B8A;
static uint BIFROST_COLOR_BROWN                = 0xFF2A2AA5;
static uint BIFROST_COLOR_BURLYWOOD            = 0xFF87B8DE;
static uint BIFROST_COLOR_CADETBLUE            = 0xFFA09E5F;
static uint BIFROST_COLOR_CHARTREUSE           = 0xFF00FF7F;
static uint BIFROST_COLOR_CHOCOLATE            = 0xFF1E69D2;
static uint BIFROST_COLOR_CORAL                = 0xFF507FFF;
static uint BIFROST_COLOR_CORNFLOWERBLUE       = 0xFFED9564;
static uint BIFROST_COLOR_CORNSILK             = 0xFFDCF8FF;
static uint BIFROST_COLOR_CRIMSON              = 0xFF3C14DC;
static uint BIFROST_COLOR_CYAN                 = 0xFFFFFF00;
static uint BIFROST_COLOR_DARKBLUE             = 0xFF8B0000;
static uint BIFROST_COLOR_DARKCYAN             = 0xFF8B8B00;
static uint BIFROST_COLOR_DARKGOLDENROD        = 0xFF0B86B8;
static uint BIFROST_COLOR_DARKGRAY             = 0xFFA9A9A9;
static uint BIFROST_COLOR_DARKGREEN            = 0xFF006400;
static uint BIFROST_COLOR_DARKKHAKI            = 0xFF6BB7BD;
static uint BIFROST_COLOR_DARKMAGENTA          = 0xFF8B008B;
static uint BIFROST_COLOR_DARKOLIVEGREEN       = 0xFF2F6B55;
static uint BIFROST_COLOR_DARKORANGE           = 0xFF008CFF;
static uint BIFROST_COLOR_DARKORCHID           = 0xFFCC3299;
static uint BIFROST_COLOR_DARKRED              = 0xFF00008B;
static uint BIFROST_COLOR_DARKSALMON           = 0xFF7A96E9;
static uint BIFROST_COLOR_DARKSEAGREEN         = 0xFF8BBC8F;
static uint BIFROST_COLOR_DARKSLATEBLUE        = 0xFF8B3D48;
static uint BIFROST_COLOR_DARKSLATEGRAY        = 0xFF4F4F2F;
static uint BIFROST_COLOR_DARKTURQUOISE        = 0xFFD1CE00;
static uint BIFROST_COLOR_DARKVIOLET           = 0xFFD30094;
static uint BIFROST_COLOR_DEEPPINK             = 0xFF9314FF;
static uint BIFROST_COLOR_DEEPSKYBLUE          = 0xFFFFBF00;
static uint BIFROST_COLOR_DIMGRAY              = 0xFF696969;
static uint BIFROST_COLOR_DODGERBLUE           = 0xFFFF901E;
static uint BIFROST_COLOR_FIREBRICK            = 0xFF2222B2;
static uint BIFROST_COLOR_FLORALWHITE          = 0xFFF0FAFF;
static uint BIFROST_COLOR_FORESTGREEN          = 0xFF228B22;
static uint BIFROST_COLOR_FUCHSIA              = 0xFFFF00FF;
static uint BIFROST_COLOR_GAINSBORO            = 0xFFDCDCDC;
static uint BIFROST_COLOR_GHOSTWHITE           = 0xFFFFF8F8;
static uint BIFROST_COLOR_GOLD                 = 0xFF00D7FF;
static uint BIFROST_COLOR_GOLDENROD            = 0xFF20A5DA;
static uint BIFROST_COLOR_GRAY                 = 0xFF808080;
static uint BIFROST_COLOR_GREEN                = 0xFF008000;
static uint BIFROST_COLOR_GREENYELLOW          = 0xFF2FFFAD;
static uint BIFROST_COLOR_HONEYDEW             = 0xFFF0FFF0;
static uint BIFROST_COLOR_HOTPINK              = 0xFFB469FF;
static uint BIFROST_COLOR_INDIANRED            = 0xFF5C5CCD;
static uint BIFROST_COLOR_INDIGO               = 0xFF82004B;
static uint BIFROST_COLOR_IVORY                = 0xFFF0FFFF;
static uint BIFROST_COLOR_KHAKI                = 0xFF8CE6F0;
static uint BIFROST_COLOR_LAVENDER             = 0xFFFAE6E6;
static uint BIFROST_COLOR_LAVENDERBLUSH        = 0xFFF5F0FF;
static uint BIFROST_COLOR_LAWNGREEN            = 0xFF00FC7C;
static uint BIFROST_COLOR_LEMONCHIFFON         = 0xFFCDFAFF;
static uint BIFROST_COLOR_LIGHTBLUE            = 0xFFE6D8AD;
static uint BIFROST_COLOR_LIGHTCORAL           = 0xFF8080F0;
static uint BIFROST_COLOR_LIGHTCYAN            = 0xFFFFFFE0;
static uint BIFROST_COLOR_LIGHTGOLDENRODYELLOW = 0xFFD2FAFA;
static uint BIFROST_COLOR_LIGHTGRAY            = 0xFFD3D3D3;
static uint BIFROST_COLOR_LIGHTGREEN           = 0xFF90EE90;
static uint BIFROST_COLOR_LIGHTPINK            = 0xFFC1B6FF;
static uint BIFROST_COLOR_LIGHTSALMON          = 0xFF7AA0FF;
static uint BIFROST_COLOR_LIGHTSEAGREEN        = 0xFFAAB220;
static uint BIFROST_COLOR_LIGHTSKYBLUE         = 0xFFFACE87;
static uint BIFROST_COLOR_LIGHTSLATEGRAY       = 0xFF998877;
static uint BIFROST_COLOR_LIGHTSTEELBLUE       = 0xFFDEC4B0;
static uint BIFROST_COLOR_LIGHTYELLOW          = 0xFFE0FFFF;
static uint BIFROST_COLOR_LIME                 = 0xFF00FF00;
static uint BIFROST_COLOR_LIMEGREEN            = 0xFF32CD32;
static uint BIFROST_COLOR_LINEN                = 0xFFE6F0FA;
static uint BIFROST_COLOR_MAGENTA              = 0xFFFF00FF;
static uint BIFROST_COLOR_MAROON               = 0xFF000080;
static uint BIFROST_COLOR_MEDIUMAQUAMARINE     = 0xFFAACD66;
static uint BIFROST_COLOR_MEDIUMBLUE           = 0xFFCD0000;
static uint BIFROST_COLOR_MEDIUMORCHID         = 0xFFD355BA;
static uint BIFROST_COLOR_MEDIUMPURPLE         = 0xFFDB7093;
static uint BIFROST_COLOR_MEDIUMSEAGREEN       = 0xFF71B33C;
static uint BIFROST_COLOR_MEDIUMSLATEBLUE      = 0xFFEE687B;
static uint BIFROST_COLOR_MEDIUMSPRINGGREEN    = 0xFF9AFA00;
static uint BIFROST_COLOR_MEDIUMTURQUOISE      = 0xFFCCD148;
static uint BIFROST_COLOR_MEDIUMVIOLETRED      = 0xFF8515C7;
static uint BIFROST_COLOR_MIDNIGHTBLUE         = 0xFF701919;
static uint BIFROST_COLOR_MINTCREAM            = 0xFFFAFFF5;
static uint BIFROST_COLOR_MISTYROSE            = 0xFFE1E4FF;
static uint BIFROST_COLOR_MOCCASIN             = 0xFFB5E4FF;
static uint BIFROST_COLOR_NAVAJOWHITE          = 0xFFADDEFF;
static uint BIFROST_COLOR_NAVY                 = 0xFF800000;
static uint BIFROST_COLOR_OLDLACE              = 0xFFE6F5FD;
static uint BIFROST_COLOR_OLIVE                = 0xFF008080;
static uint BIFROST_COLOR_OLIVEDRAB            = 0xFF238E6B;
static uint BIFROST_COLOR_ORANGE               = 0xFF00A5FF;
static uint BIFROST_COLOR_ORANGERED            = 0xFF0045FF;
static uint BIFROST_COLOR_ORCHID               = 0xFFD670DA;
static uint BIFROST_COLOR_PALEGOLDENROD        = 0xFFAAE8EE;
static uint BIFROST_COLOR_PALEGREEN            = 0xFF98FB98;
static uint BIFROST_COLOR_PALETURQUOISE        = 0xFFEEEEAF;
static uint BIFROST_COLOR_PALEVIOLETRED        = 0xFF9370DB;
static uint BIFROST_COLOR_PAPAYAWHIP           = 0xFFD5EFFF;
static uint BIFROST_COLOR_PEACHPUFF            = 0xFFB9DAFF;
static uint BIFROST_COLOR_PERU                 = 0xFF3F85CD;
static uint BIFROST_COLOR_PINK                 = 0xFFCBC0FF;
static uint BIFROST_COLOR_PLUM                 = 0xFFDDA0DD;
static uint BIFROST_COLOR_POWDERBLUE           = 0xFFE6E0B0;
static uint BIFROST_COLOR_PURPLE               = 0xFF800080;
static uint BIFROST_COLOR_RED                  = 0xFF0000FF;
static uint BIFROST_COLOR_ROSYBROWN            = 0xFF8F8FBC;
static uint BIFROST_COLOR_ROYALBLUE            = 0xFFE16941;
static uint BIFROST_COLOR_SADDLEBROWN          = 0xFF13458B;
static uint BIFROST_COLOR_SALMON               = 0xFF7280FA;
static uint BIFROST_COLOR_SANDYBROWN           = 0xFF60A4F4;
static uint BIFROST_COLOR_SEAGREEN             = 0xFF578B2E;
static uint BIFROST_COLOR_SEASHELL             = 0xFFEEF5FF;
static uint BIFROST_COLOR_SIENNA               = 0xFF2D52A0;
static uint BIFROST_COLOR_SILVER               = 0xFFC0C0C0;
static uint BIFROST_COLOR_SKYBLUE              = 0xFFEBCE87;
static uint BIFROST_COLOR_SLATEBLUE            = 0xFFCD5A6A;
static uint BIFROST_COLOR_SLATEGRAY            = 0xFF908070;
static uint BIFROST_COLOR_SNOW                 = 0xFFFAFAFF;
static uint BIFROST_COLOR_SPRINGGREEN          = 0xFF7FFF00;
static uint BIFROST_COLOR_STEELBLUE            = 0xFFB48246;
static uint BIFROST_COLOR_TAN                  = 0xFF8CB4D2;
static uint BIFROST_COLOR_TEAL                 = 0xFF808000;
static uint BIFROST_COLOR_THISTLE              = 0xFFD8BFD8;
static uint BIFROST_COLOR_TOMATO               = 0xFF4763FF;
static uint BIFROST_COLOR_TURQUOISE            = 0xFFD0E040;
static uint BIFROST_COLOR_VIOLET               = 0xFFEE82EE;
static uint BIFROST_COLOR_WHEAT                = 0xFFB3DEF5;
static uint BIFROST_COLOR_WHITE                = 0xFFFFFFFF;
static uint BIFROST_COLOR_WHITESMOKE           = 0xFFF5F5F5;
static uint BIFROST_COLOR_YELLOW               = 0xFF00FFFF;
static uint BIFROST_COLOR_YELLOWGREEN          = 0xFF32CD9A;

BF_MATH_API uchar Color_r(Color self);
BF_MATH_API uchar Color_g(Color self);
BF_MATH_API uchar Color_b(Color self);
BF_MATH_API uchar Color_a(Color self);
BF_MATH_API void  Color_setRGBA(Color* self, uchar r, uchar g, uchar b, uchar a);
BF_MATH_API void  Color_setR(Color* self, uint r);
BF_MATH_API void  Color_setG(Color* self, uint g);
BF_MATH_API void  Color_setB(Color* self, uint b);
BF_MATH_API void  Color_setA(Color* self, uint a);

#if __cplusplus
}
#endif

#endif /* vec3f_h */
