#ifndef BIFROST_GFX_RENDER_GRAPH_H
#define BIFROST_GFX_RENDER_GRAPH_H

#if __cplusplus
extern "C" {
#endif
typedef enum BifrostRenderGraphType_t
{
  BIFROST_RG_COMPUTE,
  BIFROST_RG_GRAPHICS,

} BifrostRenderGraphType;

struct BifrostRenderGraph;
typedef BifrostRenderGraph* bfRenderGraphHandle;

void bfRenderGraph_pushRenderpass(bfRenderGraphHandle self, BifrostRenderGraphType type);
void bfRenderGraph_setClearColors();
void bfRenderGraph_pushSubpass(bfRenderGraphHandle self);
void bfRenderGraph_addColorIn(bfRenderGraphHandle self);
void bfRenderGraph_addColorOut(bfRenderGraphHandle self);
void bfRenderGraph_addVertexBufferIn(bfRenderGraphHandle self);
void bfRenderGraph_addVertexBufferOut(bfRenderGraphHandle self);
void bfRenderGraph_addIndexBufferIn(bfRenderGraphHandle self);
void bfRenderGraph_addIndexBufferOut(bfRenderGraphHandle self);
void bfRenderGraph_addIndirectBufferIn(bfRenderGraphHandle self);
void bfRenderGraph_addIndirectBufferOut(bfRenderGraphHandle self);
void bfRenderGraph_addImageIn(bfRenderGraphHandle self);
void bfRenderGraph_addImageOut(bfRenderGraphHandle self);
void bfRenderGraph_addSetupCallback(bfRenderGraphHandle self);
void bfRenderGraph_addExecuteCallback(bfRenderGraphHandle self);
void bfRenderGraph_popSubpass(bfRenderGraphHandle self);
void bfRenderGraph_popRenderpass(bfRenderGraphHandle self);
void bfRenderGraph_compile(bfRenderGraphHandle self);
void bfRenderGraph_execute(bfRenderGraphHandle self);
#if __cplusplus
}
#endif

#endif /* BIFROST_GFX_RENDER_GRAPH_H */
