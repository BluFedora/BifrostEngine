/******************************************************************************/
/*!
 * @file   bf_anim2d_network.hpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   Some helpers for interacting with SRSA'a TCP server.
 *
 * @version 0.0.1
 * @date    2021-03-09
 *
 * @copyright Copyright (c) 2021
 */
/******************************************************************************/
#ifndef BF_ANIM2D_NETWORK_H
#define BF_ANIM2D_NETWORK_H

#include <stdint.h> /* uint32_t */

#if __cplusplus
extern "C" {
#endif

enum
{
  bfAnim2DPacketType_SpritesheetChanged = 0,
  bfAnim2DPacketType_TextureChanged     = 1,

  k_bfAnim2DHeaderSize      = 5u,
  k_bfAnim2DGUIDSize        = 37u,
  k_bfAnim2DTotalHeaderSize = k_bfAnim2DHeaderSize + k_bfAnim2DGUIDSize,
};

typedef struct
{
  uint32_t packet_size; /*!< Includes the header (bfAnim2DPacketHeader) size. */
  uint8_t  packet_type; /*!< bfAnim2DPacketType_* */
  char     spritesheet_uuid[k_bfAnim2DGUIDSize];

} bfAnim2DPacketHeader;

bfAnim2DPacketHeader bfAnim2DPacketHeader_read(const char* bytes);

typedef struct
{
  bfAnim2DPacketHeader header;
  uint32_t             atlas_data_size;
  const uint8_t*       atlas_data;

} bfAnim2DPacketSpritesheetChanged;

bfAnim2DPacketSpritesheetChanged bfAnim2DPacketSpritesheetChanged_read(const char* bytes);

typedef struct
{
  bfAnim2DPacketHeader header;
  uint32_t             texture_data_size;
  const char*          texture_data;

} bfAnim2DPacketTextureChanged;

bfAnim2DPacketTextureChanged bfAnim2DPacketTextureChanged_read(const char* bytes);

#if __cplusplus
}
#endif

#endif /* BF_ANIM2D_NETWORK_H */

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
