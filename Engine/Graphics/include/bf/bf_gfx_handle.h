/******************************************************************************/
/*!
 * @file   bf_gfx_handle.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Opaque Handle declarations.
 *
 * @version 0.0.1
 * @date    2020-03-22
 *
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
#ifndef BF_GFX_HANDLE_H
#define BF_GFX_HANDLE_H

#if __cplusplus
extern "C" {
#endif
#define BF_DECLARE_HANDLE(T)      \
  struct bf##T##_t;               \
  typedef struct bf##T##_t bf##T; \
  typedef bf##T*           bf##T##Handle

BF_DECLARE_HANDLE(GfxContext);
BF_DECLARE_HANDLE(GfxDevice);
BF_DECLARE_HANDLE(GfxCommandList);
BF_DECLARE_HANDLE(Buffer);
BF_DECLARE_HANDLE(VertexLayoutSet);
BF_DECLARE_HANDLE(DescriptorSet);
BF_DECLARE_HANDLE(Renderpass);
BF_DECLARE_HANDLE(ShaderModule);
BF_DECLARE_HANDLE(ShaderProgram);
BF_DECLARE_HANDLE(Texture);
BF_DECLARE_HANDLE(Framebuffer);
BF_DECLARE_HANDLE(Pipeline);
BF_DECLARE_HANDLE(WindowSurface);
typedef void* bfGfxBaseHandle;

#undef BF_DECLARE_HANDLE

/* clang-format off */

#define BF_DEFINE_GFX_HANDLE(T) struct bf##T##_t
#define BF_NULL_GFX_HANDLE      (void*)0

/* clang-format on */

#if __cplusplus
}
#endif

#endif /* BF_GFX_HANDLE_H */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2020 Shareef Abdoul-Raheem

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
