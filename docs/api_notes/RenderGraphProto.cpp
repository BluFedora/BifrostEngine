#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

using namespace std;

struct RenderGraph;
struct RenderpassBase;
struct GraphResourceBase;

using VkAccess        = string;
using VkPipelineStage = string;

enum class BufferReadUsage
{
  COMPUTE_SSBO,
  // VK_ACCESS_SHADER_READ_BIT
  // VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
  COMPUTE_UNIFORM,
  // VK_ACCESS_UNIFORM_READ_BIT
  // VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
  GRAPHICS_VERTEX,
  // VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
  // VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
  GRAPHICS_INDEX,
  // VK_ACCESS_INDEX_READ_BIT
  // VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
  GRAPHICS_UNIFORM,
  // VK_ACCESS_UNIFORM_READ_BIT
  // VK_PIPELINE_STAGE_VERTEX_SHADER_BIT or VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
  GRAPHICS_DRAW_INDIRECT,
  // VK_ACCESS_INDIRECT_COMMAND_READ_BIT
  // VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT
};

enum class BufferWriteUsage
{
  COMPUTE_SSBO,
  // VK_ACCESS_SHADER_WRITE_BIT
  // VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
  COMPUTE_IMAGE,
  // VK_ACCESS_SHADER_WRITE_BIT
  // VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
  // VK_IMAGE_LAYOUT_GENERAL
};

enum class ImageReadUsage
{
  // GRAPHICS_FRAGMENT_*
  // VK_ACCESS_SHADER_READ_BIT
  // VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
  GRAPHICS_FRAGMENT_SAMPLE,
  // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  GRAPHICS_FRAGMENT_SAMPLE_DEPTH_STENCIL_RW,
  // VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL
  GRAPHICS_FRAGMENT_SAMPLE_DEPTH_STENCIL_WR,
  // VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
  GRAPHICS_FRAGMENT_SAMPLE_DEPTH_STENCIL_RR,
  // VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
  GRAPHICS_FRAGMENT_STORAGE_IMAGE,
  // VK_IMAGE_LAYOUT_GENERAL

  // Repeat for GRAPHICS_VERTEX
  // VK_PIPELINE_STAGE_VERTEX_INPUT_BIT

  // Compute needs a layout of VK_IMAGE_LAYOUT_GENERAL
};

enum class ImageWriteUsage
{
  // The only layout available:
  // VK_IMAGE_LAYOUT_GENERAL

  // You can do stuff in Vertex, and Fragment, and Compute Shaders.

  COMPUTE_WRITE,
  // VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
  // VK_ACCESS_SHADER_WRITE_BIT
  // VK_IMAGE_LAYOUT_GENERAL -> X

  GRAPHICS_FRAGMENT_WRITE,
  // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
  // VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
  // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL -> X

  GRAPHCICS_DEPTH_STENCIL,
  // VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
  // VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
  // VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
  GRAPHCICS_STENCIL,
  // VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL
  GRAPHCICS_DEPTH,
  // VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
};

struct MemBarrierAction
{
  string                     desc;
  vector<GraphResourceBase*> targets;

  MemBarrierAction(string str) :
    desc{str},
    targets{}
  {
  }

  bool hasTarget(const GraphResourceBase* t) const
  {
    return std::find(targets.begin(), targets.end(), t) != targets.end();
  }
};

using RenderpassAction = int;
using GraphAction      = variant<MemBarrierAction, RenderpassAction>;

template<class... Ts>
struct overloaded : Ts...
{
  using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...)->overloaded<Ts...>;

enum class RenderpassType
{
  COMPUTE  = 0,
  GRAPHICS = 1,
};

const char* toString(RenderpassType type)
{
  if (type == RenderpassType::COMPUTE)
  {
    return "COMPUTE";
  }

  return "GRAPHICS";
}

struct RenderpassBase
{
  const char*                name;
  size_t                     index;
  RenderpassType             type;
  vector<GraphResourceBase*> reads;
  vector<GraphResourceBase*> writes;
  size_t                     barrier;
  size_t                     queue_family;

  explicit RenderpassBase(const char* name, RenderpassType type) :
    name{name},
    index{32},
    type(type),
    reads{},
    writes{},
    barrier{size_t(-1)},
    queue_family{0}
  {
  }

  virtual void run(RenderGraph* graph) = 0;
};

struct GraphResourceBase
{
  const char*             name;
  vector<RenderpassBase*> readers;
  vector<RenderpassBase*> writers;

  explicit GraphResourceBase(const char* name) :
    name(name),
    readers{},
    writers{}
  {
  }
};

template<typename T>
struct GraphResource : public GraphResourceBase
{
  T data;

  explicit GraphResource(const char* name) :
    GraphResourceBase(name),
    data{}
  {
  }
};

using BufferResource = GraphResource<int>;

struct GraphBuilder
{
  RenderGraph*    graph;
  RenderpassBase* pass;

  BufferResource* readBuffer(const string& name);
  BufferResource* writeBuffer(const string& name);
};

template<typename TData, typename TExec>
struct RenderPass : public RenderpassBase
{
  TData data;
  TExec exec;

  explicit RenderPass(const char* name, RenderpassType type, TExec&& exec) :
    RenderpassBase(name, type),
    data{},
    exec{std::move(exec)}
  {
  }

  void run(RenderGraph* graph) override
  {
    exec(*graph, static_cast<const TData&>(data));
  }
};

struct RenderGraph
{
  unordered_map<string, std::unique_ptr<GraphResourceBase>> resources;
  vector<std::unique_ptr<RenderpassBase>>                   passes;
  vector<GraphAction>                                       actions;

  template<typename T>
  GraphResource<T>& addResource(const char* name)
  {
    auto* const res = new GraphResource<T>(name);
    resources[name] = std::unique_ptr<GraphResourceBase>(res);
    return *res;
  }

  template<typename T, typename SetupFn, typename ExecFn>
  T& addPass(const char* name, RenderpassType type, SetupFn&& setup, ExecFn&& exec)
  {
    auto* const rp = new RenderPass<T, ExecFn>(name, type, std::forward<ExecFn>(exec));
    rp->index      = passes.size();
    passes.emplace_back(rp);
    GraphBuilder builder{this, rp};
    setup(builder, rp->data);
    return rp->data;
  }

  void compile()
  {
    const auto num_passes = passes.size();

    if (num_passes)
    {
      actions.emplace_back(RenderpassAction{0});

      for (size_t i = 1; i < num_passes; ++i)
      {
        const auto& pass      = passes[i];

        bool             needs_barrier = false;
        MemBarrierAction barrier_info  = {""};

        const auto pushBarrier = [this, &barrier_info, &pass](const string& desc, const vector<GraphResourceBase*>& targets) {
          barrier_info.desc += "[" + desc + "]";
          barrier_info.targets.insert(barrier_info.targets.end(), targets.begin(), targets.end());
          pass->barrier = actions.size();
          actions.emplace_back(barrier_info);
        };

        const auto lastOf = [i](vector<RenderpassBase*>& list, RenderpassBase*& ret) {
          for (auto reader_pass : list)
          {
            if (reader_pass->index >= i)
            {
              break;
            }

            ret = reader_pass;
          }
        };

        // Reads
        {
          string                     read_barrier;
          vector<GraphResourceBase*> targets = {};

          for (auto res : pass->reads)
          {
            RenderpassBase* last_reader_pass = nullptr;
            RenderpassBase* last_writer_pass = nullptr;

            lastOf(res->readers, last_reader_pass);
            lastOf(res->writers, last_writer_pass);

            if (last_reader_pass)
            {
              if (last_writer_pass && last_writer_pass->index > last_reader_pass->index)
              {
                read_barrier = "WRITE -> READ_" + string(res->name);
                targets.push_back(res);
              }
              else
              {
                auto& barrier = std::get<MemBarrierAction>(actions[last_reader_pass->barrier]);

                if (barrier.hasTarget(res))
                {
                  barrier.desc += string("(") + pass->name + "-" + res->name + ")";
                  pass->barrier = last_reader_pass->barrier;
                }
                else
                {
                  read_barrier = "WRITE -> READ_" + string(res->name);
                  targets.push_back(res);
                }
              }
            }
            else
            {
              read_barrier = "WRITE -> READ_" + string(res->name);
              targets.push_back(res);
            }
          }

          if (!read_barrier.empty())
          {
            pushBarrier(read_barrier, targets);
          }
        }

        // Writes
        {
          string                     read_barrier;
          vector<GraphResourceBase*> targets = {};

          for (auto write : pass->writes)
          {
            RenderpassBase* last_reader_pass = nullptr;
            RenderpassBase* last_writer_pass = nullptr;
            lastOf(write->readers, last_reader_pass);
            lastOf(write->writers, last_writer_pass);

            RenderpassBase* const last_reader_writer_pass = [last_reader_pass, last_writer_pass]() {
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
                read_barrier = "READ -> WRITE_" + string(write->name);
              }
              else
              {
                read_barrier = "WRITE -> WRITE_" + string(write->name);
              }
              targets.push_back(write);
            }
          }

          if (!read_barrier.empty())
          {
            pushBarrier(read_barrier, targets);
          }
        }

        actions.emplace_back(RenderpassAction(i));
      }
    }
  }

  void execute()
  {
    for (const auto& action : actions)
    {
      visit(
       overloaded{
        [](auto arg) {
          cout << "Invalid Action\n";
        },
        [](const MemBarrierAction& mem_barrier) {
          cout << "MEMORY_BARRIER: " << mem_barrier.desc << "\n";
        },
        [](const RenderpassAction& render_pass) {
          cout << "RENDER_PASS: " << render_pass << "\n";
        }},
       action);
    }
  }
};

BufferResource* GraphBuilder::readBuffer(const string& name)
{
  auto* const res = graph->resources[name].get();

  res->readers.emplace_back(this->pass);
  this->pass->reads.emplace_back(res);

  return (BufferResource*)res;
}

BufferResource* GraphBuilder::writeBuffer(const string& name)
{
  auto* const res = graph->resources[name].get();

  res->writers.emplace_back(this->pass);
  this->pass->writes.emplace_back(res);

  return (BufferResource*)res;
}

int main()
{
  cout << "Render Pass Prototype BGN\n\n";

  RenderGraph graph;

  struct GPass
  {
    BufferResource* i = nullptr;
  };

  graph.addResource<int>("Buffer0");
  graph.addResource<int>("Buffer1");

  const auto makeWrite = [&graph](const char* name) {
    graph.addPass<GPass>(name, RenderpassType::COMPUTE, [](GraphBuilder& builder, GPass& data) {
        data.i = builder.writeBuffer("Buffer0");
        data.i->data = 3; }, [](RenderGraph& graph, const GPass& data) {});
  };

  const auto makeRead = [&graph](const char* name) {
    graph.addPass<GPass>(name, RenderpassType::COMPUTE, [](GraphBuilder& builder, GPass& data) { data.i = builder.readBuffer("Buffer0"); }, [](RenderGraph& graph, const GPass& data) {

    });
  };

  graph.addPass<GPass>("RP0", RenderpassType::COMPUTE, [](GraphBuilder& builder, GPass& data) {
        data.i = builder.writeBuffer("Buffer0");
        data.i->data = 3; }, [](RenderGraph& graph, const GPass& data) {

  });

  makeRead("RP1");
  makeWrite("RP2");

  graph.addPass<GPass>("RP3", RenderpassType::COMPUTE, [](GraphBuilder& builder, GPass& data) {
        data.i = builder.readBuffer("Buffer0");
        data.i = builder.writeBuffer("Buffer1"); }, [](RenderGraph& graph, const GPass& data) {});

  makeRead("RP4");

  graph.addPass<GPass>("RP5", RenderpassType::COMPUTE, [](GraphBuilder& builder, GPass& data) {
        data.i = builder.readBuffer("Buffer0");
        data.i = builder.readBuffer("Buffer1"); }, [](RenderGraph& graph, const GPass& data) {});

  graph.compile();
  graph.execute();

  cout << "\nRender Pass Prototype END\n";
  return 0;
}
