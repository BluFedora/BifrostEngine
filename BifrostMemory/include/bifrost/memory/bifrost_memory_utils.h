/******************************************************************************/
/*!
 * @file   bifrost_memory_utils.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief 
 *   Contains functions useful for low level memory manipulations.
 *
 * @version 0.0.1
 * @date    2020-03-22
 * 
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
#ifndef BIFROST_MEMORY_UTILS_HPP
#define BIFROST_MEMORY_UTILS_HPP

#include <stddef.h> /* size_t */

#if __cplusplus
extern "C" {
#endif
/*!
 * @brief 
 *   Aligns size to required_alignment.
 *
 * @param size
 *   The potentially unlaigned size of an object.
 * 
 * @param required_alignment 
 *   Must be a non zero power of two.
 *
 * @return size_t 
 *   The size of the object for the required alignment,
 */
size_t alignUpSize(size_t size, size_t required_alignment);
#if __cplusplus
}
#endif

#endif /* BIFROST_MEMORY_UTILS_HPP */
