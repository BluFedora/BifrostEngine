#ifndef BIFROST_GFX_TYPES_H
#define BIFROST_GFX_TYPES_H

#if __cplusplus
extern "C"
{
#endif
typedef enum BifrostIndexType_t
{
  BIFROST_INDEX_TYPE_UINT16,
  BIFROST_INDEX_TYPE_UINT32

} BifrostIndexType;

typedef enum BifrostVertexFormatAttribute_t
{
  // NOTE(Shareef): 32-bit Float
  VFA_FLOAT32_4,
  VFA_FLOAT32_3,
  VFA_FLOAT32_2,
  VFA_FLOAT32_1,
  // NOTE(Shareef): 32-bit Unsigned Int
  VFA_UINT32_4,
  VFA_UINT32_3,
  VFA_UINT32_2,
  VFA_UINT32_1,
  // NOTE(Shareef): 32-bit Signed Int
  VFA_SINT32_4,
  VFA_SINT32_3,
  VFA_SINT32_2,
  VFA_SINT32_1,
  // NOTE(Shareef): 16-bit Unsigned Int
  VFA_USHORT16_4,
  VFA_USHORT16_3,
  VFA_USHORT16_2,
  VFA_USHORT16_1,
  // NOTE(Shareef): 16-bit Signed Int
  VFA_SSHORT16_4,
  VFA_SSHORT16_3,
  VFA_SSHORT16_2,
  VFA_SSHORT16_1,
  // NOTE(Shareef): 8-bit Unsigned Int
  VFA_UCHAR8_4,
  VFA_UCHAR8_3,
  VFA_UCHAR8_2,
  VFA_UCHAR8_1,
  // NOTE(Shareef): 8-bit Signed Int
  VFA_SCHAR8_4,
  VFA_SCHAR8_3,
  VFA_SCHAR8_2,
  VFA_SCHAR8_1,
  // NOTE(Shareef):
  //   For Color packed as 8 bit chars but need to be converted to floats
  VFA_UCHAR8_4_UNORM

} BifrostVertexFormatAttribute;

struct bfGfxCommandList_t;
typedef struct bfGfxCommandList_t bfGfxCommandList;
#if __cplusplus
}
#endif

#endif /* BIFROST_GFX_TYPES_H */
