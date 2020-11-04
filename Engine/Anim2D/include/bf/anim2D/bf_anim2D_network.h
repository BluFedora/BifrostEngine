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
