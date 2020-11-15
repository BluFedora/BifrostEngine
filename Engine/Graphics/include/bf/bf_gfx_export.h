/******************************************************************************/
/*!
* @file   BF_gfx_export.h
* @author Shareef Raheem (http://blufedora.github.io)
*
* @brief
*  Contains the macros for exporting symbols for the shared library.
*
* @version 0.0.1-beta
* @date    2020-06-21
*
* @copyright Copyright (c) 2019-2020 Shareef Abdoul-Raheem
*/
/******************************************************************************/
#ifndef BF_GFX_EXPORT_H
#define BF_GFX_EXPORT_H

#if __cplusplus
extern "C" {
#endif

#if defined _WIN32 || defined __CYGWIN__
#ifdef BF_GFX_EXPORT

#ifdef __GNUC__
#define BF_GFX_API __attribute__((dllexport))
#else
#define BF_GFX_API __declspec(dllexport)
#endif

#elif defined(BF_GFX_EXPORT_STATIC)

#define BF_GFX_API

#else

#ifdef __GNUC__
#define BF_GFX_API __attribute__((dllimport))
#else
#define BF_GFX_API __declspec(dllimport)
#endif

#endif

#define BF_GFX_NOAPI

#else
#if __GNUC__ >= 4
#define BF_GFX_API __attribute__((visibility("default")))
#define BF_GFX_NOAPI __attribute__((visibility("hidden")))
#else
#define BF_GFX_API
#define BF_GFX_NOAPI
#endif
#endif

#if __cplusplus
}
#endif

#endif /* BF_GFX_EXPORT_H */
