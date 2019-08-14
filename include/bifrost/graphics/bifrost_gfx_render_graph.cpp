// Usage Notes:
//   This Rendergraph will not allocate / create resources for you.
//   To handle transient resources the 'bfGfxCmdList_*' API should be used then registered with the graph.

// TODO(Shareef): Add unused resource culling.
//   THis isn't super high priority since I think that's the job of the person calling these functiuons.

#ifndef BIFROST_GFX_RENDER_GRAPH_HPP
#define BIFROST_GFX_RENDER_GRAPH_HPP

// IF you compile with g++ on windows with a single header you will get an error sayin incpompatible vertion.
// build: g++ -std=c++17 bifrost_gfx_render_graph.cpp -o RenderGraphTest

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector> /* vector<T> */

#define PRINT_OUT(c) std::cout << c << "\n"

#define BIFROST_GFX_RENDER_GRAPH_TEST 1
#include "bifrost_gfx_api.h"

namespace bifrost
{
#ifndef bfBit
#define bfBit(index) (1ULL << (index))
#endif

  // What 64 Characters look like:
  //   AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
  static constexpr unsigned int BIFROST_RENDERPASS_DEBUG_NAME_LEN = 64u;
  static constexpr unsigned int BIFROST_RESOURCE_NAME_LEN         = 128u;
  static constexpr unsigned int BIFROST_SUBPASS_EXTERNAL          = ~0u;
  static constexpr std::size_t  INVALID_BARRIER_IDX               = std::numeric_limits<std::size_t>::max();

  class FrameGraph;
  struct GraphResourceBase;
  struct RenderpassBase;

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

      assert(length < N && "The passed in name was too long.");

      str[length] = '\0';
    }
  };

  enum class BytecodeInst : std::uint8_t
  {
    RENDERPASS,         // [uint32_t : RenderpassIdx, uint32_t : SubpassIdx]
    EXECUTION_BARRIER,  // [uint32_t : ExecBarrierIdx]
    MEMORY_BARRIER,     // [uint32_t : MemBarrierIdx]
    IMAGE_BARRIER,      // [uint32_t : ImageBarrierIdx]
    BUFFER_BARRIER,     // [uint32_t : BufferBarrierIdx]
    NEXT_PASS,          // []
    CREATE_BUFFER,      // [uint32_t : RenderpassIdx, uint32_t : SubpassIdx, uint32_t : ResDescIdx]
    CREATE_IMAGE,       // [uint32_t : RenderpassIdx, uint32_t : SubpassIdx, uint32_t : ResDescIdx]
  };

  enum class BarrierType
  {
    EXECUTION,
    MEMORY,
    IMAGE,
    BUFFER,
    SUBPASS_DEP,
  };

  struct BarrierRef
  {
    BarrierType type  = BarrierType::EXECUTION;
    std::size_t index = INVALID_BARRIER_IDX;

    bool isValid() const
    {
      return index != INVALID_BARRIER_IDX;
    }
  };

  // Can only be merged if this
  // has the same targets AND not "BarrierType::IMAGE" or "BarrierType::BUFFER" also the queues need to match.

  struct BarrierExecution
  {
    BifrostPipelineStageFlags src_stage;
    BifrostPipelineStageFlags dst_stage;

    explicit BarrierExecution(BifrostPipelineStageFlags src, BifrostPipelineStageFlags dst) :
      src_stage{src},
      dst_stage{dst}
    {
    }
  };

  struct BarrierMemory : public BarrierExecution
  {
    BifrostAccessFlags src_access;
    BifrostAccessFlags dst_access;

    explicit BarrierMemory(
     BifrostPipelineStageFlags src_stage,
     BifrostPipelineStageFlags dst_stage,
     BifrostAccessFlags        src,
     BifrostAccessFlags        dst) :
      BarrierExecution(src_stage, dst_stage),
      src_access{src},
      dst_access{dst}
    {
    }
  };

  struct BarrierImage final : public BarrierMemory
  {
    BifrostImageLayout old_layout;
    BifrostImageLayout new_layout;
    std::uint32_t      src_queue;
    std::uint32_t      dst_queue;
    bfTextureHandle    image;  // TODO(Shareef): This isn't exactly right since images can be from Framebuffers aswell.
    std::uint32_t      aspect;
    // BIFROST_IMAGE_ASPECT_COLOR_BIT   = 0x00000001,
    // BIFROST_IMAGE_ASPECT_DEPTH_BIT   = 0x00000002,
    // BIFROST_IMAGE_ASPECT_STENCIL_BIT = 0x00000004,
    std::uint32_t base_mip_level;
    std::uint32_t level_count;
    std::uint32_t base_array_layer;
    std::uint32_t layer_count;
  };

  struct BarrierSubpassDep final : public BarrierMemory
  {
    std::uint32_t src_pass;
    std::uint32_t dst_pass;
    // .dependencyFlags = BIFROST_DEPENDENCY_BY_REGION_BIT,

    explicit BarrierSubpassDep(
     BifrostPipelineStageFlags src_stage,
     BifrostPipelineStageFlags dst_stage,
     BifrostAccessFlags        src_access,
     BifrostAccessFlags        dst_access,
     std::uint32_t             src,
     std::uint32_t             dst) :
      BarrierMemory(src_stage, dst_stage, src_access, dst_access),
      src_pass{src},
      dst_pass{dst}
    {
    }
  };

  struct BarrierBuffer final : public BarrierMemory
  {
    std::uint32_t  src_queue;
    std::uint32_t  dst_queue;
    bfBufferHandle buffer;
    std::uint64_t  offset;
    std::uint64_t  size;
  };

  // Could either be read or write.
  namespace BufferUsage
  {
    enum
    {
      // NOTE(Shareef):
      //   These first two should not be used directly as
      //   they do not specify what shader is using them.
      STORAGE_         = bfBit(0),  // read / write
      UNIFORM_         = bfBit(1),  // read
      VERTEX           = bfBit(2),  // read
      INDEX            = bfBit(3),  // read
      DRAW_INDIRECT    = bfBit(4),  // read
      SHADER_COMPUTE   = bfBit(5),
      SHADER_VERTEX    = bfBit(6),
      SHADER_FRAGMENT  = bfBit(7),
      UNIFORM_COMPUTE  = UNIFORM_ | SHADER_COMPUTE,
      UNIFORM_VERTEX   = UNIFORM_ | SHADER_VERTEX,
      UNIFORM_FRAGMENT = UNIFORM_ | SHADER_FRAGMENT,
      STORAGE_COMPUTE  = STORAGE_ | SHADER_COMPUTE,
      STORAGE_VERTEX   = STORAGE_ | SHADER_VERTEX,
      STORAGE_FRAGMENT = STORAGE_ | SHADER_FRAGMENT,
    };

    using type = std::uint8_t;
  };  // namespace BufferUsage

  enum class PipelineStage : std::uint8_t
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
    typename BufferUsage::type usage  = BufferUsage::STORAGE_COMPUTE;
    std::size_t                offset = 0;
    std::size_t                size   = INVALID_BARRIER_IDX;  // TODO(Shareef): Replace with whole size constant.

    std::uint32_t pipelineStage() const
    {
      std::uint32_t ret = 0x0;

      if (usage & BufferUsage::SHADER_COMPUTE)
      {
        ret |= BIFROST_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      }

      if (usage & BufferUsage::SHADER_VERTEX)
      {
        ret |= BIFROST_PIPELINE_STAGE_VERTEX_SHADER_BIT;
      }

      if (usage & BufferUsage::SHADER_FRAGMENT)
      {
        ret |= BIFROST_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      }

      if (usage & (BufferUsage::VERTEX | BufferUsage::INDEX))
      {
        ret |= BIFROST_PIPELINE_STAGE_VERTEX_INPUT_BIT;
      }

      if (usage & BufferUsage::DRAW_INDIRECT)
      {
        ret |= BIFROST_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
      }

      return ret;
    }

    std::uint32_t accessFlags(bool is_read) const
    {
      std::uint32_t ret = 0x0;

      if (usage & (BufferUsage::STORAGE_COMPUTE | BufferUsage::STORAGE_VERTEX | BufferUsage::STORAGE_FRAGMENT))
      {
        ret |= (is_read ? BIFROST_ACCESS_SHADER_READ_BIT : BIFROST_ACCESS_SHADER_WRITE_BIT);
      }

      if (usage & (BufferUsage::UNIFORM_VERTEX | BufferUsage::UNIFORM_FRAGMENT))
      {
        ret |= BIFROST_ACCESS_UNIFORM_READ_BIT;
      }

      if (usage & (BufferUsage::VERTEX))
      {
        ret |= BIFROST_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
      }

      if (usage & (BufferUsage::INDEX))
      {
        ret |= BIFROST_ACCESS_INDEX_READ_BIT;
      }

      if (usage & (BufferUsage::DRAW_INDIRECT))
      {
        ret |= BIFROST_ACCESS_INDIRECT_COMMAND_READ_BIT;
      }

      return ret;
    }
  };

  struct ImageDesc final
  {
    PipelineStage stage = PipelineStage::FRAGMENT;
    ImageUsage    usage = ImageUsage::WRITE_COLOR;

    std::uint32_t pipelineStage() const
    {
      std::uint32_t ret = 0x0;

      switch (usage)
      {
        case ImageUsage::READ_GENERAL:
        {
          switch (stage)
          {
            case PipelineStage::COMPUTE:
              ret |= BIFROST_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
              break;
            case PipelineStage::VERTEX:
              ret |= BIFROST_PIPELINE_STAGE_VERTEX_SHADER_BIT;
              break;
            case PipelineStage::FRAGMENT:
              ret |= BIFROST_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
              break;
          }
          break;
        }
        case ImageUsage::READ_COLOR:
        case ImageUsage::WRITE_COLOR:
        {
          ret |= BIFROST_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
          break;
        }
        case ImageUsage::READ_DEPTH_READ_STENCIL:
        case ImageUsage::READ_DEPTH_WRITE_STENCIL:
        case ImageUsage::WRITE_DEPTH_READ_STENCIL:
        case ImageUsage::WRITE_DEPTH_WRITE_STENCIL:
        {
          ret |= BIFROST_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | BIFROST_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
          break;
        }
        case ImageUsage::WRITE_GENERAL:
        {
          break;
        }
      }

      return ret;
    }

    std::uint32_t accessFlags() const
    {
      std::uint32_t ret = 0x0;

      switch (usage)
      {
        case ImageUsage::READ_COLOR:
        {
          ret |= BIFROST_ACCESS_COLOR_ATTACHMENT_READ_BIT;
          break;
        }
        case ImageUsage::WRITE_COLOR:
        {
          ret |= BIFROST_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
          break;
        }
        case ImageUsage::READ_DEPTH_READ_STENCIL:
        {
          ret |= BIFROST_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
          break;
        }
        case ImageUsage::READ_DEPTH_WRITE_STENCIL:
        {
          ret |= BIFROST_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | BIFROST_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
          break;
        }
        case ImageUsage::WRITE_DEPTH_READ_STENCIL:
        {
          ret |= BIFROST_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | BIFROST_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
          break;
        }
        case ImageUsage::WRITE_DEPTH_WRITE_STENCIL:
        {
          ret |= BIFROST_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | BIFROST_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
          break;
        }
        case ImageUsage::READ_GENERAL:
        {
          ret |= BIFROST_ACCESS_SHADER_READ_BIT;
          break;
        }
        case ImageUsage::WRITE_GENERAL:
        {
          ret |= BIFROST_ACCESS_SHADER_WRITE_BIT;
          break;
        }
      }

      return ret;
    }

    BifrostImageLayout imageLayout() const
    {
      switch (usage)
      {
        case ImageUsage::READ_COLOR: return BIFROST_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case ImageUsage::WRITE_COLOR: return BIFROST_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case ImageUsage::READ_DEPTH_READ_STENCIL: return BIFROST_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        case ImageUsage::READ_DEPTH_WRITE_STENCIL: return BIFROST_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
        case ImageUsage::WRITE_DEPTH_READ_STENCIL: return BIFROST_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
        case ImageUsage::WRITE_DEPTH_WRITE_STENCIL: return BIFROST_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case ImageUsage::READ_GENERAL: return BIFROST_IMAGE_LAYOUT_GENERAL;
        case ImageUsage::WRITE_GENERAL: return BIFROST_IMAGE_LAYOUT_GENERAL;
      }

      return BIFROST_IMAGE_LAYOUT_GENERAL;
    }
  };

  enum class ResourceType
  {
    BUFFER,
    IMAGE,
  };

  struct GraphResourceBase
  {
    ResourceType                          type;
    NameString<BIFROST_RESOURCE_NAME_LEN> name;
    Vector<RenderpassBase*>               readers;
    Vector<RenderpassBase*>               writers;

    explicit GraphResourceBase(ResourceType type, const char* name) :
      type{type},
      name{name},
      readers{},
      writers{}
    {
    }
  };

  // TODO(Shareef): This was a bad generic abstraction.
  //   A Framegraph Absolutlely cannot be reused by another app
  //   since the barriers require pretty intimate knowlege of
  //   the way resoucres are acceses.

  template<typename T>
  struct GraphResource : public GraphResourceBase
  {
    T data;

    explicit GraphResource(ResourceType type, const char* name, const T& data) :
      GraphResourceBase(type, name),
      data{data}
    {
    }
  };

  struct BufferResource final : public GraphResource<bfBufferHandle>
  {
    explicit BufferResource(const char* name, const bfBufferHandle& data) :
      GraphResource(ResourceType::BUFFER, name, data)
    {
    }
  };

  struct ImageResource final : public GraphResource<bfTextureHandle>
  {
    explicit ImageResource(const char* name, const bfTextureHandle& data) :
      GraphResource(ResourceType::IMAGE, name, data)
    {
    }
  };

  struct ResourceRef final
  {
    static ResourceRef create(BufferResource* resource, const BufferDesc& desc, bool is_read)
    {
      ResourceRef ret;
      ret.pipeline_stage_flags = desc.pipelineStage();
      ret.access_flags         = desc.accessFlags(is_read);
      // ret.image_layout = N/A;
      ret.as.buffer = resource;
      return ret;
    }

    // TODO(Shareef): Find somethign better than the comemnt below. (Maybe add is_read to the actual BufferDesc)
    // NOTE(Shareef): The unused 'is_read' is so these two create functions can
    //   use deduction for which one to use.
    static ResourceRef create(ImageResource* resource, const ImageDesc& desc, bool /*is_read*/)
    {
      ResourceRef ret;
      ret.pipeline_stage_flags = desc.pipelineStage();
      ret.access_flags         = desc.accessFlags();
      ret.image_layout         = desc.imageLayout();
      ret.as.image             = resource;
      return ret;
    }

    std::uint32_t      pipeline_stage_flags;
    BifrostImageLayout image_layout;
    std::uint32_t      access_flags;

    // TODO(Shareef): See if I can make the pointer const.
    union
    {
      GraphResourceBase* resource;
      BufferResource*    buffer;
      ImageResource*     image;
    } as;

    GraphResourceBase* operator->() const
    {
      return as.resource;
    }

    GraphResourceBase& operator*() const
    {
      return *as.resource;
    }
  };

  struct SubpassBase
  {
    // protected:

    RenderpassBase&        parent;
    Vector<ImageResource*> refs[2];
    std::size_t            index;
    Vector<BarrierRef>     subpass_deps;

    SubpassBase(RenderpassBase& parent, std::size_t idx) :
      parent{parent},
      refs{},
      index{idx},
      subpass_deps{}
    {
    }

    const Vector<ImageResource*>& writes() const
    {
      return refs[0];
    }

    const Vector<ImageResource*>& reads() const
    {
      return refs[1];
    }

    virtual void execute(FrameGraph& graph, void* data) = 0;

    BarrierRef& getBarrier(std::size_t index)
    {
      subpass_deps.resize(std::max(index + 1u, subpass_deps.size()));
      return subpass_deps[index];
    }

    virtual ~SubpassBase() = default;

   public:
    void refAttachment(std::int32_t attachment_index, PipelineStage stage, ImageUsage usage);
  };

  template<typename TData, typename TExecFn>
  struct Subpass final : public SubpassBase
  {
    // protected:
    TExecFn exec_fn;

    explicit Subpass(RenderpassBase& parent, TExecFn&& fn, size_t index) :
      SubpassBase(parent, index),
      exec_fn{std::move(fn)}
    {
    }

    // TODO(Shareef): This being virtual is exclsusively for the type safety.
    //   Can maybe replace it with a function pointer with a template.
    //   This will lead to a smaller runtime since we only need
    //

    void execute(FrameGraph& graph, void* data) override
    {
      exec_fn(graph, *static_cast<const TData*>(data));
    }
  };

  struct RenderpassBase
  {
    friend class FrameGraph;
    friend class SubpassBase;

    template<typename T>
    friend inline void deleteList(const Vector<T>& items);

    template<typename TRes, typename TDesc>
    friend void readResource(RenderpassBase* pass, TRes* res, const TDesc& desc);

    template<typename TRes, typename TDesc>
    friend void writeResource(RenderpassBase* pass, TRes* res, const TDesc& desc);

   protected:
    FrameGraph&                                   parent;
    NameString<BIFROST_RENDERPASS_DEBUG_NAME_LEN> name;
    Vector<SubpassBase*>                          subpasses;
    Vector<ResourceRef>                           reads;
    Vector<ResourceRef>                           writes;
    Vector<ImageResource*>                        attachments;
    ImageResource*                                depth_attachment;
    std::size_t                                   queue_family;
    std::size_t                                   barrier_index;
    std::size_t                                   index;
    void* const                                   data_ptr;
    bfLoadStoreFlags                              load_ops[2];       /*!< The bit being set means you want to load the data.  (if no load or clear dont care is implied) */
    bfLoadStoreFlags                              load_clear_ops[2]; /*!< The bit being set means you want to clear the data. (if no load or clear dont care is implied) */
    bfLoadStoreFlags                              store_ops[2];      /*!< The bit being set means you want to store the data.                                            */
    bool                                          is_compute;

    explicit RenderpassBase(FrameGraph& parent, const char* name, std::size_t index, void* data_ptr, bool is_compute) :
      parent{parent},
      name{name},
      subpasses{},
      reads{},
      writes{},
      attachments{},
      depth_attachment{nullptr},
      queue_family(-1),
      barrier_index(-1),
      index{index},
      data_ptr{data_ptr},
      load_ops{0x0, 0x0},
      load_clear_ops{0x0, 0x0},
      store_ops{0x0, 0x0},
      is_compute{is_compute}
    {
    }

    void compile(FrameGraph& graph);

    virtual ~RenderpassBase()
    {
      deleteList(subpasses);
    }

   public:
    BufferResource* readBuffer(const char* name, const BufferDesc& desc);
    BufferResource* writeBuffer(const char* name, const BufferDesc& desc);
    ImageResource*  addColorAttachment(const char* name);
    ImageResource*  addDepthAttachment(const char* name);
  };

  template<typename TData>
  struct Renderpass final : public RenderpassBase
  {
    friend class FrameGraph;

   protected:
    TData data;

    explicit Renderpass(FrameGraph& parent, const char* name, std::size_t index, bool is_compute) :
      RenderpassBase(parent, name, index, &data, is_compute),
      data{}
    {
    }

   public:
    template<typename TSetupFn, typename TExecFn>
    void addPass(TSetupFn&& setup, TExecFn&& exec_fn)
    {
      auto* const subpass = new Subpass<TData, TExecFn>(*this, std::forward<TExecFn>(exec_fn), subpasses.size());
      subpasses.push_back(subpass);
      setup(*subpass, data);
    }
  };

  // TODO(Shareef): This should use a custom allocator (Linear Seems appropriate)
  class FrameGraph final
  {
   private:
    Vector<RenderpassBase*>    m_Renderpasses;
    Vector<GraphResourceBase*> m_Resources;  // There should be little amount of resources generally so HashTable may not be needed??
    Vector<std::uint8_t>       m_Bytecode;
    std::uint8_t*              m_BytecodePos;
    Vector<BarrierExecution>   m_ExecutionBarriers;
    Vector<BarrierMemory>      m_MemoryBarriers;
    Vector<BarrierImage>       m_ImageBarriers;
    Vector<BarrierSubpassDep>  m_SubpassBarriers;

   public:
    explicit FrameGraph() :
      m_Renderpasses{},
      m_Resources{}
    {
    }

    void clear()
    {
      deleteList(m_Renderpasses);
      deleteList(m_Resources);
      m_Bytecode.clear();
      m_BytecodePos = nullptr;
      m_ExecutionBarriers.clear();
      m_MemoryBarriers.clear();
      m_ImageBarriers.clear();
      m_SubpassBarriers.clear();
    }

    void registerResource(const char* name, bfBufferHandle buffer)
    {
      m_Resources.emplace_back(new BufferResource(name, buffer));
    }

    void registerResource(const char* name, bfTextureHandle image)
    {
      m_Resources.emplace_back(new ImageResource(name, image));
    }

    template<typename TData, typename RSetupFn>
    void addComputePass(const char* name, RSetupFn&& setup_fn)
    {
      addPass<TData>(name, std::forward<RSetupFn>(setup_fn), true);
    }

    template<typename TData, typename RSetupFn>
    void addGraphicsPass(const char* name, RSetupFn&& setup_fn)
    {
      addPass<TData>(name, std::forward<RSetupFn>(setup_fn), false);
    }

    void compile()
    {
      struct BarrierRefEXT : public BarrierRef
      {
        // TempList Would be great here.
        Vector<GraphResourceBase*> targets = {};

        void copyFrom(const BarrierRef& b)
        {
          this->type  = b.type;
          this->index = b.index;
        }

        bool hasTarget(const GraphResourceBase* target) const
        {
          const auto end = targets.cend();
          const auto it  = std::find(targets.cbegin(), end, target);
          return it != end;
        }
      };

      // TempList Would be great here too.
      Vector<BarrierRefEXT> barriers = {};

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

          for (const auto& res : pass->reads)
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

          for (const auto& res : pass->writes)
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

    BarrierRef addSubpassBarrier(const BarrierSubpassDep& dep)
    {
      BarrierRef ret = {BarrierType::SUBPASS_DEP, m_SubpassBarriers.size()};
      m_SubpassBarriers.push_back(dep);
      return ret;
    }

    ~FrameGraph()
    {
      clear();
    }

   private:
    template<typename TData, typename RSetupFn>
    void addPass(const char* name, RSetupFn&& setup_fn, bool is_compute)
    {
      auto* const rp = new Renderpass<TData>(*this, name, m_Renderpasses.size(), is_compute);
      m_Renderpasses.push_back(rp);
      setup_fn(*rp, rp->data);
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

  template<typename T>
  static T* getResource(FrameGraph* graph, const char* name)
  {
    return (T*)graph->findResource(name);
  }

  template<typename TRes, typename TDesc>
  static void readResource(RenderpassBase* pass, TRes* res, const TDesc& desc)
  {
    res->readers.push_back(pass);
    pass->reads.push_back(ResourceRef::create(res, desc, true));
  }

  template<typename TRes, typename TDesc>
  static void writeResource(RenderpassBase* pass, TRes* res, const TDesc& desc)
  {
    res->writers.push_back(pass);
    pass->writes.push_back(ResourceRef::create(res, desc, false));
  }

  // -1 means the depth attachment
  void SubpassBase::refAttachment(std::int32_t attachment_index, PipelineStage stage, ImageUsage usage)
  {
    const ImageDesc desc{stage, usage};
    auto* const     res = (attachment_index == -1) ? parent.depth_attachment : parent.attachments[attachment_index];

    switch (usage)
    {
      case ImageUsage::WRITE_GENERAL:
      case ImageUsage::WRITE_COLOR:
      case ImageUsage::WRITE_DEPTH_WRITE_STENCIL:
      case ImageUsage::WRITE_DEPTH_READ_STENCIL:
      case ImageUsage::READ_DEPTH_WRITE_STENCIL:
      {
        writeResource(&parent, res, desc);
        break;
      }
      case ImageUsage::READ_COLOR:
      case ImageUsage::READ_DEPTH_READ_STENCIL:
      case ImageUsage::READ_GENERAL:
      {
        readResource(&parent, res, desc);
        break;
      }
    }

    switch (usage)
    {
      case ImageUsage::WRITE_COLOR:
      {
        refs[0].push_back(res);
        break;
      }
      case ImageUsage::READ_COLOR:
      {
        refs[1].push_back(res);
        break;
      }
      case ImageUsage::WRITE_DEPTH_WRITE_STENCIL:
      case ImageUsage::WRITE_DEPTH_READ_STENCIL:
      case ImageUsage::READ_DEPTH_WRITE_STENCIL:
      {
        assert(attachment_index == -1 && "Can only use depth attachment for depth usage.");
        refs[0].push_back(res);
        break;
      }
      case ImageUsage::READ_DEPTH_READ_STENCIL:
      {
        assert(attachment_index == -1 && "Can only use depth attachment for depth usage.");
        refs[1].push_back(res);
        break;
      }
      case ImageUsage::WRITE_GENERAL:
      case ImageUsage::READ_GENERAL:
      {
        assert(parent.is_compute && stage == PipelineStage::COMPUTE && "General reads and writes are exclusive to compute passes.");
        break;
      }
    }
  }

  BufferResource* RenderpassBase::readBuffer(const char* name, const BufferDesc& desc)
  {
    auto* const res = getResource<BufferResource>(&parent, name);
    readResource(this, res, desc);
    return res;
  }

  BufferResource* RenderpassBase::writeBuffer(const char* name, const BufferDesc& desc)
  {
    auto* const res = getResource<BufferResource>(&parent, name);
    writeResource(this, res, desc);
    return res;
  }

  ImageResource* RenderpassBase::addColorAttachment(const char* name)
  {
    auto* const res = getResource<ImageResource>(&parent, name);
    this->attachments.push_back(res);
    return res;
  }

  ImageResource* RenderpassBase::addDepthAttachment(const char* name)
  {
    auto* const res = getResource<ImageResource>(&parent, name);
    assert(this->depth_attachment == nullptr && "Only one depth attachment per renderpass.");
    this->depth_attachment = res;
    return res;
  }

  void RenderpassBase::compile(FrameGraph& graph)
  {
    assert(subpasses.size() >= 1 && "A render pass must have a atleast one subpass.");

    if (is_compute)
    {
      PRINT_OUT("Compile Compute Pass");
      PRINT_OUT("Compile SubPass" << 0);
    }
    else
    {
      PRINT_OUT("Compile Graphics Pass: '" << name.str << "'");

      const auto num_subpasses = subpasses.size();

      for (size_t index = 1; index < num_subpasses; ++index)
      {
        auto* const pass = subpasses[index];

        const auto addBarrier = [index, pass, &graph](const Vector<RenderpassBase*>& renderpass_list) {
          for (auto* const renderpass : renderpass_list)
          {
            if (renderpass->index < index)
            {
              auto& barrier = pass->getBarrier(renderpass->index);

              if (!barrier.isValid())
              {
                // TODO(Shareef): Incorrect paramters.
                barrier = graph.addSubpassBarrier(
                 BarrierSubpassDep(
                  BIFROST_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                  BIFROST_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                  BIFROST_ACCESS_INDIRECT_COMMAND_READ_BIT,
                  BIFROST_ACCESS_INDIRECT_COMMAND_READ_BIT,
                  renderpass->index,
                  index));

                PRINT_OUT("Subpass Dep: " << renderpass->index << " -> " << index);
              }
              else
              {
                PRINT_OUT("Upgrade Older Barrier.");
              }
            }
          }
        };

        PRINT_OUT("Compile SubPass" << index);

        for (auto* const read_res : pass->reads())
        {
          addBarrier(read_res->writers);
        }

        for (auto* const write_res : pass->writes())
        {
          addBarrier(write_res->writers);
          addBarrier(write_res->readers);
        }
      }
    }
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

  bfTextureHandle physical_resources[5];

  // Additionally Needed Data(Can be handled by a better framebuffer / texture intertop abstraction):
  //   format           : Gotten by the texture.
  //   samples          : Gotten by the texture.
  //   initial_layout   : last layout it was in or VK_IMAGE_LAYOUT_UNDEFINED.
  //     optimization: Always VK_IMAGE_LAYOUT_UNDEFINED if we loadOp::Clear or loadOpDontCare
  graph.registerResource("g_Pos", physical_resources[0]);
  graph.registerResource("g_Normal", physical_resources[1]);
  graph.registerResource("g_Spec", physical_resources[2]);
  graph.registerResource("g_Mat", physical_resources[3]);
  graph.registerResource("g_Depth", physical_resources[4]);

  graph.addGraphicsPass<GBufferData>(
   "GPass",
   [](Renderpass<GBufferData>& pass, GBufferData& data) {
     // Additionally Needed Data:
     //   load_op          : can be flags
     //   store_op         : can be flags
     //   stencil_load_op  : can be flags
     //   stencil_store_op : can be flags
     //   final_layout     : Ok easy to specify ithout the retarded 'initial_layout' part

     data.outputs[0] = pass.addColorAttachment("g_Pos");
     data.outputs[1] = pass.addColorAttachment("g_Normal");
     data.outputs[2] = pass.addColorAttachment("g_Spec");
     data.outputs[3] = pass.addColorAttachment("g_Mat");
     data.outputs[4] = pass.addColorAttachment("g_Depth");

     pass.addPass(
      [](SubpassBase& subpass, GBufferData& data) {
        subpass.refAttachment(0, PipelineStage::FRAGMENT, ImageUsage::WRITE_COLOR);
        subpass.refAttachment(1, PipelineStage::FRAGMENT, ImageUsage::WRITE_COLOR);
        subpass.refAttachment(2, PipelineStage::FRAGMENT, ImageUsage::WRITE_COLOR);
        subpass.refAttachment(3, PipelineStage::FRAGMENT, ImageUsage::WRITE_COLOR);
      },
      [](FrameGraph& graph, const GBufferData& data) {
        // Do Draw Code Here
      });
   });

  graph.addGraphicsPass<GBufferData>(
   "GPass0",
   [](Renderpass<GBufferData>& pass, GBufferData& data) {
     data.outputs[0] = pass.addColorAttachment("g_Pos");
     data.outputs[1] = pass.addColorAttachment("g_Normal");
     data.outputs[2] = pass.addColorAttachment("g_Spec");
     data.outputs[3] = pass.addColorAttachment("g_Mat");
     data.outputs[4] = pass.addColorAttachment("g_Depth");

     // Main G Pass
     pass.addPass(
      [](SubpassBase& subpass, GBufferData& data) {
        subpass.refAttachment(0, PipelineStage::FRAGMENT, ImageUsage::WRITE_COLOR);
        subpass.refAttachment(1, PipelineStage::FRAGMENT, ImageUsage::WRITE_COLOR);
        subpass.refAttachment(2, PipelineStage::FRAGMENT, ImageUsage::WRITE_COLOR);
        subpass.refAttachment(3, PipelineStage::FRAGMENT, ImageUsage::WRITE_COLOR);
      },
      [](FrameGraph& graph, const GBufferData& data) {
        // Do Draw Code Here
      });

     // Lighting Pass
     pass.addPass(
      [](SubpassBase& subpass, GBufferData& data) {
        subpass.refAttachment(0, PipelineStage::FRAGMENT, ImageUsage::READ_COLOR);
        subpass.refAttachment(1, PipelineStage::FRAGMENT, ImageUsage::READ_COLOR);
        subpass.refAttachment(2, PipelineStage::FRAGMENT, ImageUsage::WRITE_COLOR);
        subpass.refAttachment(3, PipelineStage::FRAGMENT, ImageUsage::READ_COLOR);
      },
      [](FrameGraph& graph, const GBufferData& data) {
        // Do Draw Code Here
      });

     // Extra Pass
     pass.addPass(
      [](SubpassBase& subpass, GBufferData& data) {
        subpass.refAttachment(0, PipelineStage::FRAGMENT, ImageUsage::READ_COLOR);
        subpass.refAttachment(1, PipelineStage::FRAGMENT, ImageUsage::READ_COLOR);
        subpass.refAttachment(2, PipelineStage::FRAGMENT, ImageUsage::READ_COLOR);
        subpass.refAttachment(3, PipelineStage::FRAGMENT, ImageUsage::READ_COLOR);
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
