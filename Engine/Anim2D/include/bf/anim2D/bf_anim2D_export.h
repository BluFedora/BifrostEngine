
#ifndef BF_SPRITE_ANIM_API_H
#define BF_SPRITE_ANIM_API_H

#if defined _WIN32 || defined __CYGWIN__
#ifdef BF_ANIM2D_EXPORT

#ifdef __GNUC__
#define BF_ANIM2D_API __attribute__((dllexport))
#else
#define BF_ANIM2D_API __declspec(dllexport)
#endif

#elif defined(BF_ANIM2D_EXPORT_STATIC)

#define BF_ANIM2D_API

#else

#ifdef __GNUC__
#define BF_ANIM2D_API __attribute__((dllimport))
#else
#define BF_ANIM2D_API __declspec(dllimport)
#endif

#endif

#define BF_ANIM2D_NOAPI

#else
#if __GNUC__ >= 4
#define BF_ANIM2D_API __attribute__((visibility("default")))
#define BF_ANIM2D_NOAPI __attribute__((visibility("hidden")))
#else
#define BF_ANIM2D_API
#define BF_ANIM2D_NOAPI
#endif
#endif

#ifdef __GNUC__
#define BF_CDECL __attribute__((__cdecl__))
#else
#define BF_CDECL __cdecl
#endif

#endif /* BF_SPRITE_ANIM_API_H */
