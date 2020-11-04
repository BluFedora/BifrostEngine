/******************************************************************************/
/*!
* @file   bifrost_vm_export.h
* @author Shareef Raheem (http://blufedora.github.io)
* @par
*    Bifrost Scripting Language
*
* @brief
*  Contains the macros for exporting symbols for the shared library.
*
* @version 0.0.1-beta
* @date    2019-07-01
*
* @copyright Copyright (c) 2019-2020 Shareef Abdoul-Raheem
*/
/******************************************************************************/
#ifndef BF_VM_EXPORT_H
#define BF_VM_EXPORT_H

#if __cplusplus
extern "C" {
#endif

#if defined _WIN32 || defined __CYGWIN__
#ifdef BF_VM_EXPORT

#ifdef __GNUC__
#define BF_VM_API __attribute__((dllexport))
#else
#define BF_VM_API __declspec(dllexport)
#endif

#elif defined(BF_VM_EXPORT_STATIC)

#define BF_VM_API

#else
#ifdef __GNUC__
#define BF_VM_API __attribute__((dllimport))
#else
#define BF_VM_API __declspec(dllimport)
#endif
#endif

#define BIFROST_VM_NOAPI

#else
#if __GNUC__ >= 4
#define BF_VM_API __attribute__((visibility("default")))
#define BF_VM_APIAPI __attribute__((visibility("hidden")))
#else
#define BF_VM_API
#define BF_VM_APIAPI
#endif
#endif

#if __cplusplus
}
#endif

#endif /* BF_VM_EXPORT_H */
