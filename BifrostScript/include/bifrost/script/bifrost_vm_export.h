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
#ifndef BIFROST_VM_EXPORT_H
#define BIFROST_VM_EXPORT_H

#if __cplusplus
extern "C" {
#endif

#if defined _WIN32 || defined __CYGWIN__
#ifdef BIFROST_VM_EXPORT
#ifdef __GNUC__
#define BIFROST_VM_API __attribute__((dllexport))
#else
#define BIFROST_VM_API __declspec(dllexport)
#endif
#elif defined(BIFROST_VM_EXPORT_STATIC)
#define BIFROST_VM_API
#else
#ifdef __GNUC__
#define BIFROST_VM_API __attribute__((dllimport))
#else
#define BIFROST_VM_API __declspec(dllimport)
#endif
#endif
#define NOT_BIFROST_VM_API
#else
#if __GNUC__ >= 4
#define BIFROST_VM_API __attribute__((visibility("default")))
#define BIFROST_VM_NOAPI __attribute__((visibility("hidden")))
#else
#define BIFROST_VM_API
#define BIFROST_VM_NOAPI
#endif
#endif

#if __cplusplus
}
#endif

#endif /* BIFROST_VM_EXPORT_H */
