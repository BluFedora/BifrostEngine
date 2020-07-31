// Usage Notes:
//   > This Rendergraph will not allocate / create resources for you.
//     To handle transient resources the 'bfGfxCmdList_*' API should be used then registered with the graph.
//   > This will only handle the most basic sync needed.
//     Manual Queue transfer will be needed to

// TODO(Shareef): Add unused resource culling.
//   This isn't super high priority since I think that's the job of the person calling these functiuons.

/*
 Subpasss Notes:
  Input attachments can only be used in the Fragment Shader. 
  The Input attachment must also be bound with the apporpriate descriptor set.
*/

#ifndef BIFROST_GFX_RENDER_GRAPH_HPP
#define BIFROST_GFX_RENDER_GRAPH_HPP

#pragma warning(disable : 26812)

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector> /* vector<T> */

#define PRINT_OUT(c) std::cout << c << "\n"

#define BIFROST_GFX_RENDER_GRAPH_TEST 1
#include "bifrost_gfx_api.h"

namespace bf
{
  // What 64 Characters look like:
  //   AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
  static constexpr unsigned int  BIFROST_RENDERPASS_DEBUG_NAME_LEN = 64u;
  static constexpr unsigned int  BIFROST_RESOURCE_NAME_LEN         = 128u;
  static constexpr std::uint32_t INVALID_BARRIER_IDX               = std::numeric_limits<std::uint32_t>::max();

  class RenderGraph;
  struct GraphResourceBase;
  struct RenderpassBase;

  template<typename T>
  using Vector = std::vector<T>;

  template<typename T>
  static void deleteList(const Vector<T>& items)
  {
    for (auto* item : items)
    {
      delete item;
    }
  }

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
    RENDERPASS,         // [uint32_t : RenderpassIdx]
    EXECUTION_BARRIER,  // [uint32_t : ExecBarrierIdx]
    MEMORY_BARRIER,     // [uint32_t : MemBarrierIdx]
  };

  enum class BarrierType
  {
    EXECUTION,
    MEMORY,
    SUBPASS_DEP,
  };

  struct BarrierRef
  {
    BarrierType   type  = BarrierType::EXECUTION;
    std::uint32_t index = INVALID_BARRIER_IDX;

    [[nodiscard]] bool isValid() const
    {
      return index != INVALID_BARRIER_IDX;
    }
  };

  // Can only be merged if this
  // has the same targets AND not "BarrierType::IMAGE" or "BarrierType::BUFFER" also the queues need to match.

  struct BarrierExecution
  {
    std::uint32_t src_stage;  // BifrostPipelineStageFlags
    std::uint32_t dst_stage;  // BifrostPipelineStageFlags

    explicit BarrierExecution(std::uint32_t src, std::uint32_t dst) :
      src_stage{src},
      dst_stage{dst}
    {
    }
  };

  struct BarrierMemory : BarrierExecution
  {
    std::uint32_t src_access;  // BifrostAccessFlags
    std::uint32_t dst_access;  // BifrostAccessFlags

    explicit BarrierMemory(
     std::uint32_t src_stage,
     std::uint32_t dst_stage,
     std::uint32_t src,
     std::uint32_t dst) :
      BarrierExecution(src_stage, dst_stage),
      src_access{src},
      dst_access{dst}
    {
    }
  };

  //*
  struct BarrierImage final : public BarrierMemory
  {
    BifrostImageLayout old_layout;
    BifrostImageLayout new_layout;
    std::uint32_t      src_queue;
    std::uint32_t      dst_queue;
    bfTextureHandle    image;
    // std::uint32_t      aspect;
    std::uint32_t base_mip_level;
    std::uint32_t level_count;
    std::uint32_t base_array_layer;
    std::uint32_t layer_count;
  };
  //*/

  struct BarrierSubpassDep final : public BarrierMemory
  {
    std::uint32_t src_pass;
    std::uint32_t dst_pass;
    // .dependencyFlags = BIFROST_DEPENDENCY_BY_REGION_BIT,

    explicit BarrierSubpassDep(
     std::uint32_t src_stage,
     std::uint32_t dst_stage,
     std::uint32_t src_access,
     std::uint32_t dst_access,
     std::uint32_t src,
     std::uint32_t dst) :
      BarrierMemory(src_stage, dst_stage, src_access, dst_access),
      src_pass{src},
      dst_pass{dst}
    {
    }
  };

  //*
  struct BarrierBuffer final : public BarrierMemory
  {
    std::uint32_t  src_queue;
    std::uint32_t  dst_queue;
    bfBufferHandle buffer;
    std::uint64_t  offset;
    std::uint64_t  size;
  };
  //*/

  // Could either be read or write.
  namespace BufferUsage
  {
    enum
    {
      // NOTE(Shareef):
      //   These first two should not be used directly as
      //   they do not specify what shader is using them.
      STORAGE_         = bfBit(0),                    // read / write
      UNIFORM_         = bfBit(1),                    // read
      VERTEX           = bfBit(2),                    // read
      INDEX            = bfBit(3),                    // read
      DRAW_INDIRECT    = bfBit(4),                    // read
      SHADER_COMPUTE   = bfBit(5),                    // read / write
      SHADER_VERTEX    = bfBit(6),                    // read / write
      SHADER_FRAGMENT  = bfBit(7),                    // read / write
      SHADER_TRANSFER  = bfBit(8),                    // read / write (Not really a shader, just handles host transfer cases)
      UNIFORM_COMPUTE  = UNIFORM_ | SHADER_COMPUTE,   // read / write
      UNIFORM_VERTEX   = UNIFORM_ | SHADER_VERTEX,    // read / write
      UNIFORM_FRAGMENT = UNIFORM_ | SHADER_FRAGMENT,  // read / write
      STORAGE_COMPUTE  = STORAGE_ | SHADER_COMPUTE,   // read / write
      STORAGE_VERTEX   = STORAGE_ | SHADER_VERTEX,    // read / write
      STORAGE_FRAGMENT = STORAGE_ | SHADER_FRAGMENT,  // read / write
    };

    using type = std::uint16_t;
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
    BufferUsage::type usage  = BufferUsage::STORAGE_COMPUTE;
    bfBufferSize      offset = 0;
    bfBufferSize      size   = BIFROST_BUFFER_WHOLE_SIZE;

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

      if (usage & BufferUsage::SHADER_TRANSFER)
      {
        ret |= BIFROST_PIPELINE_STAGE_TRANSFER_BIT;
      }

      return ret;
    }

    std::uint32_t accessFlags(bool is_read) const
    {
      std::uint32_t ret = 0x0;

      if (usage & (BufferUsage::STORAGE_COMPUTE | BufferUsage::STORAGE_VERTEX | BufferUsage::STORAGE_FRAGMENT))
      {
        ret |= is_read ? BIFROST_ACCESS_SHADER_READ_BIT : BIFROST_ACCESS_SHADER_WRITE_BIT;
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

      if (usage & (BufferUsage::SHADER_TRANSFER))
      {
        ret |= is_read ? BIFROST_ACCESS_TRANSFER_READ_BIT : BIFROST_ACCESS_TRANSFER_WRITE_BIT;
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

  struct GraphResourceBase
  {
    NameString<BIFROST_RESOURCE_NAME_LEN> name;
    Vector<RenderpassBase*>               readers;
    Vector<RenderpassBase*>               writers;

    explicit GraphResourceBase(const char* name) :
      name{name},
      readers{},
      writers{}
    {
    }
  };

  // TODO(Shareef): This was a bad generic abstraction.
  //   A Framegraph Absolutlely cannot be reused by another app
  //   since the barriers require pretty intimate knowlege of
  //   the way resoucres are accessed.

  template<typename T>
  struct GraphResource : public GraphResourceBase
  {
    T data;

    explicit GraphResource(const char* name, const T& data) :
      GraphResourceBase(name),
      data{data}
    {
    }
  };

  struct BufferResource final : public GraphResource<bfBufferHandle>
  {
    explicit BufferResource(const char* name, const bfBufferHandle& data) :
      GraphResource(name, data)
    {
    }
  };

  struct ImageResource final : public GraphResource<bfTextureHandle>
  {
    explicit ImageResource(const char* name, const bfTextureHandle& data) :
      GraphResource(name, data)
    {
    }
  };

  struct ResourceRef final
  {
    static ResourceRef create(BufferResource* resource, const BufferDesc& desc, bool is_read)
    {
      ResourceRef ret{};
      ret.pipeline_stage_flags = desc.pipelineStage();
      ret.access_flags         = desc.accessFlags(is_read);
      // ret.image_layout = N/A;
      ret.as.buffer = resource;
      return ret;
    }

    // TODO(Shareef): Find something better than the comment below. (Maybe add is_read to the actual BufferDesc)
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

    RenderpassBase&     parent;
    Vector<ResourceRef> refs[2];
    std::size_t         index;
    Vector<BarrierRef>  subpass_deps;

    SubpassBase(RenderpassBase& parent, std::size_t idx) :
      parent{parent},
      refs{},
      index{idx},
      subpass_deps{}
    {
    }

    [[nodiscard]] const Vector<ResourceRef>& writes() const
    {
      return refs[0];
    }

    [[nodiscard]] const Vector<ResourceRef>& reads() const;

    virtual void execute(RenderGraph& graph, void* data) = 0;

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

    void execute(RenderGraph& graph, void* data) override
    {
      exec_fn(graph, *static_cast<const TData*>(data));
    }
  };

  struct RenderpassBase
  {
    friend class RenderGraph;
    friend struct SubpassBase;

    template<typename T>
    friend void deleteList(const Vector<T>& items);

    template<typename TRes, typename TDesc>
    friend ResourceRef readResource(RenderpassBase* pass, TRes* res, const TDesc& desc);

    template<typename TRes, typename TDesc>
    friend ResourceRef writeResource(RenderpassBase* pass, TRes* res, const TDesc& desc);

   protected:
    RenderGraph&                                  parent;
    NameString<BIFROST_RENDERPASS_DEBUG_NAME_LEN> name;
    Vector<SubpassBase*>                          subpasses;
    Vector<ResourceRef>                           reads;
    Vector<ResourceRef>                           writes;
    Vector<ImageResource*>                        attachments;
    std::size_t                                   queue_family;
    BarrierRef                                    barrier_ref;  // TODO: Find a way to only have this variable while compiling.
    std::uint32_t                                 index;
    void* const                                   data_ptr;
    bool                                          is_compute;

    explicit RenderpassBase(RenderGraph& parent, const char* name, std::uint32_t index, void* data_ptr, bool is_compute) :
      parent{parent},
      name{name},
      subpasses{},
      reads{},
      writes{},
      attachments{},
      queue_family(-1),
      barrier_ref{},
      index{index},
      data_ptr{data_ptr},
      is_compute{is_compute}
    {
    }

    void compile(RenderGraph& graph);

    virtual ~RenderpassBase()
    {
      deleteList(subpasses);
    }

   public:
    BufferResource*    readBuffer(const char* name, const BufferDesc& desc);
    BufferResource*    writeBuffer(const char* name, const BufferDesc& desc);
    ImageResource*     addAttachment(const char* name, BifrostImageLayout final_layout, bfBool32 may_alias);
    const ResourceRef& findRef(GraphResourceBase* ptr, bool is_read) const
    {
      auto& list = is_read ? reads : writes;

      for (const auto& ref : list)
      {
        if (ref.as.resource == ptr)
        {
          return ref;
        }
      }

      throw "Could not find resource ref";
    }
  };

  template<typename TData>
  struct Renderpass final : public RenderpassBase
  {
    friend class RenderGraph;

   protected:
    TData data;

    explicit Renderpass(RenderGraph& parent, const char* name, std::uint32_t index, bool is_compute) :
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
  class RenderGraph final
  {
    friend struct RenderpassBase;

   private:
    Vector<RenderpassBase*>    m_Renderpasses;
    Vector<GraphResourceBase*> m_Resources;  // There should be little amount of resources generally so HashTable may not be needed??
    Vector<std::uint8_t>       m_Bytecode;
    std::uint8_t*              m_BytecodePos;  // TODO: Make this local to the execute function.
    Vector<BarrierMemory>      m_MemoryBarriers;
    Vector<BarrierSubpassDep>  m_SubpassBarriers;  // TODO: Make this renderpass local

   public:
    explicit RenderGraph() :
      m_Renderpasses{},
      m_Resources{},
      m_BytecodePos(nullptr)
    {
    }

    void clear()
    {
      deleteList(m_Renderpasses);
      deleteList(m_Resources);
      m_Bytecode.clear();
      m_BytecodePos = nullptr;
      m_MemoryBarriers.clear();
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
      // TempList Would be great here.
      Vector<GraphResourceBase*> targets{};

      const auto hasTarget = [&targets](const GraphResourceBase* target) -> bool {
        const auto end = targets.cend();
        const auto it  = std::find(targets.cbegin(), end, target);
        return it != end;
      };

      std::uint32_t index = 0;
      for (const auto& pass : m_Renderpasses)
      {
        targets.clear();

        const auto lastOf = [pass, index](const Vector<RenderpassBase*>& list) -> RenderpassBase* {
          RenderpassBase* ret = nullptr;

          for (auto rp : list)
          {
            if (rp->index >= index)
            {
              break;
            }

            ret = rp;
          }

          return ret == pass ? nullptr : ret;
        };

        // Reads
        for (const auto& res : pass->reads)
        {
          auto* const last_reader_pass = lastOf(res->readers);
          auto* const last_writer_pass = lastOf(res->writers);

          if (last_reader_pass)
          {
            if (last_writer_pass && last_writer_pass->index > last_reader_pass->index)
            {
              const auto& src_ref = last_writer_pass->findRef(res.as.resource, false);

              pass->barrier_ref = addBarrier(BarrierMemory(
               src_ref.pipeline_stage_flags,
               res.pipeline_stage_flags,
               src_ref.access_flags,
               res.access_flags));

              targets.push_back(res.as.resource);
              PRINT_OUT("WRITE -> READ");
            }
            else
            {
              auto& barrier_ref = last_reader_pass->barrier_ref;
              assert(barrier_ref.isValid());
              auto& barrier = m_MemoryBarriers[barrier_ref.index];

              if (hasTarget(res.as.resource))
              {
                const auto& src_ref = last_reader_pass->findRef(res.as.resource, true);

                barrier.src_stage |= src_ref.pipeline_stage_flags;
                barrier.dst_stage |= res.pipeline_stage_flags;
                barrier.src_access |= src_ref.access_flags;
                barrier.dst_access |= res.access_flags;

                pass->barrier_ref = barrier_ref;
              }
              else
              {
                targets.push_back(res.as.resource);
                PRINT_OUT("WRITE -> READ");
              }
            }
          }
          // TODO(SR): See if the else case needs to happen.
        }

        // Writes
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
            const bool  is_read = last_reader_writer_pass == last_reader_pass;
            const auto& src_ref = last_reader_writer_pass->findRef(res.as.resource, is_read);

            if (!pass->barrier_ref.isValid())
            {
              pass->barrier_ref = addBarrier(BarrierMemory(
               src_ref.pipeline_stage_flags,
               res.pipeline_stage_flags,
               src_ref.access_flags,
               res.access_flags));

              if (is_read)
              {
                PRINT_OUT("READ -> WRITE");
              }
              else
              {
                PRINT_OUT("WRITE -> WRITE");
              }
            }
            else
            {
              auto& barrier = m_MemoryBarriers[pass->barrier_ref.index];

              barrier.src_stage |= src_ref.pipeline_stage_flags;
              barrier.dst_stage |= res.pipeline_stage_flags;
              barrier.src_access |= src_ref.access_flags;
              barrier.dst_access |= res.access_flags;

              if (is_read)
              {
                PRINT_OUT("Upgrade Older Barrier. READ -> WRITE");
              }
              else
              {
                PRINT_OUT("Upgrade Older Barrier. WRITE -> WRITE");
              }
            }
          }
        }

        pass->compile(*this);

        bytecodeWriteAction(BytecodeInst::RENDERPASS, index);
        ++index;
      }
    }

    void execute()
    {
      m_BytecodePos       = m_Bytecode.data();
      const auto code_end = m_BytecodePos + m_Bytecode.size();

      while (m_BytecodePos != code_end)
      {
        const auto inst = bytecodeReadInst();

        switch (inst)
        {
          case BytecodeInst::RENDERPASS:
          {
            const auto idx = bytecodeReadUint32();

            PRINT_OUT("BytecodeInst::RENDERPASS " << idx);

            auto& renderpass = m_Renderpasses[idx];

            for (auto* pass : renderpass->subpasses)
            {
              pass->execute(*this, renderpass->data_ptr);
            }
            break;
          }
          case BytecodeInst::EXECUTION_BARRIER:
          {
            const auto idx = bytecodeReadUint32();

            PRINT_OUT("BytecodeInst::EXECUTION_BARRIER " << idx);

            break;
          }
          case BytecodeInst::MEMORY_BARRIER:
          {
            const auto idx = bytecodeReadUint32();

            PRINT_OUT("BytecodeInst::MEMORY_BARRIER " << idx);
            break;
          }
          default:
          {
            PRINT_OUT("Invalid action: " << (int)inst);
            break;
          }
        }
      }
    }

    GraphResourceBase* findResource(const char* name) const
    {
      const std::size_t name_len = std::strlen(name);

      for (auto* const res : m_Resources)
      {
        if (name_len == res->name.length && std::strcmp(name, res->name.str) == 0)
        {
          return res;
        }
      }

      throw "Bad";
    }

    BarrierRef addBarrier(const BarrierMemory& dep)
    {
      const BarrierRef ret = {BarrierType::MEMORY, uint32_t(m_MemoryBarriers.size())};
      bytecodeWriteAction(BytecodeInst::MEMORY_BARRIER, ret.index);
      m_MemoryBarriers.push_back(dep);
      return ret;
    }

    BarrierRef addSubpassBarrier(const BarrierSubpassDep& dep)
    {
      const BarrierRef ret = {BarrierType::SUBPASS_DEP, uint32_t(m_SubpassBarriers.size())};
      m_SubpassBarriers.push_back(dep);
      return ret;
    }

    ~RenderGraph()
    {
      clear();
    }

   private:
    template<typename TData, typename RSetupFn>
    void addPass(const char* name, RSetupFn&& setup_fn, bool is_compute)
    {
      auto* const rp = new Renderpass<TData>(*this, name, uint32_t(m_Renderpasses.size()), is_compute);
      m_Renderpasses.push_back(rp);
      setup_fn(*rp, rp->data);
    }

    void bytecodeWriteAction(BytecodeInst inst, std::uint32_t idx)
    {
      auto offset = m_Bytecode.size();

      m_Bytecode.resize(offset + sizeof(idx) + 1);

      m_Bytecode[offset++] = (std::uint8_t)inst;
      m_Bytecode[offset++] = (idx >> 24) & 0xFF;
      m_Bytecode[offset++] = (idx >> 16) & 0xFF;
      m_Bytecode[offset++] = (idx >> 8) & 0xFF;
      m_Bytecode[offset++] = (idx >> 0) & 0xFF;
    }

    BytecodeInst bytecodeReadInst()
    {
      const BytecodeInst ret = (BytecodeInst)m_BytecodePos[0];
      ++m_BytecodePos;
      return ret;
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
  static T* getResource(RenderGraph* graph, const char* name)
  {
    return static_cast<T*>(graph->findResource(name));
  }

  template<typename TRes, typename TDesc>
  static ResourceRef readResource(RenderpassBase* pass, TRes* res, const TDesc& desc)
  {
    const ResourceRef ref = ResourceRef::create(res, desc, true);
    res->readers.push_back(pass);
    pass->reads.push_back(ref);
    return ref;
  }

  template<typename TRes, typename TDesc>
  static ResourceRef writeResource(RenderpassBase* pass, TRes* res, const TDesc& desc)
  {
    const ResourceRef ref = ResourceRef::create(res, desc, false);
    res->writers.push_back(pass);
    pass->writes.push_back(ref);
    return ref;
  }

  const Vector<ResourceRef>& SubpassBase::reads() const
  {
    return refs[1];
  }

  void SubpassBase::refAttachment(std::int32_t attachment_index, PipelineStage stage, ImageUsage usage)
  {
    const ImageDesc desc{stage, usage};
    auto* const     res = parent.attachments[attachment_index];

    ResourceRef ref{};

    switch (usage)
    {
      case ImageUsage::WRITE_GENERAL:
      case ImageUsage::WRITE_COLOR:
      case ImageUsage::WRITE_DEPTH_WRITE_STENCIL:
      case ImageUsage::WRITE_DEPTH_READ_STENCIL:
      case ImageUsage::READ_DEPTH_WRITE_STENCIL:
      {
        ref = writeResource(&parent, res, desc);
        break;
      }
      case ImageUsage::READ_COLOR:
      case ImageUsage::READ_DEPTH_READ_STENCIL:
      case ImageUsage::READ_GENERAL:
      {
        ref = readResource(&parent, res, desc);
        break;
      }
    }

    switch (usage)
    {
      case ImageUsage::WRITE_COLOR:
      case ImageUsage::WRITE_DEPTH_WRITE_STENCIL:
      case ImageUsage::WRITE_DEPTH_READ_STENCIL:
      case ImageUsage::READ_DEPTH_WRITE_STENCIL:
      {
        refs[0].push_back(ref);
        break;
      }
      case ImageUsage::READ_COLOR:
      case ImageUsage::READ_DEPTH_READ_STENCIL:
      {
        refs[1].push_back(ref);
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

  ImageResource* RenderpassBase::addAttachment(const char* name, BifrostImageLayout final_layout, bfBool32 may_alias)
  {
    auto* const res = getResource<ImageResource>(&parent, name);
    this->attachments.push_back(res);
    return res;
  }

  void RenderpassBase::compile(RenderGraph& graph)
  {
    assert(!subpasses.empty() && "A render pass must have a atleast one subpass.");

    if (is_compute)
    {
      PRINT_OUT("Compile Compute Pass");
      PRINT_OUT("Compile SubPass" << 0);
    }
    else
    {
      PRINT_OUT("Compile Graphics Pass: '" << name.str << "'");

      const auto num_subpasses = subpasses.size();

      for (uint32_t index = 1; index < num_subpasses; ++index)
      {
        auto* const pass = subpasses[index];

        const auto addBarrier = [index, pass, &graph](const Vector<RenderpassBase*>& renderpass_list, const ResourceRef& dst_ref, bool is_read) {
          for (auto* const renderpass : renderpass_list)
          {
            if (renderpass->index < index)
            {
              const ResourceRef& src_ref = renderpass->findRef(dst_ref.as.resource, is_read);

              auto& barrier = pass->getBarrier(renderpass->index);

              if (!barrier.isValid())
              {
                barrier = graph.addSubpassBarrier(
                 BarrierSubpassDep(
                  src_ref.pipeline_stage_flags,
                  dst_ref.pipeline_stage_flags,
                  src_ref.access_flags,
                  dst_ref.access_flags,
                  renderpass->index,
                  index));

                PRINT_OUT("Subpass Dep: " << renderpass->index << " -> " << index);
              }
              else
              {
                auto& subpass_dep = graph.m_SubpassBarriers[barrier.index];

                subpass_dep.src_stage |= src_ref.pipeline_stage_flags;
                subpass_dep.dst_stage |= dst_ref.pipeline_stage_flags;
                subpass_dep.src_access |= src_ref.access_flags;
                subpass_dep.dst_access |= dst_ref.access_flags;

                PRINT_OUT("Upgrade Older Barrier.");
              }
            }
          }
        };

        PRINT_OUT("Compile SubPass" << index);

        for (const auto& read_res : pass->reads())
        {
          addBarrier(read_res->writers, read_res, false);
        }

        for (const auto& write_res : pass->writes())
        {
          addBarrier(write_res->writers, write_res, false);
          addBarrier(write_res->readers, write_res, true);
        }
      }
    }
  }
}  // namespace bifrost

#if BIFROST_GFX_RENDER_GRAPH_TEST

int main()
{
  using namespace std;
  using namespace bf;

  RenderGraph graph;

  struct GBufferData
  {
    ImageResource* gBufferImages[4];
    ImageResource* lightingComposite;
  };

  bfTextureHandle physical_resources[5] = {nullptr};

  // Additionally Needed Data(Can be handled by a better framebuffer / texture intertop abstraction):
  //   format           : Gotten by the texture.
  //   samples          : Gotten by the texture.
  //   initial_layout   : last layout it was in or VK_IMAGE_LAYOUT_UNDEFINED.
  //     optimization: Always VK_IMAGE_LAYOUT_UNDEFINED if we loadOp::Clear or loadOpDontCare

  graph.registerResource("g_PosRough", physical_resources[0]);
  graph.registerResource("g_NormalAO", physical_resources[1]);
  graph.registerResource("g_AlbedoMetallic", physical_resources[2]);
  graph.registerResource("g_Depth", physical_resources[3]);
  graph.registerResource("LightingComposite", physical_resources[4]);

  graph.addGraphicsPass<GBufferData>(
   "GBufferPass",
   [](Renderpass<GBufferData>& pass, GBufferData& data) {
     data.gBufferImages[0] = pass.addAttachment("g_PosRough", BIFROST_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false);
     data.gBufferImages[1] = pass.addAttachment("g_NormalAO", BIFROST_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false);
     data.gBufferImages[2] = pass.addAttachment("g_AlbedoMetallic", BIFROST_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false);
     data.gBufferImages[3] = pass.addAttachment("g_Depth", BIFROST_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, false);

     pass.addPass(
      [](SubpassBase& subpass, GBufferData& data) {
        subpass.refAttachment(0, PipelineStage::FRAGMENT, ImageUsage::WRITE_COLOR);
        subpass.refAttachment(1, PipelineStage::FRAGMENT, ImageUsage::WRITE_COLOR);
        subpass.refAttachment(2, PipelineStage::FRAGMENT, ImageUsage::WRITE_COLOR);
        subpass.refAttachment(3, PipelineStage::FRAGMENT, ImageUsage::WRITE_DEPTH_WRITE_STENCIL);
      },
      [](RenderGraph& graph, const GBufferData& data) {
        PRINT_OUT("GBuffer Drawing Happens Here.");
      });
   });

  graph.addGraphicsPass<GBufferData>(
   "Lighting Compositing Pass",
   [](Renderpass<GBufferData>& pass, GBufferData& data) {
     data.gBufferImages[0]  = pass.addAttachment("g_PosRough", BIFROST_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false);
     data.gBufferImages[1]  = pass.addAttachment("g_NormalAO", BIFROST_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false);
     data.gBufferImages[2]  = pass.addAttachment("g_AlbedoMetallic", BIFROST_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false);
     data.gBufferImages[3]  = pass.addAttachment("g_Depth", BIFROST_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, false);
     data.lightingComposite = pass.addAttachment("LightingComposite", BIFROST_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false);

     pass.addPass(
      [](SubpassBase& subpass, GBufferData& data) {
        subpass.refAttachment(0, PipelineStage::FRAGMENT, ImageUsage::READ_COLOR);
        subpass.refAttachment(1, PipelineStage::FRAGMENT, ImageUsage::READ_COLOR);
        subpass.refAttachment(2, PipelineStage::FRAGMENT, ImageUsage::READ_COLOR);
        subpass.refAttachment(3, PipelineStage::FRAGMENT, ImageUsage::WRITE_DEPTH_WRITE_STENCIL);
        subpass.refAttachment(4, PipelineStage::FRAGMENT, ImageUsage::WRITE_COLOR);
      },
      [](RenderGraph& graph, const GBufferData& data) {
        PRINT_OUT("Light Compositing Drawing Happens Here.");
      });
   });

  std::cout << "\n";
  graph.compile();
  std::cout << "\n\n";
  std::cout << "\n";
  graph.execute();
  std::cout << "\n\n";

  getchar();
  return 0;
}
#endif

#endif /* BIFROST_GFX_RENDER_GRAPH_HPP */
