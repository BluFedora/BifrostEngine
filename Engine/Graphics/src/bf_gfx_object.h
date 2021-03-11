/******************************************************************************/
/*!
 * @file   bf_gfx_object.h
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   Shared base interface for all graphics objects.
 *
 * @version 0.0.1
 * @date    2021-03-10
 *
 * @copyright Copyright (c) 2021
 */
/******************************************************************************/
#ifndef BF_GFX_OBJECT_H
#define BF_GFX_OBJECT_H

#include "bf/bf_gfx_api.h" // bfFrameCount_t

#include <stdint.h> // uint64_t

#if __cplusplus
extern "C" {
#endif

#define bfFrameCountMax 0xFFFFFFFF
typedef uint32_t bfFrameCount_t;

typedef enum bfGfxObjectType
{
  BF_GFX_OBJECT_BUFFER         = 0,
  BF_GFX_OBJECT_RENDERPASS     = 1,
  BF_GFX_OBJECT_SHADER_MODULE  = 2,
  BF_GFX_OBJECT_SHADER_PROGRAM = 3,
  BF_GFX_OBJECT_DESCRIPTOR_SET = 4,
  BF_GFX_OBJECT_TEXTURE        = 5,
  BF_GFX_OBJECT_FRAMEBUFFER    = 6,
  BF_GFX_OBJECT_PIPELINE       = 7,

} bfGfxObjectType;  // 3 bits worth of data.

typedef struct bfBaseGfxObject bfBaseGfxObject;
struct bfBaseGfxObject
{
  bfGfxObjectType  type;
  bfBaseGfxID      id;
  bfBaseGfxObject* next;
  uint64_t         hash_code;
  bfFrameCount_t   last_frame_used;
};

void bfBaseGfxObject_ctor(bfBaseGfxObject* self, bfGfxObjectType type);

typedef struct bfBaseGfxObjectStore
{
  bfBaseGfxObject** object_map;
  bfBaseGfxID*      id_map;

} bfBaseGfxObjectStore;

#if __cplusplus
}
#endif

#endif /* BF_GFX_OBJECT_H */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2021 Shareef Abdoul-Raheem

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
/******************************************************************************/