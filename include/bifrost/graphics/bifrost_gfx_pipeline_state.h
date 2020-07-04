/*!
 * @file   bifrost_gfx_pipeline_state.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 * @version 0.0.1
 * @date    2020-03-22
 *
 * @copyright Copyright (c) 2020
 */
#ifndef BIFROST_GFX_PIPELINE_STATE_H
#define BIFROST_GFX_PIPELINE_STATE_H

#include "bifrost_gfx_handle.h" /* bf*Handle                   */
#include <stdint.h>             /* uint64_t, uint32_t, int32_t */

#define MASK_FOR_BITS(n) ((1ULL << (n)) - 1)

#if __cplusplus
extern "C" {
#endif
typedef enum BifrostDrawMode_t
{
  BIFROST_DRAW_MODE_POINT_LIST     = 0,  // NOTE(Shareef): Each 1 Vertex (1 <= n)
  BIFROST_DRAW_MODE_LINE_LIST      = 1,  // NOTE(Shareef): Each 2 Vertex (2 <= n)
  BIFROST_DRAW_MODE_LINE_STRIP     = 2,  // NOTE(Shareef): Each 1 Vertex (2 <= n)
  BIFROST_DRAW_MODE_TRIANGLE_LIST  = 3,  // NOTE(Shareef): Each 3 Vertex (3 <= n)
  BIFROST_DRAW_MODE_TRIANGLE_STRIP = 4,  // NOTE(Shareef): Each 3 Vertex (3 <= n)
  BIFROST_DRAW_MODE_TRIANGLE_FAN   = 5,  // NOTE(Shareef): Each 3 Vertex (3 <= n)

} BifrostDrawMode;  // NOTE(Shareef): 3-bits

typedef enum BifrostBlendFactor_t
{
  BIFROST_BLEND_FACTOR_ZERO                     = 0,
  BIFROST_BLEND_FACTOR_ONE                      = 1,
  BIFROST_BLEND_FACTOR_SRC_COLOR                = 2,
  BIFROST_BLEND_FACTOR_ONE_MINUS_SRC_COLOR      = 3,
  BIFROST_BLEND_FACTOR_DST_COLOR                = 4,
  BIFROST_BLEND_FACTOR_ONE_MINUS_DST_COLOR      = 5,
  BIFROST_BLEND_FACTOR_SRC_ALPHA                = 6,
  BIFROST_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA      = 7,
  BIFROST_BLEND_FACTOR_DST_ALPHA                = 8,
  BIFROST_BLEND_FACTOR_ONE_MINUS_DST_ALPHA      = 9,
  BIFROST_BLEND_FACTOR_CONSTANT_COLOR           = 10,
  BIFROST_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR = 11,
  BIFROST_BLEND_FACTOR_CONSTANT_ALPHA           = 12,
  BIFROST_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA = 13,
  BIFROST_BLEND_FACTOR_SRC_ALPHA_SATURATE       = 14,
  BIFROST_BLEND_FACTOR_SRC1_COLOR               = 15,
  BIFROST_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR     = 16,
  BIFROST_BLEND_FACTOR_SRC1_ALPHA               = 17,
  BIFROST_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA     = 18,
  BIFROST_BLEND_FACTOR_NONE                     = 19,

} BifrostBlendFactor;  // NOTE(Shareef): 5-bits (src) + 5-bits (dst)

typedef enum BifrostFrontFace_t
{
  BIFROST_FRONT_FACE_CCW = 0,
  BIFROST_FRONT_FACE_CW  = 1,

} BifrostFrontFace;  // NOTE(Shareef): 1-bit

typedef enum BifrostCullFaceFlags_t
{
  BIFROST_CULL_FACE_NONE  = 0ull,
  BIFROST_CULL_FACE_FRONT = 1ull,
  BIFROST_CULL_FACE_BACK  = 2ull,
  BIFROST_CULL_FACE_BOTH  = BIFROST_CULL_FACE_FRONT | BIFROST_CULL_FACE_BACK, // (3ull)

} BifrostCullFaceFlags;  // NOTE(Shareef): 2-bits

// NOTE(Shareef): DepthTest  (1-bit)
// NOTE(Shareef): DepthWrite (1-bit)

typedef enum BifrostCompareOp_t
{
  BIFROST_COMPARE_OP_NEVER            = 0ull,
  BIFROST_COMPARE_OP_LESS_THAN        = 1ull,
  BIFROST_COMPARE_OP_EQUAL            = 2ull,
  BIFROST_COMPARE_OP_LESS_OR_EQUAL    = 3ull,
  BIFROST_COMPARE_OP_GREATER          = 4ull,
  BIFROST_COMPARE_OP_NOT_EQUAL        = 5ull,
  BIFROST_COMPARE_OP_GREATER_OR_EQUAL = 6ull,
  BIFROST_COMPARE_OP_ALWAYS           = 7ull,

} BifrostCompareOp;  // NOTE(Shareef): 3-bits

// NOTE(Shareef): StencilTest (1-bit)

enum
{
  BIFROST_PIPELINE_STATE_DRAW_MODE_BITS    = 3,
  BIFROST_PIPELINE_STATE_BLEND_FACTOR_BITS = 5,
  BIFROST_PIPELINE_STATE_FRONT_FACE_BITS   = 1,
  BIFROST_PIPELINE_STATE_CULL_FACE_BITS    = 2,
  BIFROST_PIPELINE_STATE_DEPTH_TEST_BITS   = 1,
  BIFROST_PIPELINE_STATE_DEPTH_WRITE_BITS  = 1,
  BIFROST_PIPELINE_STATE_DEPTH_OP_BITS     = 3,
  BIFROST_PIPELINE_STATE_STENCIL_TEST_BITS = 1,

  BIFROST_PIPELINE_STATE_DRAW_MODE_OFFSET    = 0x0000000000000000ull,
  BIFROST_PIPELINE_STATE_BLEND_SRC_OFFSET    = BIFROST_PIPELINE_STATE_DRAW_MODE_OFFSET + BIFROST_PIPELINE_STATE_DRAW_MODE_BITS,
  BIFROST_PIPELINE_STATE_BLEND_DST_OFFSET    = BIFROST_PIPELINE_STATE_BLEND_SRC_OFFSET + BIFROST_PIPELINE_STATE_BLEND_FACTOR_BITS,
  BIFROST_PIPELINE_STATE_FRONT_FACE_OFFSET   = BIFROST_PIPELINE_STATE_BLEND_DST_OFFSET + BIFROST_PIPELINE_STATE_BLEND_FACTOR_BITS,
  BIFROST_PIPELINE_STATE_CULL_FACE_OFFSET    = BIFROST_PIPELINE_STATE_FRONT_FACE_OFFSET + BIFROST_PIPELINE_STATE_FRONT_FACE_BITS,
  BIFROST_PIPELINE_STATE_DEPTH_TEST_OFFSET   = BIFROST_PIPELINE_STATE_CULL_FACE_OFFSET + BIFROST_PIPELINE_STATE_CULL_FACE_BITS,
  BIFROST_PIPELINE_STATE_DEPTH_WRITE_OFFSET  = BIFROST_PIPELINE_STATE_DEPTH_TEST_OFFSET + BIFROST_PIPELINE_STATE_DEPTH_TEST_BITS,
  BIFROST_PIPELINE_STATE_DEPTH_OP_OFFSET     = BIFROST_PIPELINE_STATE_DEPTH_WRITE_OFFSET + BIFROST_PIPELINE_STATE_DEPTH_WRITE_BITS,
  BIFROST_PIPELINE_STATE_STENCIL_TEST_OFFSET = BIFROST_PIPELINE_STATE_DEPTH_OP_OFFSET + BIFROST_PIPELINE_STATE_DEPTH_OP_BITS,

  BIFROST_PIPELINE_STATE_DRAW_MODE_MASK    = MASK_FOR_BITS(BIFROST_PIPELINE_STATE_DRAW_MODE_BITS) << BIFROST_PIPELINE_STATE_DRAW_MODE_OFFSET,
  BIFROST_PIPELINE_STATE_BLEND_SRC_MASK    = MASK_FOR_BITS(BIFROST_PIPELINE_STATE_BLEND_FACTOR_BITS) << BIFROST_PIPELINE_STATE_BLEND_SRC_OFFSET,
  BIFROST_PIPELINE_STATE_BLEND_DST_MASK    = MASK_FOR_BITS(BIFROST_PIPELINE_STATE_BLEND_FACTOR_BITS) << BIFROST_PIPELINE_STATE_BLEND_DST_OFFSET,
  BIFROST_PIPELINE_STATE_FRONT_FACE_MASK   = MASK_FOR_BITS(BIFROST_PIPELINE_STATE_FRONT_FACE_BITS) << BIFROST_PIPELINE_STATE_FRONT_FACE_OFFSET,
  BIFROST_PIPELINE_STATE_CULL_FACE_MASK    = MASK_FOR_BITS(BIFROST_PIPELINE_STATE_CULL_FACE_BITS) << BIFROST_PIPELINE_STATE_CULL_FACE_OFFSET,
  BIFROST_PIPELINE_STATE_DEPTH_TEST_MASK   = MASK_FOR_BITS(BIFROST_PIPELINE_STATE_DEPTH_TEST_BITS) << BIFROST_PIPELINE_STATE_DEPTH_TEST_OFFSET,
  BIFROST_PIPELINE_STATE_DEPTH_WRITE_MASK  = MASK_FOR_BITS(BIFROST_PIPELINE_STATE_DEPTH_WRITE_BITS) << BIFROST_PIPELINE_STATE_DEPTH_WRITE_OFFSET,
  BIFROST_PIPELINE_STATE_DEPTH_OP_MASK     = MASK_FOR_BITS(BIFROST_PIPELINE_STATE_DEPTH_OP_BITS) << BIFROST_PIPELINE_STATE_DEPTH_OP_OFFSET,
  BIFROST_PIPELINE_STATE_STENCIL_TEST_MASK = MASK_FOR_BITS(BIFROST_PIPELINE_STATE_STENCIL_TEST_BITS) << BIFROST_PIPELINE_STATE_STENCIL_TEST_OFFSET,
};

typedef enum BifrostPipelineDynamicFlags_t
{
  BIFROST_PIPELINE_DYNAMIC_NONE                 = 0x0,
  BIFROST_PIPELINE_DYNAMIC_VIEWPORT             = (1 << 0),
  BIFROST_PIPELINE_DYNAMIC_SCISSOR              = (1 << 1),
  BIFROST_PIPELINE_DYNAMIC_LINE_WIDTH           = (1 << 2),
  BIFROST_PIPELINE_DYNAMIC_DEPTH_BIAS           = (1 << 3),
  BIFROST_PIPELINE_DYNAMIC_BLEND_CONSTANTS      = (1 << 4),
  BIFROST_PIPELINE_DYNAMIC_DEPTH_BOUNDS         = (1 << 5),
  BIFROST_PIPELINE_DYNAMIC_STENCIL_COMPARE_MASK = (1 << 6),
  BIFROST_PIPELINE_DYNAMIC_STENCIL_WRITE_MASK   = (1 << 7),
  BIFROST_PIPELINE_DYNAMIC_STENCIL_REFERENCE    = (1 << 8),

} BifrostPipelineDynamicFlags;  // 9 bits

// 31 bits here

typedef enum BifrostStencilOp_t
{
  BIFROST_STENCIL_OP_KEEP                = 0,
  BIFROST_STENCIL_OP_ZERO                = 1,
  BIFROST_STENCIL_OP_REPLACE             = 2,
  BIFROST_STENCIL_OP_INCREMENT_AND_CLAMP = 3,
  BIFROST_STENCIL_OP_DECREMENT_AND_CLAMP = 4,
  BIFROST_STENCIL_OP_INVERT              = 5,
  BIFROST_STENCIL_OP_INCREMENT_AND_WRAP  = 6,
  BIFROST_STENCIL_OP_DECREMENT_AND_WRAP  = 7,

} BifrostStencilOp;  // 3 bits

typedef enum
{
  BIFROST_POLYGON_MODE_FILL  = 0,
  BIFROST_POLYGON_MODE_LINE  = 1,
  BIFROST_POLYGON_MODE_POINT = 2,

} BifrostPolygonFillMode;  // 2 bits

typedef enum
{
  BIFROST_BLEND_OP_ADD     = 0,
  BIFROST_BLEND_OP_SUB     = 1,
  BIFROST_BLEND_OP_REV_SUB = 2,
  BIFROST_BLEND_OP_MIN     = 3,
  BIFROST_BLEND_OP_MAX     = 4,

} BifrostBlendOp;  // 3 bits

typedef enum
{
  BIFROST_LOGIC_OP_CLEAR      = 0,
  BIFROST_LOGIC_OP_AND        = 1,
  BIFROST_LOGIC_OP_AND_REV    = 2,
  BIFROST_LOGIC_OP_COPY       = 3,
  BIFROST_LOGIC_OP_AND_INV    = 4,
  BIFROST_LOGIC_OP_NONE       = 5,
  BIFROST_LOGIC_OP_XOR        = 6,
  BIFROST_LOGIC_OP_OR         = 7,
  BIFROST_LOGIC_OP_NOR        = 8,
  BIFROST_LOGIC_OP_EQUIVALENT = 9,
  BIFROST_LOGIC_OP_INV        = 10,
  BIFROST_LOGIC_OP_OR_REV     = 11,
  BIFROST_LOGIC_OP_COPY_INV   = 12,
  BIFROST_LOGIC_OP_OR_INV     = 13,
  BIFROST_LOGIC_OP_NAND       = 14,
  BIFROST_LOGIC_OP_SET        = 15,

} BifrostLogicOp;  // 4 bits

typedef enum BifrostColorMask_t
{
  BIFROST_COLOR_MASK_R    = 1 << 0,
  BIFROST_COLOR_MASK_G    = 1 << 1,
  BIFROST_COLOR_MASK_B    = 1 << 2,
  BIFROST_COLOR_MASK_A    = 1 << 3,
  BIFROST_COLOR_MASK_RGBA = BIFROST_COLOR_MASK_R | BIFROST_COLOR_MASK_G | BIFROST_COLOR_MASK_B | BIFROST_COLOR_MASK_A,

} BifrostColorMask;

typedef struct
{
  // 30 Bits
  uint32_t color_write_mask : 4;
  uint32_t color_blend_op : 3;
  uint32_t color_blend_src : BIFROST_PIPELINE_STATE_BLEND_FACTOR_BITS;  // 5
  uint32_t color_blend_dst : BIFROST_PIPELINE_STATE_BLEND_FACTOR_BITS;  // 5
  uint32_t alpha_blend_op : 3;
  uint32_t alpha_blend_src : BIFROST_PIPELINE_STATE_BLEND_FACTOR_BITS;  // 5
  uint32_t alpha_blend_dst : BIFROST_PIPELINE_STATE_BLEND_FACTOR_BITS;  // 5
  uint32_t _pad : 2;

} bfFramebufferBlending;

typedef struct
{
  /*
    NOTES:
      * Each stencil state needs 36 bits of data (72 total for front and back).
      * '_pad' must be zeroed out for consistent hashing / comparing behavior.
  */

  /********************************************************************************************************/
  /* BITS:                                                             || Size                   | Offset */
  uint64_t draw_mode : BIFROST_PIPELINE_STATE_DRAW_MODE_BITS;          /* 3 (BifrostDrawMode)    |   0    */
  uint64_t front_face : BIFROST_PIPELINE_STATE_FRONT_FACE_BITS;        /* 1                      |   3    */
  uint64_t cull_face : BIFROST_PIPELINE_STATE_CULL_FACE_BITS;          /* 2                      |   4    */
  uint64_t do_depth_test : BIFROST_PIPELINE_STATE_DEPTH_TEST_BITS;     /* 1                      |   6    */
  uint64_t do_depth_clamp : 1;                                         /* 1                      |   7    */
  uint64_t do_depth_bounds_test : 1;                                   /* 1                      |   8    */
  uint64_t depth_write : BIFROST_PIPELINE_STATE_DEPTH_WRITE_BITS;      /* 1                      |   9    */
  uint64_t depth_test_op : BIFROST_PIPELINE_STATE_DEPTH_OP_BITS;       /* 3 (BifrostCompareOp)   |   10   */
  uint64_t do_stencil_test : BIFROST_PIPELINE_STATE_STENCIL_TEST_BITS; /* 1                      |   13   */
  uint64_t primitive_restart : 1;                                      /* 1                      |   14   */
  uint64_t rasterizer_discard : 1;                                     /* 1                      |   15   */
  uint64_t do_depth_bias : 1;                                          /* 1                      |   16   */
  uint64_t do_sample_shading : 1;                                      /* 1                      |   17   */
  uint64_t alpha_to_coverage : 1;                                      /* 1                      |   18   */
  uint64_t alpha_to_one : 1;                                           /* 1                      |   19   */
  uint64_t do_logic_op : 1;                                            /* 1                      |   20   */
  uint64_t logic_op : 4;                                               /* 4                      |   21   */
  uint64_t fill_mode : 2;                                              /* 2                      |   25   */
  uint64_t stencil_face_front_fail_op : 3;                             /* 3 (BifrostStencilOp)   |   27   */
  uint64_t stencil_face_front_pass_op : 3;                             /* 3 (BifrostStencilOp)   |   30   */
  uint64_t stencil_face_front_depth_fail_op : 3;                       /* 3 (BifrostStencilOp)   |   33   */
  uint64_t stencil_face_front_compare_op : 3;                          /* 3 (BifrostCompareOp)   |   36   */
  uint64_t stencil_face_front_compare_mask : 8;                        /* 8 (uint8_t)            |   39   */
  uint64_t stencil_face_front_write_mask : 8;                          /* 8 (uint8_t)            |   47   */
  uint64_t stencil_face_front_reference : 8;                           /* 8 (uint8_t)            |   55   */
  uint64_t stencil_face_back_fail_op : 3;                              /* 3 (BifrostStencilOp)   |   63   */
  uint64_t stencil_face_back_pass_op : 3;                              /* 3 (BifrostStencilOp)   |   66   */
  uint64_t stencil_face_back_depth_fail_op : 3;                        /* 3 (BifrostStencilOp)   |   69   */
  uint64_t stencil_face_back_compare_op : 3;                           /* 3 (BifrostCompareOp)   |   72   */
  uint64_t stencil_face_back_compare_mask : 8;                         /* 8 (uint8_t)            |   75   */
  uint64_t stencil_face_back_write_mask : 8;                           /* 8 (uint8_t)            |   83   */
  uint64_t stencil_face_back_reference : 8;                            /* 8 (uint8_t)            |   91   */
  uint64_t dynamic_viewport : 1;                                       /* 1 (bool)               |   99   */
  uint64_t dynamic_scissor : 1;                                        /* 1 (bool)               |   100  */
  uint64_t dynamic_line_width : 1;                                     /* 1 (bool)               |   101  */
  uint64_t dynamic_depth_bias : 1;                                     /* 1 (bool)               |   102  */
  uint64_t dynamic_blend_constants : 1;                                /* 1 (bool)               |   103  */
  uint64_t dynamic_depth_bounds : 1;                                   /* 1 (bool)               |   104  */
  uint64_t dynamic_stencil_cmp_mask : 1;                               /* 1 (bool)               |   105  */
  uint64_t dynamic_stencil_write_mask : 1;                             /* 1 (bool)               |   106  */
  uint64_t dynamic_stencil_reference : 1;                              /* 1 (bool)               |   107  */
  uint64_t _pad : 19;                                                  /* 19 (zeroed-out)        |   108  */
  /********************************************************************************************************/

} bfPipelineState; /* 107 used bits (19 extra bits) */

typedef struct BifrostViewport_t
{
  float x, y, width, height, min_depth, max_depth;  // 24bytes

} BifrostViewport;

typedef struct BifrostScissorRect_t
{
  int32_t  x, y;           // 8bytes
  uint32_t width, height;  // 8bytes

} BifrostScissorRect;

typedef union BifrostClearColor_t
{
  float    float32[4];
  int32_t  int32[4];
  uint32_t uint32[4];

} BifrostClearColor;

typedef struct BifrostClearDepthStencil_t
{
  float    depth;
  uint32_t stencil;

} BifrostClearDepthStencil;

typedef union BifrostClearValue_t
{
  BifrostClearColor        color;
  BifrostClearDepthStencil depthStencil;

} BifrostClearValue;

typedef struct BifrostPipelineDepthInfo_t
{
  float bias_constant_factor;
  float bias_clamp;
  float bias_slope_factor;
  float min_bound;
  float max_bound;

} BifrostPipelineDepthInfo;

typedef struct bfPipelineCache_t
{
  bfPipelineState          state;
  BifrostViewport          viewport;
  BifrostScissorRect       scissor_rect;
  float                    blend_constants[4];
  float                    line_width;
  BifrostPipelineDepthInfo depth;
  float                    min_sample_shading;
  uint32_t                 sample_mask; /* must default to 0xFFFFFFFF */
  uint32_t                 subpass_index;
  bfFramebufferBlending    blending[BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS];
  bfShaderProgramHandle    program;
  bfRenderpassHandle       renderpass;
  bfVertexLayoutSetHandle  vertex_set_layout;

} bfPipelineCache;

uint64_t bfPipelineCache_state0Mask(const bfPipelineState* self);
uint64_t bfPipelineCache_state1Mask(const bfPipelineState* self);

#if __cplusplus
}
#endif

#endif /* BIFROST_GFX_PIPELINE_STATE_H */
