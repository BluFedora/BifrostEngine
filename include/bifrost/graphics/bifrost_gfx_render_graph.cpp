#ifndef BIFROST_GFX_RENDER_GRAPH_HPP
#define BIFROST_GFX_RENDER_GRAPH_HPP

// IF you compile with g++ on windows with a single header you will get an error sayin incpompatible vertion.

// build w: 'g++ -std=c++17 -m32 bifrost_gfx_render_graph.cpp -o RenderGraphTest'

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector> /* vector<T> */

#define PRINT_OUT(c) std::cout << (c) << "\n"

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

  template<typename T>
  static inline void deleteList(const Vector<T>& items)
  {
    for (auto* item : items)
    {
      delete item;
    }
  }

  using String = std::string;

  template<std::size_t N>
  struct NameString final
  {
    char        str[N];
    std::size_t length;

    explicit NameString(const char* str_in) :
      str{'\0'},
      length{0u}
    {
      while (str_in[length] && length < N)
      {
        str[length] = str_in[length];
        ++length;
      }

      str[length] = '\0';
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
    CREATE_BUFFER,
    CREATE_IMAGE,
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
    Vector<GraphResourceBase*> color_refs[2];
    Vector<GraphResourceBase*> depth_refs[2];

    SubpassBase() :
      color_refs{},
      depth_refs{}
    {
    }

    void addColorOut(GraphResourceBase* res)
    {
      color_refs[0].push_back(res);
    }

    void addColorIn(GraphResourceBase* res)
    {
      color_refs[1].push_back(res);
    }

    void addDepthOut(GraphResourceBase* res)
    {
      depth_refs[0].push_back(res);
    }

    void addDepthIn(GraphResourceBase* res)
    {
      depth_refs[1].push_back(res);
    }

    virtual void execute(FrameGraph& graph, void* data) = 0;

    virtual ~SubpassBase() = default;
  };

  template<typename TData, typename TExecFn>
  struct Subpass final : public SubpassBase
  {
    TExecFn exec_fn;

    explicit Subpass(TExecFn&& fn) :
      SubpassBase(),
      exec_fn{std::move(fn)}
    {
    }

    void execute(FrameGraph& graph, void* data) override
    {
      exec_fn(graph, *static_cast<const TData*>(data));
    }
  };

  struct RenderpassBase
  {
    NameString<BIFROST_RENDERPASS_DEBUG_NAME_LEN> name;
    Vector<SubpassBase*>                          subpasses;
    Vector<GraphResourceBase*>                    reads;
    Vector<GraphResourceBase*>                    writes;
    Vector<GraphResourceBase*>                    attachments;
    GraphResourceBase*                            depth_attachment;
    std::size_t                                   queue_family;
    std::size_t                                   barrier_index;
    std::size_t                                   index;
    void* const                                   data_ptr;

    explicit RenderpassBase(const char* name, std::size_t index, void* data_ptr) :
      name{name},
      subpasses{},
      reads{},
      writes{},
      attachments{},
      depth_attachment{nullptr},
      queue_family(-1),
      barrier_index(-1),
      index{index},
      data_ptr{data_ptr}
    {
    }

    virtual void compile(FrameGraph& graph) = 0;

    // This is bad...
    void execute(FrameGraph& graph)
    {
      for (SubpassBase* const pass : subpasses)
      {
        pass->execute(graph, data_ptr);
      }
    }

    virtual ~RenderpassBase()
    {
      deleteList(subpasses);
    }
  };

  template<typename TData, bool is_compute>
  struct Renderpass final : public RenderpassBase
  {
    TData data;

    explicit Renderpass(const char* name, std::size_t index) :
      RenderpassBase(name, index, &data),
      data{}
    {
    }

    void compile(FrameGraph& graph) override
    {
      if constexpr (is_compute)
      {
        assert(subpasses.size() == 1 && "A compute pass must have a exactly one subpass.");

        PRINT_OUT("Compile Compute Pass");
      }
      else
      {
        assert(subpasses.size() >= 1 && "A graphics pass must have a atleast one subpass.");
        // TODO(Shareef): Compiling of subpasses.

        PRINT_OUT("Compile Graphics Pass");
      }
    }

    template<typename TSetupFn, typename TExecFn>
    void addPass(TSetupFn&& setup, TExecFn&& exec_fn)
    {
      auto* const subpass = new Subpass<TData, TExecFn>(std::forward<TExecFn>(exec_fn));
      subpasses.push_back(subpass);
      setup(*subpass, data);
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
  struct GraphResource final : public GraphResourceBase
  {
    T       data;
    TCreate desc;

    explicit GraphResource(const char* name, const T& data) :
      GraphResourceBase(name),
      data{data},
      desc{}
    {
    }
  };

  // Could either be read or wrote.
  namespace BufferUsage
  {
    enum
    {
      STORAGE       = bfBit(0),
      UNIFORM       = bfBit(1),
      VERTEX        = bfBit(2),
      INDEX         = bfBit(3),
      DRAW_INDIRECT = bfBit(4),
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

  // TODO(Shareef): Fill this with actual data.
  using bfBuffer       = void*;
  using bfImage        = void*;
  using BufferResource = GraphResource<bfBuffer, BufferDesc>;
  using ImageResource  = GraphResource<bfImage, ImageDesc>;

  struct GraphBuilder final
  {
    FrameGraph*     graph;
    RenderpassBase* pass;

    BufferResource* createBuffer(const char* name, const BufferDesc& desc);
    BufferResource* readBuffer(const char* name, const BufferDesc& desc);
    BufferResource* writeBuffer(const char* name, const BufferDesc& desc);
    ImageResource*  createImage(const char* name, const ImageDesc& desc);
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
      m_Resources.emplace_back(new BufferResource(name, buffer));
    }

    void registerImage(const char* name, bfImage image)
    {
      m_Resources.emplace_back(new ImageResource(name, image));
    }

    template<typename TData, typename RSetupFn>
    void addComputePass(const char* name, RSetupFn&& setup_fn)
    {
      addPass<ComputePass<TData>, TData>(name, std::forward<RSetupFn>(setup_fn));
    }

    template<typename TData, typename RSetupFn>
    void addGraphicsPass(const char* name, RSetupFn&& setup_fn)
    {
      addPass<GraphicsPass<TData>, TData>(name, std::forward<RSetupFn>(setup_fn));
    }

    void compile()
    {
      std::size_t index = 0;
      for (const auto& pass : m_Renderpasses)
      {
        const auto lastOf = [index](const Vector<RenderpassBase*>& list) -> RenderpassBase* {
          RenderpassBase* ret = nullptr;

          for (auto rp : list)
          {
            if (rp->index >= index)
            {
              break;
            }

            ret = rp;
          }

          return ret;
        };

        // Reads
        {
          // string                     read_barrier;
          // vector<GraphResourceBase*> targets = {};

          for (auto* const res : pass->reads)
          {
            auto* const last_reader_pass = lastOf(res->readers);
            auto* const last_writer_pass = lastOf(res->writers);

            if (last_reader_pass)
            {
              if (last_writer_pass && last_writer_pass->index > last_reader_pass->index)
              {
                // read_barrier = "WRITE -> READ_" + string(res->name);
                // targets.push_back(res);

                PRINT_OUT("WRITE -> READ");
              }
              else
              {
                // auto& barrier = std::get<MemBarrierAction>(actions[last_reader_pass->barrier]);

                // if (barrier.hasTarget(res))
                {
                 // barrier.desc += string("(") + pass->name + "-" + res->name + ")";
                 // pass->barrier = last_reader_pass->barrier;
                }
                // else
                {
                  // read_barrier = "WRITE -> READ_" + string(res->name);
                  // targets.push_back(res);
                }

                PRINT_OUT("WRITE -> READ");
              }
            }
            else
            {
              // read_barrier = "WRITE -> READ_" + string(res->name);
              // targets.push_back(res);
              PRINT_OUT("WRITE -> READ");
            }
          }
        }

        // Writes
        {
          // string                     read_barrier;
          // vector<GraphResourceBase*> targets = {};

          for (auto* const res : pass->writes)
          {
            auto* const last_reader_pass        = lastOf(res->readers);
            auto* const last_writer_pass        = lastOf(res->writers);
            auto* const last_reader_writer_pass = [last_reader_pass, last_writer_pass]() -> RenderpassBase* {
              if (last_reader_pass && last_writer_pass)
              {
                return last_reader_pass->index > last_writer_pass->index ? last_reader_pass : last_writer_pass;
              }

              return last_reader_pass ? last_reader_pass : last_writer_pass;
            }();

            if (last_reader_writer_pass)
            {
              if (last_reader_writer_pass == last_reader_pass)
              {
                // read_barrier = "READ -> WRITE_" + string(write->name);
                PRINT_OUT("READ -> WRITE");
              }
              else
              {
                // read_barrier = "WRITE -> WRITE_" + string(write->name);
                PRINT_OUT("WRITE -> WRITE");
              }

              // targets.push_back(write);
            }
          }
        }

        pass->compile(*this);
        // actions.emplace_back(RenderpassAction(i));

        ++index;
      }
    }

    void execute()
    {
      m_BytecodePos = m_Bytecode.data();
    }

    GraphResourceBase* findResource(const char* name) const
    {
      for (auto* const res : m_Resources)
      {
        if (std::strcmp(name, res->name.str) == 0)
        {
          return res;
        }
      }

      throw "Bad";
      return nullptr;
    }

    ~FrameGraph()
    {
      deleteList(m_Renderpasses);
      deleteList(m_Resources);
    }

   private:
    template<typename PassType, typename TData, typename RSetupFn>
    void addPass(const char* name, RSetupFn&& setup_fn)
    {
      auto* const rp = new GraphicsPass<TData>(name, m_Renderpasses.size());
      m_Renderpasses.push_back(rp);
      GraphBuilder builder{this, rp};
      setup_fn(builder, *rp, rp->data);
    }

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

  template<typename T, typename T2>
  static T* getResource(FrameGraph* graph, const char* name, const T2& desc)
  {
    auto* const res = (T*)graph->findResource(name);
    res->desc       = desc;
    return res;
  }

  static void readResource(RenderpassBase* pass, GraphResourceBase* res)
  {
    res->readers.push_back(pass);
    pass->reads.push_back(res);
  }

  static void writeResource(RenderpassBase* pass, GraphResourceBase* res)
  {
    res->writers.push_back(pass);
    pass->writes.push_back(res);
  }

  BufferResource* GraphBuilder::createBuffer(const char* name, const BufferDesc& desc)
  {
    auto* const res = getResource<BufferResource>(graph, name, desc);
    // This would crash actually.
    // TODO(Shareef): Semantics??
    return res;
  }

  BufferResource* GraphBuilder::readBuffer(const char* name, const BufferDesc& desc)
  {
    auto* const res = getResource<BufferResource>(graph, name, desc);
    readResource(pass, res);
    return res;
  }

  BufferResource* GraphBuilder::writeBuffer(const char* name, const BufferDesc& desc)
  {
    auto* const res = getResource<BufferResource>(graph, name, desc);
    writeResource(pass, res);
    return res;
  }

  ImageResource* GraphBuilder::createImage(const char* name, const ImageDesc& desc)
  {
    auto* const res = getResource<ImageResource>(graph, name, desc);
    // This would crash actually.
    // TODO(Shareef): Semantics??
    return res;
  }

  ImageResource* GraphBuilder::refImage(const char* name, const ImageDesc& desc)
  {
    auto* const res = getResource<ImageResource>(graph, name, desc);

    if (desc.usage == ImageUsage::WRITE_COLOR)
    {
      pass->attachments.push_back(res);
      writeResource(pass, res);
    }
    else if (desc.usage == ImageUsage::WRITE_DEPTH_WRITE_STENCIL || desc.usage == ImageUsage::WRITE_DEPTH_READ_STENCIL || desc.usage == ImageUsage::READ_DEPTH_WRITE_STENCIL)
    {
      assert(pass->depth_attachment == nullptr && "Only one depth attachment per renderpass.");
      pass->depth_attachment = res;
      writeResource(pass, res);
    }
    else if (desc.usage == ImageUsage::WRITE_GENERAL)
    {
      writeResource(pass, res);
    }

    if (desc.usage == ImageUsage::READ_COLOR || desc.usage == ImageUsage::READ_DEPTH_READ_STENCIL || desc.usage == ImageUsage::READ_GENERAL)
    {
      readResource(pass, res);
    }

    return res;
  }
}  // namespace bifrost

#if BIFROST_GFX_RENDER_GRAPH_TEST

int main()
{
  using namespace std;
  using namespace bifrost;

  std::cout << "Render Pass Prototype BGN\n\n";

  FrameGraph graph;

  struct GBufferData
  {
    ImageResource* outputs[5];
  };

  bfImage physical_resources[5];

  graph.registerImage("g_Pos", physical_resources[0]);
  graph.registerImage("g_Normal", physical_resources[1]);
  graph.registerImage("g_Spec", physical_resources[2]);
  graph.registerImage("g_Mat", physical_resources[3]);
  graph.registerImage("g_Depth", physical_resources[4]);

  graph.addGraphicsPass<GBufferData>(
   "GPass",
   [](GraphBuilder& builder, GraphicsPass<GBufferData>& pass, GBufferData& data) {
     ImageDesc pos, normal, spec, mat, depth;

     pos.usage    = ImageUsage::WRITE_COLOR;
     normal.usage = ImageUsage::WRITE_COLOR;
     spec.usage   = ImageUsage::WRITE_COLOR;
     mat.usage    = ImageUsage::WRITE_COLOR;
     depth.usage  = ImageUsage::WRITE_DEPTH_WRITE_STENCIL;

     data.outputs[0] = builder.refImage("g_Pos", pos);
     data.outputs[1] = builder.refImage("g_Normal", normal);
     data.outputs[2] = builder.refImage("g_Spec", spec);
     data.outputs[3] = builder.refImage("g_Mat", mat);
     data.outputs[4] = builder.refImage("g_Depth", depth);

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

  graph.addGraphicsPass<GBufferData>(
   "GPass0",
   [](GraphBuilder& builder, GraphicsPass<GBufferData>& pass, GBufferData& data) {
     ImageDesc pos, normal, spec, mat, depth;

     pos.usage    = ImageUsage::WRITE_COLOR;
     normal.usage = ImageUsage::WRITE_COLOR;
     spec.usage   = ImageUsage::WRITE_COLOR;
     mat.usage    = ImageUsage::WRITE_COLOR;
     depth.usage  = ImageUsage::WRITE_DEPTH_WRITE_STENCIL;

     data.outputs[0] = builder.refImage("g_Pos", pos);
     data.outputs[1] = builder.refImage("g_Normal", normal);
     data.outputs[2] = builder.refImage("g_Spec", spec);
     data.outputs[3] = builder.refImage("g_Mat", mat);
     data.outputs[4] = builder.refImage("g_Depth", depth);

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
