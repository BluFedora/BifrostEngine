/******************************************************************************/
/*!
 * @file   bf_gfx_types.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Defines basic enumerations used throughout the API.
 * 
 * @version 0.0.1
 * @date    2020-03-22
 *
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
#ifndef BF_GFX_TYPES_H
#define BF_GFX_TYPES_H

#include <stdint.h> /* uint32_t */

#if __cplusplus
extern "C" {
#endif
typedef enum
{
  BF_INDEX_TYPE_UINT16,
  BF_INDEX_TYPE_UINT32,

} bfGfxIndexType;

/* clang-format off */

typedef enum
{
  //
  // 32-bit Float Formats
  //
  BF_VFA_FLOAT32_4,  BF_VFA_FLOAT32_3,  BF_VFA_FLOAT32_2,  BF_VFA_FLOAT32_1,
  //
  // 32-bit Unsigned Int Formats
  //
  BF_VFA_UINT32_4,   BF_VFA_UINT32_3,   BF_VFA_UINT32_2,   BF_VFA_UINT32_1,
  //
  // 32-bit Signed Int Formats
  //
  BF_VFA_SINT32_4,   BF_VFA_SINT32_3,   BF_VFA_SINT32_2,   BF_VFA_SINT32_1,
  //
  // 16-bit Unsigned Int Formats
  //
  BF_VFA_USHORT16_4, BF_VFA_USHORT16_3, BF_VFA_USHORT16_2, BF_VFA_USHORT16_1,
  //
  // 16-bit Signed Int Formats
  //
  BF_VFA_SSHORT16_4, BF_VFA_SSHORT16_3, BF_VFA_SSHORT16_2, BF_VFA_SSHORT16_1,
  //
  // 8-bit Unsigned Int Formats
  //
  BF_VFA_UCHAR8_4,   BF_VFA_UCHAR8_3,   BF_VFA_UCHAR8_2,   BF_VFA_UCHAR8_1,
  //
  // 8-bit Signed Int Formats
  //
  BF_VFA_SCHAR8_4,   BF_VFA_SCHAR8_3,   BF_VFA_SCHAR8_2,   BF_VFA_SCHAR8_1,
  //
  // For Color packed as 8 bit chars but need to be converted / normalized to floats
  //
  BF_VFA_UCHAR8_4_UNORM,

} bfGfxVertexFormatAttribute;

/* clang-format on */

//
// Compatible with Vulkan's VkFormat enum.
//   [https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkFormat.html]
//
typedef enum
{
  BF_IMAGE_FORMAT_UNDEFINED                                  = 0,
  BF_IMAGE_FORMAT_R4G4_UNORM_PACK8                           = 1,
  BF_IMAGE_FORMAT_R4G4B4A4_UNORM_PACK16                      = 2,
  BF_IMAGE_FORMAT_B4G4R4A4_UNORM_PACK16                      = 3,
  BF_IMAGE_FORMAT_R5G6B5_UNORM_PACK16                        = 4,
  BF_IMAGE_FORMAT_B5G6R5_UNORM_PACK16                        = 5,
  BF_IMAGE_FORMAT_R5G5B5A1_UNORM_PACK16                      = 6,
  BF_IMAGE_FORMAT_B5G5R5A1_UNORM_PACK16                      = 7,
  BF_IMAGE_FORMAT_A1R5G5B5_UNORM_PACK16                      = 8,
  BF_IMAGE_FORMAT_R8_UNORM                                   = 9,
  BF_IMAGE_FORMAT_R8_SNORM                                   = 10,
  BF_IMAGE_FORMAT_R8_USCALED                                 = 11,
  BF_IMAGE_FORMAT_R8_SSCALED                                 = 12,
  BF_IMAGE_FORMAT_R8_UINT                                    = 13,
  BF_IMAGE_FORMAT_R8_SINT                                    = 14,
  BF_IMAGE_FORMAT_R8_SRGB                                    = 15,
  BF_IMAGE_FORMAT_R8G8_UNORM                                 = 16,
  BF_IMAGE_FORMAT_R8G8_SNORM                                 = 17,
  BF_IMAGE_FORMAT_R8G8_USCALED                               = 18,
  BF_IMAGE_FORMAT_R8G8_SSCALED                               = 19,
  BF_IMAGE_FORMAT_R8G8_UINT                                  = 20,
  BF_IMAGE_FORMAT_R8G8_SINT                                  = 21,
  BF_IMAGE_FORMAT_R8G8_SRGB                                  = 22,
  BF_IMAGE_FORMAT_R8G8B8_UNORM                               = 23,
  BF_IMAGE_FORMAT_R8G8B8_SNORM                               = 24,
  BF_IMAGE_FORMAT_R8G8B8_USCALED                             = 25,
  BF_IMAGE_FORMAT_R8G8B8_SSCALED                             = 26,
  BF_IMAGE_FORMAT_R8G8B8_UINT                                = 27,
  BF_IMAGE_FORMAT_R8G8B8_SINT                                = 28,
  BF_IMAGE_FORMAT_R8G8B8_SRGB                                = 29,
  BF_IMAGE_FORMAT_B8G8R8_UNORM                               = 30,
  BF_IMAGE_FORMAT_B8G8R8_SNORM                               = 31,
  BF_IMAGE_FORMAT_B8G8R8_USCALED                             = 32,
  BF_IMAGE_FORMAT_B8G8R8_SSCALED                             = 33,
  BF_IMAGE_FORMAT_B8G8R8_UINT                                = 34,
  BF_IMAGE_FORMAT_B8G8R8_SINT                                = 35,
  BF_IMAGE_FORMAT_B8G8R8_SRGB                                = 36,
  BF_IMAGE_FORMAT_R8G8B8A8_UNORM                             = 37,
  BF_IMAGE_FORMAT_R8G8B8A8_SNORM                             = 38,
  BF_IMAGE_FORMAT_R8G8B8A8_USCALED                           = 39,
  BF_IMAGE_FORMAT_R8G8B8A8_SSCALED                           = 40,
  BF_IMAGE_FORMAT_R8G8B8A8_UINT                              = 41,
  BF_IMAGE_FORMAT_R8G8B8A8_SINT                              = 42,
  BF_IMAGE_FORMAT_R8G8B8A8_SRGB                              = 43,
  BF_IMAGE_FORMAT_B8G8R8A8_UNORM                             = 44,
  BF_IMAGE_FORMAT_B8G8R8A8_SNORM                             = 45,
  BF_IMAGE_FORMAT_B8G8R8A8_USCALED                           = 46,
  BF_IMAGE_FORMAT_B8G8R8A8_SSCALED                           = 47,
  BF_IMAGE_FORMAT_B8G8R8A8_UINT                              = 48,
  BF_IMAGE_FORMAT_B8G8R8A8_SINT                              = 49,
  BF_IMAGE_FORMAT_B8G8R8A8_SRGB                              = 50,
  BF_IMAGE_FORMAT_A8B8G8R8_UNORM_PACK32                      = 51,
  BF_IMAGE_FORMAT_A8B8G8R8_SNORM_PACK32                      = 52,
  BF_IMAGE_FORMAT_A8B8G8R8_USCALED_PACK32                    = 53,
  BF_IMAGE_FORMAT_A8B8G8R8_SSCALED_PACK32                    = 54,
  BF_IMAGE_FORMAT_A8B8G8R8_UINT_PACK32                       = 55,
  BF_IMAGE_FORMAT_A8B8G8R8_SINT_PACK32                       = 56,
  BF_IMAGE_FORMAT_A8B8G8R8_SRGB_PACK32                       = 57,
  BF_IMAGE_FORMAT_A2R10G10B10_UNORM_PACK32                   = 58,
  BF_IMAGE_FORMAT_A2R10G10B10_SNORM_PACK32                   = 59,
  BF_IMAGE_FORMAT_A2R10G10B10_USCALED_PACK32                 = 60,
  BF_IMAGE_FORMAT_A2R10G10B10_SSCALED_PACK32                 = 61,
  BF_IMAGE_FORMAT_A2R10G10B10_UINT_PACK32                    = 62,
  BF_IMAGE_FORMAT_A2R10G10B10_SINT_PACK32                    = 63,
  BF_IMAGE_FORMAT_A2B10G10R10_UNORM_PACK32                   = 64,
  BF_IMAGE_FORMAT_A2B10G10R10_SNORM_PACK32                   = 65,
  BF_IMAGE_FORMAT_A2B10G10R10_USCALED_PACK32                 = 66,
  BF_IMAGE_FORMAT_A2B10G10R10_SSCALED_PACK32                 = 67,
  BF_IMAGE_FORMAT_A2B10G10R10_UINT_PACK32                    = 68,
  BF_IMAGE_FORMAT_A2B10G10R10_SINT_PACK32                    = 69,
  BF_IMAGE_FORMAT_R16_UNORM                                  = 70,
  BF_IMAGE_FORMAT_R16_SNORM                                  = 71,
  BF_IMAGE_FORMAT_R16_USCALED                                = 72,
  BF_IMAGE_FORMAT_R16_SSCALED                                = 73,
  BF_IMAGE_FORMAT_R16_UINT                                   = 74,
  BF_IMAGE_FORMAT_R16_SINT                                   = 75,
  BF_IMAGE_FORMAT_R16_SFLOAT                                 = 76,
  BF_IMAGE_FORMAT_R16G16_UNORM                               = 77,
  BF_IMAGE_FORMAT_R16G16_SNORM                               = 78,
  BF_IMAGE_FORMAT_R16G16_USCALED                             = 79,
  BF_IMAGE_FORMAT_R16G16_SSCALED                             = 80,
  BF_IMAGE_FORMAT_R16G16_UINT                                = 81,
  BF_IMAGE_FORMAT_R16G16_SINT                                = 82,
  BF_IMAGE_FORMAT_R16G16_SFLOAT                              = 83,
  BF_IMAGE_FORMAT_R16G16B16_UNORM                            = 84,
  BF_IMAGE_FORMAT_R16G16B16_SNORM                            = 85,
  BF_IMAGE_FORMAT_R16G16B16_USCALED                          = 86,
  BF_IMAGE_FORMAT_R16G16B16_SSCALED                          = 87,
  BF_IMAGE_FORMAT_R16G16B16_UINT                             = 88,
  BF_IMAGE_FORMAT_R16G16B16_SINT                             = 89,
  BF_IMAGE_FORMAT_R16G16B16_SFLOAT                           = 90,
  BF_IMAGE_FORMAT_R16G16B16A16_UNORM                         = 91,
  BF_IMAGE_FORMAT_R16G16B16A16_SNORM                         = 92,
  BF_IMAGE_FORMAT_R16G16B16A16_USCALED                       = 93,
  BF_IMAGE_FORMAT_R16G16B16A16_SSCALED                       = 94,
  BF_IMAGE_FORMAT_R16G16B16A16_UINT                          = 95,
  BF_IMAGE_FORMAT_R16G16B16A16_SINT                          = 96,
  BF_IMAGE_FORMAT_R16G16B16A16_SFLOAT                        = 97,
  BF_IMAGE_FORMAT_R32_UINT                                   = 98,
  BF_IMAGE_FORMAT_R32_SINT                                   = 99,
  BF_IMAGE_FORMAT_R32_SFLOAT                                 = 100,
  BF_IMAGE_FORMAT_R32G32_UINT                                = 101,
  BF_IMAGE_FORMAT_R32G32_SINT                                = 102,
  BF_IMAGE_FORMAT_R32G32_SFLOAT                              = 103,
  BF_IMAGE_FORMAT_R32G32B32_UINT                             = 104,
  BF_IMAGE_FORMAT_R32G32B32_SINT                             = 105,
  BF_IMAGE_FORMAT_R32G32B32_SFLOAT                           = 106,
  BF_IMAGE_FORMAT_R32G32B32A32_UINT                          = 107,
  BF_IMAGE_FORMAT_R32G32B32A32_SINT                          = 108,
  BF_IMAGE_FORMAT_R32G32B32A32_SFLOAT                        = 109,
  BF_IMAGE_FORMAT_R64_UINT                                   = 110,
  BF_IMAGE_FORMAT_R64_SINT                                   = 111,
  BF_IMAGE_FORMAT_R64_SFLOAT                                 = 112,
  BF_IMAGE_FORMAT_R64G64_UINT                                = 113,
  BF_IMAGE_FORMAT_R64G64_SINT                                = 114,
  BF_IMAGE_FORMAT_R64G64_SFLOAT                              = 115,
  BF_IMAGE_FORMAT_R64G64B64_UINT                             = 116,
  BF_IMAGE_FORMAT_R64G64B64_SINT                             = 117,
  BF_IMAGE_FORMAT_R64G64B64_SFLOAT                           = 118,
  BF_IMAGE_FORMAT_R64G64B64A64_UINT                          = 119,
  BF_IMAGE_FORMAT_R64G64B64A64_SINT                          = 120,
  BF_IMAGE_FORMAT_R64G64B64A64_SFLOAT                        = 121,
  BF_IMAGE_FORMAT_B10G11R11_UFLOAT_PACK32                    = 122,
  BF_IMAGE_FORMAT_E5B9G9R9_UFLOAT_PACK32                     = 123,
  BF_IMAGE_FORMAT_D16_UNORM                                  = 124,
  BF_IMAGE_FORMAT_X8_D24_UNORM_PACK32                        = 125,
  BF_IMAGE_FORMAT_D32_SFLOAT                                 = 126,
  BF_IMAGE_FORMAT_S8_UINT                                    = 127,
  BF_IMAGE_FORMAT_D16_UNORM_S8_UINT                          = 128,
  BF_IMAGE_FORMAT_D24_UNORM_S8_UINT                          = 129,
  BF_IMAGE_FORMAT_D32_SFLOAT_S8_UINT                         = 130,
  BF_IMAGE_FORMAT_BC1_RGB_UNORM_BLOCK                        = 131,
  BF_IMAGE_FORMAT_BC1_RGB_SRGB_BLOCK                         = 132,
  BF_IMAGE_FORMAT_BC1_RGBA_UNORM_BLOCK                       = 133,
  BF_IMAGE_FORMAT_BC1_RGBA_SRGB_BLOCK                        = 134,
  BF_IMAGE_FORMAT_BC2_UNORM_BLOCK                            = 135,
  BF_IMAGE_FORMAT_BC2_SRGB_BLOCK                             = 136,
  BF_IMAGE_FORMAT_BC3_UNORM_BLOCK                            = 137,
  BF_IMAGE_FORMAT_BC3_SRGB_BLOCK                             = 138,
  BF_IMAGE_FORMAT_BC4_UNORM_BLOCK                            = 139,
  BF_IMAGE_FORMAT_BC4_SNORM_BLOCK                            = 140,
  BF_IMAGE_FORMAT_BC5_UNORM_BLOCK                            = 141,
  BF_IMAGE_FORMAT_BC5_SNORM_BLOCK                            = 142,
  BF_IMAGE_FORMAT_BC6H_UFLOAT_BLOCK                          = 143,
  BF_IMAGE_FORMAT_BC6H_SFLOAT_BLOCK                          = 144,
  BF_IMAGE_FORMAT_BC7_UNORM_BLOCK                            = 145,
  BF_IMAGE_FORMAT_BC7_SRGB_BLOCK                             = 146,
  BF_IMAGE_FORMAT_ETC2_R8G8B8_UNORM_BLOCK                    = 147,
  BF_IMAGE_FORMAT_ETC2_R8G8B8_SRGB_BLOCK                     = 148,
  BF_IMAGE_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK                  = 149,
  BF_IMAGE_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK                   = 150,
  BF_IMAGE_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK                  = 151,
  BF_IMAGE_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK                   = 152,
  BF_IMAGE_FORMAT_EAC_R11_UNORM_BLOCK                        = 153,
  BF_IMAGE_FORMAT_EAC_R11_SNORM_BLOCK                        = 154,
  BF_IMAGE_FORMAT_EAC_R11G11_UNORM_BLOCK                     = 155,
  BF_IMAGE_FORMAT_EAC_R11G11_SNORM_BLOCK                     = 156,
  BF_IMAGE_FORMAT_ASTC_4x4_UNORM_BLOCK                       = 157,
  BF_IMAGE_FORMAT_ASTC_4x4_SRGB_BLOCK                        = 158,
  BF_IMAGE_FORMAT_ASTC_5x4_UNORM_BLOCK                       = 159,
  BF_IMAGE_FORMAT_ASTC_5x4_SRGB_BLOCK                        = 160,
  BF_IMAGE_FORMAT_ASTC_5x5_UNORM_BLOCK                       = 161,
  BF_IMAGE_FORMAT_ASTC_5x5_SRGB_BLOCK                        = 162,
  BF_IMAGE_FORMAT_ASTC_6x5_UNORM_BLOCK                       = 163,
  BF_IMAGE_FORMAT_ASTC_6x5_SRGB_BLOCK                        = 164,
  BF_IMAGE_FORMAT_ASTC_6x6_UNORM_BLOCK                       = 165,
  BF_IMAGE_FORMAT_ASTC_6x6_SRGB_BLOCK                        = 166,
  BF_IMAGE_FORMAT_ASTC_8x5_UNORM_BLOCK                       = 167,
  BF_IMAGE_FORMAT_ASTC_8x5_SRGB_BLOCK                        = 168,
  BF_IMAGE_FORMAT_ASTC_8x6_UNORM_BLOCK                       = 169,
  BF_IMAGE_FORMAT_ASTC_8x6_SRGB_BLOCK                        = 170,
  BF_IMAGE_FORMAT_ASTC_8x8_UNORM_BLOCK                       = 171,
  BF_IMAGE_FORMAT_ASTC_8x8_SRGB_BLOCK                        = 172,
  BF_IMAGE_FORMAT_ASTC_10x5_UNORM_BLOCK                      = 173,
  BF_IMAGE_FORMAT_ASTC_10x5_SRGB_BLOCK                       = 174,
  BF_IMAGE_FORMAT_ASTC_10x6_UNORM_BLOCK                      = 175,
  BF_IMAGE_FORMAT_ASTC_10x6_SRGB_BLOCK                       = 176,
  BF_IMAGE_FORMAT_ASTC_10x8_UNORM_BLOCK                      = 177,
  BF_IMAGE_FORMAT_ASTC_10x8_SRGB_BLOCK                       = 178,
  BF_IMAGE_FORMAT_ASTC_10x10_UNORM_BLOCK                     = 179,
  BF_IMAGE_FORMAT_ASTC_10x10_SRGB_BLOCK                      = 180,
  BF_IMAGE_FORMAT_ASTC_12x10_UNORM_BLOCK                     = 181,
  BF_IMAGE_FORMAT_ASTC_12x10_SRGB_BLOCK                      = 182,
  BF_IMAGE_FORMAT_ASTC_12x12_UNORM_BLOCK                     = 183,
  BF_IMAGE_FORMAT_ASTC_12x12_SRGB_BLOCK                      = 184,
  BF_IMAGE_FORMAT_G8B8G8R8_422_UNORM                         = 1000156000,
  BF_IMAGE_FORMAT_B8G8R8G8_422_UNORM                         = 1000156001,
  BF_IMAGE_FORMAT_G8_B8_R8_3PLANE_420_UNORM                  = 1000156002,
  BF_IMAGE_FORMAT_G8_B8R8_2PLANE_420_UNORM                   = 1000156003,
  BF_IMAGE_FORMAT_G8_B8_R8_3PLANE_422_UNORM                  = 1000156004,
  BF_IMAGE_FORMAT_G8_B8R8_2PLANE_422_UNORM                   = 1000156005,
  BF_IMAGE_FORMAT_G8_B8_R8_3PLANE_444_UNORM                  = 1000156006,
  BF_IMAGE_FORMAT_R10X6_UNORM_PACK16                         = 1000156007,
  BF_IMAGE_FORMAT_R10X6G10X6_UNORM_2PACK16                   = 1000156008,
  BF_IMAGE_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16         = 1000156009,
  BF_IMAGE_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16     = 1000156010,
  BF_IMAGE_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16     = 1000156011,
  BF_IMAGE_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16 = 1000156012,
  BF_IMAGE_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16  = 1000156013,
  BF_IMAGE_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16 = 1000156014,
  BF_IMAGE_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16  = 1000156015,
  BF_IMAGE_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16 = 1000156016,
  BF_IMAGE_FORMAT_R12X4_UNORM_PACK16                         = 1000156017,
  BF_IMAGE_FORMAT_R12X4G12X4_UNORM_2PACK16                   = 1000156018,
  BF_IMAGE_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16         = 1000156019,
  BF_IMAGE_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16     = 1000156020,
  BF_IMAGE_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16     = 1000156021,
  BF_IMAGE_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16 = 1000156022,
  BF_IMAGE_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16  = 1000156023,
  BF_IMAGE_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16 = 1000156024,
  BF_IMAGE_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16  = 1000156025,
  BF_IMAGE_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16 = 1000156026,
  BF_IMAGE_FORMAT_G16B16G16R16_422_UNORM                     = 1000156027,
  BF_IMAGE_FORMAT_B16G16R16G16_422_UNORM                     = 1000156028,
  BF_IMAGE_FORMAT_G16_B16_R16_3PLANE_420_UNORM               = 1000156029,
  BF_IMAGE_FORMAT_G16_B16R16_2PLANE_420_UNORM                = 1000156030,
  BF_IMAGE_FORMAT_G16_B16_R16_3PLANE_422_UNORM               = 1000156031,
  BF_IMAGE_FORMAT_G16_B16R16_2PLANE_422_UNORM                = 1000156032,
  BF_IMAGE_FORMAT_G16_B16_R16_3PLANE_444_UNORM               = 1000156033,
  BF_IMAGE_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG                = 1000054000,
  BF_IMAGE_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG                = 1000054001,
  BF_IMAGE_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG                = 1000054002,
  BF_IMAGE_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG                = 1000054003,
  BF_IMAGE_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG                 = 1000054004,
  BF_IMAGE_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG                 = 1000054005,
  BF_IMAGE_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG                 = 1000054006,
  BF_IMAGE_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG                 = 1000054007,
  BF_IMAGE_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT                  = 1000066000,
  BF_IMAGE_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT                  = 1000066001,
  BF_IMAGE_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT                  = 1000066002,
  BF_IMAGE_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT                  = 1000066003,
  BF_IMAGE_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT                  = 1000066004,
  BF_IMAGE_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT                  = 1000066005,
  BF_IMAGE_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT                  = 1000066006,
  BF_IMAGE_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT                  = 1000066007,
  BF_IMAGE_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT                 = 1000066008,
  BF_IMAGE_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT                 = 1000066009,
  BF_IMAGE_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT                 = 1000066010,
  BF_IMAGE_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT                = 1000066011,
  BF_IMAGE_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT                = 1000066012,
  BF_IMAGE_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT                = 1000066013,
  BF_IMAGE_FORMAT_MAX_ENUM                                   = 0x7FFFFFFF,

} bfGfxImageFormat;

//
// Compatible with Vulkan's VkImageAspectFlagBits enum.
//
typedef enum
{
  BF_PIPELINE_STAGE_TOP_OF_PIPE_BIT                    = 0x00000001,
  BF_PIPELINE_STAGE_DRAW_INDIRECT_BIT                  = 0x00000002,
  BF_PIPELINE_STAGE_VERTEX_INPUT_BIT                   = 0x00000004,
  BF_PIPELINE_STAGE_VERTEX_SHADER_BIT                  = 0x00000008,
  BF_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT    = 0x00000010,
  BF_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT = 0x00000020,
  BF_PIPELINE_STAGE_GEOMETRY_SHADER_BIT                = 0x00000040,
  BF_PIPELINE_STAGE_FRAGMENT_SHADER_BIT                = 0x00000080,
  BF_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT           = 0x00000100,
  BF_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT            = 0x00000200,
  BF_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT        = 0x00000400,
  BF_PIPELINE_STAGE_COMPUTE_SHADER_BIT                 = 0x00000800,
  BF_PIPELINE_STAGE_TRANSFER_BIT                       = 0x00001000,
  BF_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT                 = 0x00002000,
  BF_PIPELINE_STAGE_HOST_BIT                           = 0x00004000,
  BF_PIPELINE_STAGE_ALL_GRAPHICS_BIT                   = 0x00008000,
  BF_PIPELINE_STAGE_ALL_COMMANDS_BIT                   = 0x00010000,

} bfGfxPipelineStageFlags;

typedef uint32_t bfGfxPipelineStageBits;


//
// Compatible with Vulkan's VkAccessFlagBits enum.
//
typedef enum
{
  BF_ACCESS_INDIRECT_COMMAND_READ_BIT          = 0x00000001,
  BF_ACCESS_INDEX_READ_BIT                     = 0x00000002,
  BF_ACCESS_VERTEX_ATTRIBUTE_READ_BIT          = 0x00000004,
  BF_ACCESS_UNIFORM_READ_BIT                   = 0x00000008,
  BF_ACCESS_INPUT_ATTACHMENT_READ_BIT          = 0x00000010,
  BF_ACCESS_SHADER_READ_BIT                    = 0x00000020,
  BF_ACCESS_SHADER_WRITE_BIT                   = 0x00000040,
  BF_ACCESS_COLOR_ATTACHMENT_READ_BIT          = 0x00000080,
  BF_ACCESS_COLOR_ATTACHMENT_WRITE_BIT         = 0x00000100,
  BF_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT  = 0x00000200,
  BF_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT = 0x00000400,
  BF_ACCESS_TRANSFER_READ_BIT                  = 0x00000800,
  BF_ACCESS_TRANSFER_WRITE_BIT                 = 0x00001000,
  BF_ACCESS_HOST_READ_BIT                      = 0x00002000,
  BF_ACCESS_HOST_WRITE_BIT                     = 0x00004000,
  BF_ACCESS_MEMORY_READ_BIT                    = 0x00008000,
  BF_ACCESS_MEMORY_WRITE_BIT                   = 0x00010000,

} bfGfxAccessFlags;

typedef uint32_t bfGfxAccessFlagsBits;

//
// Compatible with Vulkan's VkSampleCountFlagBits enum.
//
typedef enum
{
  BF_SAMPLE_1  = 0x00000001,
  BF_SAMPLE_2  = 0x00000002,
  BF_SAMPLE_4  = 0x00000004,
  BF_SAMPLE_8  = 0x00000008,
  BF_SAMPLE_16 = 0x00000010,
  BF_SAMPLE_32 = 0x00000020,
  BF_SAMPLE_64 = 0x00000040,

} bfGfxSampleFlags;

typedef enum
{
  BF_IMAGE_LAYOUT_UNDEFINED                                  = 0,
  BF_IMAGE_LAYOUT_GENERAL                                    = 1,
  BF_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL                   = 2,
  BF_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL           = 3,
  BF_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL            = 4,
  BF_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL                   = 5,
  BF_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL                       = 6,
  BF_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL                       = 7,
  BF_IMAGE_LAYOUT_PRE_INITIALIZED                            = 8,
  BF_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL = 1000117000,
  BF_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL = 1000117001,
  BF_IMAGE_LAYOUT_PRESENT_SRC_KHR                            = 1000001002,
  BF_IMAGE_LAYOUT_SHARED_PRESENT_KHR                         = 1000111000,

} bfGfxImageLayout;

typedef enum
{
  BF_GFX_QUEUE_GRAPHICS,
  BF_GFX_QUEUE_COMPUTE,
  BF_GFX_QUEUE_TRANSFER,
  BF_GFX_QUEUE_PRESENT,
  BF_GFX_QUEUE_MAX,
  BF_GFX_QUEUE_IGNORE = ~0U,

} bfGfxQueueType;
#if __cplusplus
}
#endif

#endif /* BF_GFX_TYPES_H */
