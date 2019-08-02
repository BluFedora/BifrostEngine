#ifndef BIFROST_GFX_COMMAND_LIST_H
#define BIFROST_GFX_COMMAND_LIST_H

#include "bifrost_gfx_types.h" /* bfGfxCommandList */

#if __cplusplus
extern "C"
{
#endif
void bfGfxCmdList_beginRenderpass();
void bfGfxCmdList_nextSubpass();
void bfGfxCmdList_bindPipeline();
void bfGfxCmdList_bindMaterial();
void bfGfxCmdList_bindShader();
void bfGfxCmdList_bindVertexDesc();
void bfGfxCmdList_bindVertexBuffer();
void bfGfxCmdList_bindIndexBuffer();
void bfGfxCmdList_pipelineSaveState();
// Pipeline Static State
void bfGfxCmdList_pipelineSetDrawMode();
void bfGfxCmdList_pipelineSetBlendSrc();
void bfGfxCmdList_pipelineSetBlendDst();
void bfGfxCmdList_pipelineSetFrontFace();
void bfGfxCmdList_pipelineSetCullFace();
void bfGfxCmdList_pipelineSetDepthTesting();
void bfGfxCmdList_pipelineSetDepthWrite();
void bfGfxCmdList_pipelineSetDepthTestOp();
void bfGfxCmdList_pipelineSetStencilTesting();
void bfGfxCmdList_pipelineSetPrimativeRestart();
void bfGfxCmdList_pipelineSetRasterizerDiscard();
void bfGfxCmdList_pipelineSetDepthBias();
void bfGfxCmdList_pipelineSetSampleShading();
void bfGfxCmdList_pipelineSetAlphaToCoverage();
void bfGfxCmdList_pipelineSetAlphaToOne();
void bfGfxCmdList_pipelineSetLogicOp();
void bfGfxCmdList_pipelineSetPolygonFillMode();
void bfGfxCmdList_pipelineSetColorBlendOp();
void bfGfxCmdList_pipelineSetAlphaBlendOp();
void bfGfxCmdList_pipelineSetBlendSrcAlpha();
void bfGfxCmdList_pipelineSetBlendDstAlpha();
void bfGfxCmdList_pipelineSetColorWriteMask();
void bfGfxCmdList_pipelineSetStencilFaceFrontFailOp();
void bfGfxCmdList_pipelineSetStencilFaceFrontPassOp();
void bfGfxCmdList_pipelineSetStencilFaceFrontDepthFailOp();
void bfGfxCmdList_pipelineSetStencilFaceFrontCompareOp();
void bfGfxCmdList_pipelineSetStencilFaceFrontCompareMask();
void bfGfxCmdList_pipelineSetStencilFaceFrontWriteMask();
void bfGfxCmdList_pipelineSetStencilFaceFrontReference();
void bfGfxCmdList_pipelineSetStencilFaceBackFailOp();
void bfGfxCmdList_pipelineSetStencilFaceBackPassOp();
void bfGfxCmdList_pipelineSetStencilFaceBackDepthFailOp();
void bfGfxCmdList_pipelineSetStencilFaceBackCompareOp();
void bfGfxCmdList_pipelineSetStencilFaceBackCompareMask();
void bfGfxCmdList_pipelineSetStencilFaceBackWriteMask();
void bfGfxCmdList_pipelineSetStencilFaceBackReference();
void bfGfxCmdList_pipelineSetDynamicStates();
void bfGfxCmdList_pipelineSetViewport();
void bfGfxCmdList_pipelineSetScissorRect();
void bfGfxCmdList_pipelineSetBlendConstants();
void bfGfxCmdList_pipelineSetLineWidth();
void bfGfxCmdList_pipelineSetDepthBiasConstantFactor();
void bfGfxCmdList_pipelineSetDepthBiasClamp();
void bfGfxCmdList_pipelineSetDepthBiasSlopeFactor();
void bfGfxCmdList_pipelineSetMinSampleShading();
void bfGfxCmdList_pipelineSetSampleCountFlags();
void bfGfxCmdList_pipelineRestoreState();
// Draw Cmds
void bfGfxCmdList_draw();
void bfGfxCmdList_drawIndexed();
void bfGfxCmdList_endRenderpass();
void bfGfxCmdList_submit();
// Transient Resources
void bfGfxCmdList_createFramebuffer();
void bfGfxCmdList_createVertexBuffer();
void bfGfxCmdList_createIndexBuffer();
#if __cplusplus
}
#endif

#endif /* BIFROST_GFX_COMMAND_LIST_H */
