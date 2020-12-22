/******************************************************************************/
/*!
 * @file   bf_gfx_pipeline_state.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Handles a packed representation of the GPU pipeline.
 * 
 * @version 0.0.1
 * @date    2020-03-22
 *
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
#ifndef BF_GFX_PIPELINE_STATE_H
#define BF_GFX_PIPELINE_STATE_H

#include "bf_gfx_handle.h" /* bf*Handle */

#include <stdint.h> /* uint64_t, uint32_t, int32_t */

#define MASK_FOR_BITS(n) ((1ULL << (n)) - 1)

#if __cplusplus
extern "C" {
#endif
typedef enum
{
  BF_DRAW_MODE_POINT_LIST     = 0,  //!< Each 1 Vertex (1 <= n)
  BF_DRAW_MODE_LINE_LIST      = 1,  //!< Each 2 Vertex (2 <= n)
  BF_DRAW_MODE_LINE_STRIP     = 2,  //!< Each 1 Vertex (2 <= n)
  BF_DRAW_MODE_TRIANGLE_LIST  = 3,  //!< Each 3 Vertex (3 <= n)
  BF_DRAW_MODE_TRIANGLE_STRIP = 4,  //!< Each 3 Vertex (3 <= n)
  BF_DRAW_MODE_TRIANGLE_FAN   = 5,  //!< Each 3 Vertex (3 <= n)

} bfDrawMode; /*! Requires 3-bits */

typedef enum
{
  BF_BLEND_FACTOR_ZERO                     = 0,
  BF_BLEND_FACTOR_ONE                      = 1,
  BF_BLEND_FACTOR_SRC_COLOR                = 2,
  BF_BLEND_FACTOR_ONE_MINUS_SRC_COLOR      = 3,
  BF_BLEND_FACTOR_DST_COLOR                = 4,
  BF_BLEND_FACTOR_ONE_MINUS_DST_COLOR      = 5,
  BF_BLEND_FACTOR_SRC_ALPHA                = 6,
  BF_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA      = 7,
  BF_BLEND_FACTOR_DST_ALPHA                = 8,
  BF_BLEND_FACTOR_ONE_MINUS_DST_ALPHA      = 9,
  BF_BLEND_FACTOR_CONSTANT_COLOR           = 10,
  BF_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR = 11,
  BF_BLEND_FACTOR_CONSTANT_ALPHA           = 12,
  BF_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA = 13,
  BF_BLEND_FACTOR_SRC_ALPHA_SATURATE       = 14,
  BF_BLEND_FACTOR_SRC1_COLOR               = 15,
  BF_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR     = 16,
  BF_BLEND_FACTOR_SRC1_ALPHA               = 17,
  BF_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA     = 18,
  BF_BLEND_FACTOR_NONE                     = 19,

} bfBlendFactor;  // Requires 5-bits (src) + 5-bits (dst)

typedef enum
{
  BF_FRONT_FACE_CCW = 0,
  BF_FRONT_FACE_CW  = 1,

} bfFrontFace;  // Requires 1-bit

typedef enum
{
  BF_CULL_FACE_NONE  = 0ull,
  BF_CULL_FACE_FRONT = 1ull,
  BF_CULL_FACE_BACK  = 2ull,
  BF_CULL_FACE_BOTH  = BF_CULL_FACE_FRONT | BF_CULL_FACE_BACK,  // (3ull)

} bfCullFaceFlags;  // Requires 2-bits

// DepthTest bool Requires (1-bit)
// DepthWrite bool Requires (1-bit)

typedef enum
{
  BF_COMPARE_OP_NEVER            = 0ull,
  BF_COMPARE_OP_LESS_THAN        = 1ull,
  BF_COMPARE_OP_EQUAL            = 2ull,
  BF_COMPARE_OP_LESS_OR_EQUAL    = 3ull,
  BF_COMPARE_OP_GREATER          = 4ull,
  BF_COMPARE_OP_NOT_EQUAL        = 5ull,
  BF_COMPARE_OP_GREATER_OR_EQUAL = 6ull,
  BF_COMPARE_OP_ALWAYS           = 7ull,

} bfCompareOp;  // Requires 3-bits

// StencilTest (1-bit)

enum
{
  BF_PIPELINE_STATE_DRAW_MODE_BITS    = 3,
  BF_PIPELINE_STATE_BLEND_FACTOR_BITS = 5,
  BF_PIPELINE_STATE_FRONT_FACE_BITS   = 1,
  BF_PIPELINE_STATE_CULL_FACE_BITS    = 2,
  BF_PIPELINE_STATE_DEPTH_TEST_BITS   = 1,
  BF_PIPELINE_STATE_DEPTH_WRITE_BITS  = 1,
  BF_PIPELINE_STATE_DEPTH_OP_BITS     = 3,
  BF_PIPELINE_STATE_STENCIL_TEST_BITS = 1,

  BF_PIPELINE_STATE_DRAW_MODE_OFFSET    = 0x0000000000000000ull,
  BF_PIPELINE_STATE_BLEND_SRC_OFFSET    = BF_PIPELINE_STATE_DRAW_MODE_OFFSET + BF_PIPELINE_STATE_DRAW_MODE_BITS,
  BF_PIPELINE_STATE_BLEND_DST_OFFSET    = BF_PIPELINE_STATE_BLEND_SRC_OFFSET + BF_PIPELINE_STATE_BLEND_FACTOR_BITS,
  BF_PIPELINE_STATE_FRONT_FACE_OFFSET   = BF_PIPELINE_STATE_BLEND_DST_OFFSET + BF_PIPELINE_STATE_BLEND_FACTOR_BITS,
  BF_PIPELINE_STATE_CULL_FACE_OFFSET    = BF_PIPELINE_STATE_FRONT_FACE_OFFSET + BF_PIPELINE_STATE_FRONT_FACE_BITS,
  BF_PIPELINE_STATE_DEPTH_TEST_OFFSET   = BF_PIPELINE_STATE_CULL_FACE_OFFSET + BF_PIPELINE_STATE_CULL_FACE_BITS,
  BF_PIPELINE_STATE_DEPTH_WRITE_OFFSET  = BF_PIPELINE_STATE_DEPTH_TEST_OFFSET + BF_PIPELINE_STATE_DEPTH_TEST_BITS,
  BF_PIPELINE_STATE_DEPTH_OP_OFFSET     = BF_PIPELINE_STATE_DEPTH_WRITE_OFFSET + BF_PIPELINE_STATE_DEPTH_WRITE_BITS,
  BF_PIPELINE_STATE_STENCIL_TEST_OFFSET = BF_PIPELINE_STATE_DEPTH_OP_OFFSET + BF_PIPELINE_STATE_DEPTH_OP_BITS,

  BF_PIPELINE_STATE_DRAW_MODE_MASK    = MASK_FOR_BITS(BF_PIPELINE_STATE_DRAW_MODE_BITS) << BF_PIPELINE_STATE_DRAW_MODE_OFFSET,
  BF_PIPELINE_STATE_BLEND_SRC_MASK    = MASK_FOR_BITS(BF_PIPELINE_STATE_BLEND_FACTOR_BITS) << BF_PIPELINE_STATE_BLEND_SRC_OFFSET,
  BF_PIPELINE_STATE_BLEND_DST_MASK    = MASK_FOR_BITS(BF_PIPELINE_STATE_BLEND_FACTOR_BITS) << BF_PIPELINE_STATE_BLEND_DST_OFFSET,
  BF_PIPELINE_STATE_FRONT_FACE_MASK   = MASK_FOR_BITS(BF_PIPELINE_STATE_FRONT_FACE_BITS) << BF_PIPELINE_STATE_FRONT_FACE_OFFSET,
  BF_PIPELINE_STATE_CULL_FACE_MASK    = MASK_FOR_BITS(BF_PIPELINE_STATE_CULL_FACE_BITS) << BF_PIPELINE_STATE_CULL_FACE_OFFSET,
  BF_PIPELINE_STATE_DEPTH_TEST_MASK   = MASK_FOR_BITS(BF_PIPELINE_STATE_DEPTH_TEST_BITS) << BF_PIPELINE_STATE_DEPTH_TEST_OFFSET,
  BF_PIPELINE_STATE_DEPTH_WRITE_MASK  = MASK_FOR_BITS(BF_PIPELINE_STATE_DEPTH_WRITE_BITS) << BF_PIPELINE_STATE_DEPTH_WRITE_OFFSET,
  BF_PIPELINE_STATE_DEPTH_OP_MASK     = MASK_FOR_BITS(BF_PIPELINE_STATE_DEPTH_OP_BITS) << BF_PIPELINE_STATE_DEPTH_OP_OFFSET,
  BF_PIPELINE_STATE_STENCIL_TEST_MASK = MASK_FOR_BITS(BF_PIPELINE_STATE_STENCIL_TEST_BITS) << BF_PIPELINE_STATE_STENCIL_TEST_OFFSET,
};

typedef enum
{
  BF_PIPELINE_DYNAMIC_NONE                 = 0x0,
  BF_PIPELINE_DYNAMIC_VIEWPORT             = (1 << 0),
  BF_PIPELINE_DYNAMIC_SCISSOR              = (1 << 1),
  BF_PIPELINE_DYNAMIC_LINE_WIDTH           = (1 << 2),
  BF_PIPELINE_DYNAMIC_DEPTH_BIAS           = (1 << 3),
  BF_PIPELINE_DYNAMIC_BLEND_CONSTANTS      = (1 << 4),
  BF_PIPELINE_DYNAMIC_DEPTH_BOUNDS         = (1 << 5),
  BF_PIPELINE_DYNAMIC_STENCIL_COMPARE_MASK = (1 << 6),
  BF_PIPELINE_DYNAMIC_STENCIL_WRITE_MASK   = (1 << 7),
  BF_PIPELINE_DYNAMIC_STENCIL_REFERENCE    = (1 << 8),

} bfPipelineDynamicFlags;  // Requires 9 bits

// 31 bits here

typedef enum
{
  BF_STENCIL_OP_KEEP                = 0,
  BF_STENCIL_OP_ZERO                = 1,
  BF_STENCIL_OP_REPLACE             = 2,
  BF_STENCIL_OP_INCREMENT_AND_CLAMP = 3,
  BF_STENCIL_OP_DECREMENT_AND_CLAMP = 4,
  BF_STENCIL_OP_INVERT              = 5,
  BF_STENCIL_OP_INCREMENT_AND_WRAP  = 6,
  BF_STENCIL_OP_DECREMENT_AND_WRAP  = 7,

} bfStencilOp;  // Requires 3 bits

typedef enum
{
  BF_POLYGON_MODE_FILL  = 0,
  BF_POLYGON_MODE_LINE  = 1,
  BF_POLYGON_MODE_POINT = 2,

} bfPolygonFillMode;  // Requires 2 bits

typedef enum
{
  BF_BLEND_OP_ADD     = 0,
  BF_BLEND_OP_SUB     = 1,
  BF_BLEND_OP_REV_SUB = 2,
  BF_BLEND_OP_MIN     = 3,
  BF_BLEND_OP_MAX     = 4,

} bfBlendOp;  // Requires 3 bits

typedef enum
{
  BF_LOGIC_OP_CLEAR      = 0,
  BF_LOGIC_OP_AND        = 1,
  BF_LOGIC_OP_AND_REV    = 2,
  BF_LOGIC_OP_COPY       = 3,
  BF_LOGIC_OP_AND_INV    = 4,
  BF_LOGIC_OP_NONE       = 5,
  BF_LOGIC_OP_XOR        = 6,
  BF_LOGIC_OP_OR         = 7,
  BF_LOGIC_OP_NOR        = 8,
  BF_LOGIC_OP_EQUIVALENT = 9,
  BF_LOGIC_OP_INV        = 10,
  BF_LOGIC_OP_OR_REV     = 11,
  BF_LOGIC_OP_COPY_INV   = 12,
  BF_LOGIC_OP_OR_INV     = 13,
  BF_LOGIC_OP_NAND       = 14,
  BF_LOGIC_OP_SET        = 15,

} bfLogicOp;  // Requires 4 bits

typedef enum
{
  BF_COLOR_MASK_R    = 1 << 0,
  BF_COLOR_MASK_G    = 1 << 1,
  BF_COLOR_MASK_B    = 1 << 2,
  BF_COLOR_MASK_A    = 1 << 3,
  BF_COLOR_MASK_RGBA = BF_COLOR_MASK_R | BF_COLOR_MASK_G | BF_COLOR_MASK_B | BF_COLOR_MASK_A,

} bfColorMask;

typedef struct
{
  // 30 Bits
  uint32_t color_write_mask : 4; // (bfColorMask)
  uint32_t color_blend_op : 3;
  uint32_t color_blend_src : BF_PIPELINE_STATE_BLEND_FACTOR_BITS;  // 5
  uint32_t color_blend_dst : BF_PIPELINE_STATE_BLEND_FACTOR_BITS;  // 5
  uint32_t alpha_blend_op : 3;
  uint32_t alpha_blend_src : BF_PIPELINE_STATE_BLEND_FACTOR_BITS;  // 5
  uint32_t alpha_blend_dst : BF_PIPELINE_STATE_BLEND_FACTOR_BITS;  // 5
  uint32_t _pad : 2;

} bfFramebufferBlending;

typedef struct
{
  /*
    NOTE(SR):
      - Each stencil state needs 36 bits of data (72 total for front and back).
      - '_pad' must be zeroed out for consistent hashing / comparing behavior.
  */

  /*********************************************************************************************************/
  /* BITS:                                                        ||              Size            | Offset */
  uint64_t draw_mode : BF_PIPELINE_STATE_DRAW_MODE_BITS;          /*  3 (bfDrawMode)              |     0  */
  uint64_t front_face : BF_PIPELINE_STATE_FRONT_FACE_BITS;        /*  1                           |     3  */
  uint64_t cull_face : BF_PIPELINE_STATE_CULL_FACE_BITS;          /*  2                           |     4  */
  uint64_t do_depth_test : BF_PIPELINE_STATE_DEPTH_TEST_BITS;     /*  1                           |     6  */
  uint64_t do_depth_clamp : 1;                                    /*  1                           |     7  */
  uint64_t do_depth_bounds_test : 1;                              /*  1                           |     8  */
  uint64_t depth_write : BF_PIPELINE_STATE_DEPTH_WRITE_BITS;      /*  1                           |     9  */
  uint64_t depth_test_op : BF_PIPELINE_STATE_DEPTH_OP_BITS;       /*  3 (bfCompareOp)             |    10  */
  uint64_t do_stencil_test : BF_PIPELINE_STATE_STENCIL_TEST_BITS; /*  1                           |    13  */
  uint64_t primitive_restart : 1;                                 /*  1                           |    14  */
  uint64_t rasterizer_discard : 1;                                /*  1                           |    15  */
  uint64_t do_depth_bias : 1;                                     /*  1                           |    16  */
  uint64_t do_sample_shading : 1;                                 /*  1                           |    17  */
  uint64_t alpha_to_coverage : 1;                                 /*  1                           |    18  */
  uint64_t alpha_to_one : 1;                                      /*  1                           |    19  */
  uint64_t do_logic_op : 1;                                       /*  1                           |    20  */
  uint64_t logic_op : 4;                                          /*  4 (bfLogicOp)               |    21  */
  uint64_t fill_mode : 2;                                         /*  2                           |    25  */
  uint64_t stencil_face_front_fail_op : 3;                        /*  3 (bfStencilOp)             |    27  */
  uint64_t stencil_face_front_pass_op : 3;                        /*  3 (bfStencilOp)             |    30  */
  uint64_t stencil_face_front_depth_fail_op : 3;                  /*  3 (bfStencilOp)             |    33  */
  uint64_t stencil_face_front_compare_op : 3;                     /*  3 (bfCompareOp)             |    36  */
  uint64_t stencil_face_front_compare_mask : 8;                   /*  8 (uint8_t)                 |    39  */
  uint64_t stencil_face_front_write_mask : 8;                     /*  8 (uint8_t)                 |    47  */
  uint64_t stencil_face_front_reference : 8;                      /*  8 (uint8_t)                 |    55  */
  uint64_t stencil_face_back_fail_op : 3;                         /*  3 (bfStencilOp)             |    63  */
  uint64_t stencil_face_back_pass_op : 3;                         /*  3 (bfStencilOp)             |    66  */
  uint64_t stencil_face_back_depth_fail_op : 3;                   /*  3 (bfStencilOp)             |    69  */
  uint64_t stencil_face_back_compare_op : 3;                      /*  3 (bfCompareOp)             |    72  */
  uint64_t stencil_face_back_compare_mask : 8;                    /*  8 (uint8_t)                 |    75  */
  uint64_t stencil_face_back_write_mask : 8;                      /*  8 (uint8_t)                 |    83  */
  uint64_t stencil_face_back_reference : 8;                       /*  8 (uint8_t)                 |    91  */
  uint64_t dynamic_viewport : 1;                                  /*  1 (bool)                    |    99  */
  uint64_t dynamic_scissor : 1;                                   /*  1 (bool)                    |   100  */
  uint64_t dynamic_line_width : 1;                                /*  1 (bool)                    |   101  */
  uint64_t dynamic_depth_bias : 1;                                /*  1 (bool)                    |   102  */
  uint64_t dynamic_blend_constants : 1;                           /*  1 (bool)                    |   103  */
  uint64_t dynamic_depth_bounds : 1;                              /*  1 (bool)                    |   104  */
  uint64_t dynamic_stencil_cmp_mask : 1;                          /*  1 (bool)                    |   105  */
  uint64_t dynamic_stencil_write_mask : 1;                        /*  1 (bool)                    |   106  */
  uint64_t dynamic_stencil_reference : 1;                         /*  1 (bool)                    |   107  */
  uint64_t subpass_index : 2;                                     /*  2 (0-3,k_bfGfxMaxSubpasses) |   108  */
  uint64_t _pad : 17;                                             /* 17 (zeroed-out)              |   110  */
  /*********************************************************************************************************/

} bfPipelineState; /* 110 used bits (17 extra bits) */

typedef struct
{
  float x, y, width, height, min_depth, max_depth;  // 24bytes

} bfViewport;

typedef struct
{
  int32_t  x, y;           // 8bytes
  uint32_t width, height;  // 8bytes

} bfScissorRect;

typedef union
{
  float    float32[4];
  int32_t  int32[4];
  uint32_t uint32[4];

} bfClearColor;

typedef struct
{
  float    depth;
  uint32_t stencil;

} bfClearDepthStencil;

typedef union
{
  bfClearColor        color;
  bfClearDepthStencil depthStencil;

} bfClearValue;

typedef struct
{
  float bias_constant_factor;
  float bias_clamp;
  float bias_slope_factor;
  float min_bound;
  float max_bound;

} bfPipelineDepthInfo;

typedef struct
{
  bfPipelineState         state;
  bfViewport              viewport;
  bfScissorRect           scissor_rect;
  float                   blend_constants[4];
  float                   line_width;
  bfPipelineDepthInfo     depth;
  float                   min_sample_shading;
  uint32_t                sample_mask; /* must default to 0xFFFFFFFF */
  bfFramebufferBlending   blending[k_bfGfxMaxAttachments];
  bfShaderProgramHandle   program;
  bfRenderpassHandle      renderpass;
  bfVertexLayoutSetHandle vertex_set_layout;

} bfPipelineCache;

uint64_t bfPipelineCache_state0Mask(const bfPipelineState* self);
uint64_t bfPipelineCache_state1Mask(const bfPipelineState* self);

#if __cplusplus
}
#endif

#endif /* BF_GFX_PIPELINE_STATE_H */
