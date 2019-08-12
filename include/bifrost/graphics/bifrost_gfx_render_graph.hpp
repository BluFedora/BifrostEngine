#ifndef BIFROST_GFX_RENDER_GRAPH_HPP
#define BIFROST_GFX_RENDER_GRAPH_HPP

#include <cassert>
#include <unordered_map>  // TODO(Shareef): Swicth to custom hash map.
#include <vector>         /* vector<T> */

#define BIFROST_GFX_RENDER_GRAPH_TEST 1

namespace bifrost
{
#ifndef bfBit
#define bfBit(index) (1ULL << (index))
#endif

  static constexpr unsigned int BIFROST_RENDERPASS_DEBUG_NAME_LEN = 64u;
  static constexpr unsigned int BIFROST_RESOURCE_NAME_LEN         = 128u;

  class FrameGraph;
  struct GraphResourceBase;

  template<typename T>
  using Vector = std::vector<T>;

  template<typename K, typename V>
  using HashTable = std::unordered_map<k, V>;
  using String    = std::string;

  template<std::size_t N>
  struct NameString final
  {
    char str[N];

    explicit NameString(const char* str_in) :
      str{}
    {
      std::strncpy(str, str_in, N);
    }
  };

  enum class BytecodeInst : std::uint8_t
  {
    RENDERPASS,
    /*
    EXECUTION_BARRIER,
    MEMORY_BARRIER,
    IMAGE_BARRIER,
    BUFFER_BARRIER,
    */
    BARRIER,
    NEXT_PASS,
  };

  enum class BarrierType
  {
    EXECUTION,
    MEMORY,
    IMAGE,
    BUFFER,
  };

  // Can only be merged if this
  // has the same targets AND
  // not "BarrierType::IMAGE" or "BarrierType::BUFFER"
  // also the queues need to match.

  class GraphBarrier final
  {
   private:
    BarrierType                m_Type;
    Vector<GraphResourceBase*> m_Targets;

   public:
    explicit GraphBarrier(BarrierType type) :
      m_Type{type},
      m_Targets{}
    {
    }

    bool hasTarget(const GraphResourceBase* t) const
    {
      const auto it = std::find(m_Targets.begin(), m_Targets.end(), t);
      return it != m_Targets.end();
    }
  };

  struct SubpassBase
  {
    void addColorIn(GraphResourceBase*)
    {
    }

    void addColorOut(GraphResourceBase*)
    {
    }

    void addDepthIn(GraphResourceBase*)
    {
    }

    void addDepthOut(GraphResourceBase*)
    {
    }

    virtual void execute(FrameGraph& graph, void* data);
  };

  template<typename TData, typename TExecFn>
  struct Subpass final : public SubpassBase
  {
    TExecFn exec_fn;

    void execute(FrameGraph& graph, void* data) override
    {
      exec_fn(graph, *static_cast<const TData*>(data));
    }
  };

  enum class RenderpassType
  {
    GRAPHICS,
    COMPUTE,
    IMAGE_BLIT,
  };

  struct RenderpassBase
  {
    NameString<BIFROST_RENDERPASS_DEBUG_NAME_LEN> name;
    Vector<SubpassBase*>                          subpasses;
    Vector<GraphResourceBase*>                    reads;
    Vector<GraphResourceBase*>                    writes;
    std::size_t                                   queue_family;
    std::size_t                                   barrier_index;
    std::size_t                                   index;
    void* const                                   data_ptr;

    explicit RenderpassBase(const char* name, std::size_t index, void* data_ptr) :
      name{name},
      subpasses{},
      reads{},
      writes{},
      queue_family(-1),
      barrier_index(-1),
      index{index},
      data_ptr{data_ptr}
    {
    }

    void execute(FrameGraph& graph)
    {
      for (SubpassBase* const pass : subpasses)
      {
        pass->execute(graph, data_ptr);
      }
    }
  };

  template<typename TData, bool is_compute>
  struct Renderpass : public RenderpassBase
  {
    TData data;

    explicit Renderpass(const char* name, std::size_t index) :
      RenderpassBase(name, index, &data),
      data{}
    {
    }

    void compile(FrameGraph& graph)
    {
      if constexpr (is_compute)
      {
        assert(subpasses.size() == 1 "A compute pass must have a exactly one subpass.");
      }
      else
      {
        assert(subpasses.size() >= 1 "A graphics pass must have a atleast one subpass.");
        // TODO(Shareef): More compiling of subpasses.
      }
    }

    template<typename TSetupFn, typename TExecFn>
    Subpass<TData, TExecFn>& addPass(TSetupFn&& setup, TExecFn&& exec_fn)
    {
    }
  };

  struct GraphResourceBase
  {
    NameString<BIFROST_RESOURCE_NAME_LEN> name;
    Vector<RenderpassBase*>               readers;
    Vector<RenderpassBase*>               writers;

    explicit GraphResourceBase(const char* name_in) :
      name{name_in},
      readers{},
      writers{}
    {
    }
  };

  template<typename T, typename TCreate>
  struct GraphResource : public GraphResourceBase
  {
    T data;

    explicit GraphResource(const char* name) :
      GraphResourceBase(name),
      data{}
    {
    }
  };

  template<typename T>
  static inline void deleteList(const Vector<T>& items)
  {
    for (auto* item : items)
    {
      delete item;
    }
  }

  struct BufferDesc;
  struct ImageDesc;

  // TODO(Shareef): Fill this with actual data.
  using bfBuffer       = void*;
  using bfImage        = void*;
  using BufferResource = GraphResource<bfBuffer, BufferDesc>;
  using ImageResource  = GraphResource<bfImage, ImageDesc>;

  struct GraphBuilder final
  {
    FrameGraph*     graph;
    RenderpassBase* pass;

    BufferResource* readBuffer(const char* name, const BufferDesc& desc);
    BufferResource* writeBuffer(const char* name, const BufferDesc& desc);
    ImageResource*  refImage(const char* name, const ImageDesc& desc);
  };

  template<typename TData>
  using ComputePass = Renderpass<TData, true>;

  template<typename TData>
  using GraphicsPass = Renderpass<TData, false>;

  // TODO(Shareef): This should use a custom allocator (Linear Seems appropriate)
  class FrameGraph final
  {
   private:
    Vector<RenderpassBase*>    m_Renderpasses;
    Vector<GraphResourceBase*> m_Resources;  // There should be little amount of resources generally so HashTable may not be needed??
    Vector<GraphBarrier>       m_Barriers;
    Vector<std::uint8_t>       m_Bytecode;
    std::uint8_t*              m_BytecodePos;

   public:
    explicit FrameGraph() :
      m_Renderpasses{},
      m_Resources{}
    {
    }

    void registerBuffer(const char* name, bfBuffer buffer)
    {
    }

    void registerImage(const char* name, bfImage image)
    {
    }

    template<typename TData, typename RSetupFn>
    void addComputePass(const char* name, RSetupFn&& setup_fn)
    {
      // ComputePass<TData>
    }

    template<typename TData, typename RSetupFn>
    void addGraphicsPass(const char* name, RSetupFn&& setup_fn)
    {
      // GraphcisPass<TData>
    }

    void compile()
    {
    }

    void execute()
    {
      m_BytecodePos = m_Bytecode.data();
    }

    ~FrameGraph()
    {
      deleteList(m_Renderpasses);
      deleteList(m_Resources);
    }

   private:
    std::uint32_t bytecodeReadUint32()
    {
      const std::uint8_t* const buffer = m_BytecodePos;

      const std::uint32_t ret = static_cast<std::uint32_t>(
       static_cast<std::uint8_t>(buffer[0]) << 24 |
       static_cast<std::uint8_t>(buffer[1]) << 16 |
       static_cast<std::uint8_t>(buffer[2]) << 8 |
       static_cast<std::uint8_t>(buffer[3]));

      m_BytecodePos += 4;
      return ret;
    }
  };

  // CONCRETE IMPL HERE AFTER THIS POINT ABOVE IS ABSTRACT REUSEABLE 'Library' Code.

  // Could either be read or wrote.
  namespace BufferUsage
  {
    enum
    {
      STORAGE       = bfBit(0),
      UNIFORM       = bfBit(1),
      VERTEX        = bfBit(2),
      INDEX         = bfBit(3),
      UNIFORM       = bfBit(4),
      DRAW_INDIRECT = bfBit(5),
    };

    using type = std::uint8_t;
  };  // namespace BufferUsage

  enum class ImageStage : std::uint8_t
  {
    COMPUTE,   // For Compute passes
    VERTEX,    // For Gfx Passes
    FRAGMENT,  // For Gfx Passes
  };

  // This is practically layout.
  enum class ImageUsage : std::uint8_t
  {
    READ_COLOR,
    WRITE_COLOR,
    READ_DEPTH_READ_STENCIL,
    READ_DEPTH_WRITE_STENCIL,
    WRITE_DEPTH_READ_STENCIL,
    WRITE_DEPTH_WRITE_STENCIL,
    READ_GENERAL,
    WRITE_GENERAL,
  };

  struct BufferDesc final
  {
    typename BufferUsage::type usage  = BufferUsage::STORAGE;
    std::size_t                offset = 0;
    std::size_t                size   = 0;
  };

  enum class ImageSizeType
  {
    SIZE_FRAMEBUFFER_RELATIVE,
    SIZE_ABSOLUTE,
  };

  // format
  // loadOp
  // storeOp
  struct ImageDesc final
  {
    ImageStage    stage       = ImageStage::FRAGMENT;
    ImageUsage    usage       = ImageUsage::WRITE_COLOR;
    float         size_dim[2] = {1.0f, 1.0f}; /* width, height */
    ImageSizeType size_type   = ImageSizeType::SIZE_FRAMEBUFFER_RELATIVE;
    unsigned int  samples     = 1;
    unsigned int  mip_levels  = 1;
    unsigned int  depth       = 1;
  };

}  // namespace bifrost

#if BIFROST_GFX_RENDER_GRAPH_TEST
#include <iostream>

int main()
{
  using namespace std;
  using namespace bifrost;

  std::cout << "Render Pass Prototype BGN\n\n";

  FrameGraph graph;

  struct GBufferData
  {
    ImageResource* outputs[4];
  };

  bfImage physical_resources[4];

  graph.registerImage("g_Pos", physical_resources[0]);
  graph.registerImage("g_Normal", physical_resources[1]);
  graph.registerImage("g_Spec", physical_resources[2]);
  graph.registerImage("g_Mat", physical_resources[3]);

  graph.addGraphicsPass<GBufferData>(
   "GPass",
   [](GraphBuilder& builder, GraphicsPass<GBufferData>& pass, GBufferData& data) {
     ImageDesc pos, normal, spec, mat;

     pos.usage    = ImageUsage::WRITE_COLOR;
     normal.usage = ImageUsage::WRITE_COLOR;
     spec.usage   = ImageUsage::WRITE_COLOR;
     mat.usage    = ImageUsage::WRITE_COLOR;

     data.outputs[0] = builder.refImage("g_Pos", pos);
     data.outputs[1] = builder.refImage("g_Normal", normal);
     data.outputs[2] = builder.refImage("g_Spec", spec);
     data.outputs[3] = builder.refImage("g_Mat", mat);

     pass.addPass(
      [](SubpassBase& subpass, GBufferData& data) {
        subpass.addColorOut(data.outputs[0]);
        subpass.addColorOut(data.outputs[1]);
        subpass.addColorOut(data.outputs[2]);
        subpass.addColorOut(data.outputs[3]);
      },
      [](FrameGraph& graph, const GBufferData& data) {
        // Do Draw Code Here
      });
   });

  graph.compile();
  graph.execute();

  std::cout << "\nRender Pass Prototype END\n";
  return 0;
}
#endif

#endif /* BIFROST_GFX_RENDER_GRAPH_HPP */
