//
// File:        bta_main.cpp
// Author:      Shareef Abdoul-Raheem
// Description: The main driver for the build tool analyzer tool.
//
// Copyright Nintendo of America (c) 2020
//

#include "bf/Platform.h"               /* Platform Windowing API */
#include <bf/bf_gfx_api.h>             /* Graphics API           */
#include <bf/bifrost_imgui_glfw.hpp>   /* bifrost::imgui::*      */
#include <bf/utility/bifrost_json.hpp> /* JSON OOP API           */

#include <imgui/imgui.h>   /* ImGui::*  */
#include <implot/implot.h> /* ImPlot::* */

#include <imgui/imgui_internal.h> /* DockBuilderGetNode (must be included after 'imgui/imgui.h') */

#include "bta_dialog.hpp" /* Dialog API */

#include <filesystem> /* std::filesystem::* */
#include <fstream>    /* ofstream           */
#include <memory>     /* unique_ptr         */
#include <string>     /* string             */
#include <vector>     /* vector             */

struct SourceTimeEvent;

static bool sortBasedOnEvtDurationTime(const SourceTimeEvent* a, const SourceTimeEvent* b);

using SourceTimeEventPtr = std::unique_ptr<SourceTimeEvent>;

struct SourceTimeEvent final
{
  std::string                   name;
  int                           time_bgn;
  int                           time_end;
  std::vector<SourceTimeEvent*> children;

  SourceTimeEvent(std::string&& name, int start, int duration) :
    name{std::move(name)},
    time_bgn{start},
    time_end{start + duration},
    children{}
  {
    std::transform(this->name.begin(), this->name.end(), this->name.begin(), [](char c) -> char {
      return c == '\\' ? '/' : c;
    });
  }

  int duration() const { return time_end - time_bgn; }

  bool contains(const SourceTimeEvent& rhs) const
  {
    return rhs.time_bgn >= time_bgn && rhs.time_end <= time_end;
  }

  void sortChildren()
  {
    std::sort(children.begin(), children.end(), &sortBasedOnEvtDurationTime);

    for (SourceTimeEvent* const child : children)
    {
      child->sortChildren();
    }
  }
};

static bool sortBasedOnEvtStartAndDurationTime(const SourceTimeEventPtr& a, const SourceTimeEventPtr& b)
{
  using StartDurationPair = std::pair<int, int>;

  return std::less<>{}(StartDurationPair{a->time_bgn, -a->duration()}, StartDurationPair{b->time_bgn, -b->duration()});
}

static bool sortBasedOnEvtDurationTime(const SourceTimeEvent* a, const SourceTimeEvent* b)
{
  return std::greater<>{}(a->duration(), b->duration());
}

template<typename T>
static bool sortBasedOnTotalTime(const T& a, const T& b)
{
  return std::less<>{}(a.total_duration, b.total_duration);
}

struct SourceFile final
{
  //
  // This was much easier to implemented with reference sematics hence the pointers.
  //

  std::string                     name;
  std::string                     path;
  std::vector<SourceTimeEventPtr> events;
  std::vector<SourceTimeEvent*>   hierarchical_events;
  int                             total_duration;

  void postProcessEvents()
  {
    if (!events.empty())
    {
      hierarchical_events.clear();

      std::sort(events.begin(), events.end(), &sortBasedOnEvtStartAndDurationTime);

      hierarchical_events.push_back(events[0].get());

      const std::size_t             events_length = events.size();
      std::vector<SourceTimeEvent*> parent_stack  = {hierarchical_events[0]};
      std::size_t                   next_index    = 1;

      while (next_index < events_length)
      {
        SourceTimeEvent& next_evt = *events[next_index];

        while (!parent_stack.empty())
        {
          SourceTimeEvent& potential_parent = *parent_stack.back();

          if (potential_parent.contains(next_evt))
          {
            potential_parent.children.push_back(&next_evt);
            break;
          }

          parent_stack.pop_back();
        }

        if (parent_stack.empty())
        {
          hierarchical_events.push_back(&next_evt);
        }

        parent_stack.push_back(&next_evt);

        ++next_index;
      }

      std::sort(hierarchical_events.begin(), hierarchical_events.end(), &sortBasedOnEvtDurationTime);

      for (SourceTimeEvent* evt : hierarchical_events)
      {
        evt->sortChildren();
      }
    }
  }
};

struct Project;

using ProjectPtr = std::unique_ptr<Project>;

struct Project final
{
  std::string             name;
  std::vector<SourceFile> sources;
  int                     total_duration;

  Project(std::string&& name, std::vector<SourceFile>&& sources) :
    name{std::move(name)},
    sources{std::move(sources)},
    total_duration{0}
  {
  }

  void postProcessEvents()
  {
    total_duration = 0;

    for (SourceFile& source : sources)
    {
      source.postProcessEvents();
      total_duration += source.total_duration;
    }

    std::sort(sources.begin(), sources.end(), [](const SourceFile& a, const SourceFile& b) {
      return a.total_duration > b.total_duration;
    });
  }
};

// Holds the data for anything needed by this tool
struct MainApplication final
{
  bfGfxContextHandle                 gfx_ctx;
  bfWindowSurfaceHandle              main_window_surface;
  std::vector<ProjectPtr>            open_projects;
  Project*                           selected_project      = nullptr;
  SourceFile*                        selected_source       = nullptr;
  std::vector<std::filesystem::path> project_to_load       = {};
  int                                total_project_to_load = -1;
  char                               search_buffer[64]     = "";
  std::size_t                        search_buffer_length  = 0;

  void drawUI();

 private:
  void loadFolder(const std::string& folder_path_str);
  void addProject(const std::filesystem::path& folder);

  // Commands
  void cmdOpenFolder();
  void cmdOpenProject();
  void cmdArchiveBuild();

  // GUI
  void drawMenubar();
  void drawSourceTimeEvent(const SourceTimeEvent& event) const;
};

// Callbacks for the main window.
static void onEventReceived(bfWindow* window, bfEvent* event);
static void onRenderFrame(bfWindow* window);

enum /* ErrorCodes */
{
  ERROR_NONE            = 0,
  ERROR_PLATFORM_INIT   = 1,
  ERROR_PLATFORM_WINDOW = 2,
  ERROR_DIALOG_INIT     = 3,
};

extern "C" int main(int argc, char* argv[])
{
  if (!bta::dialogInit())
  {
    return ERROR_DIALOG_INIT;
  }

  // Platform Initialization
  {
    bfPlatformInitParams platform_params;
    platform_params.argc      = argc;
    platform_params.argv      = argv;
    platform_params.allocator = &bfPlatformDefaultAllocator;
    platform_params.user_data = nullptr;

    if (!bfPlatformInit(platform_params))
    {
      return ERROR_PLATFORM_INIT;
    }
  }

  // Window Initialization

  const uint32_t  window_flags = k_bfWindowFlagsDefault & ~k_bfWindowFlagIsMaximizedOnShow;
  bfWindow* const main_window  = bfPlatformCreateWindow("Build Time Analyzer | v2020.0.5", 1280, 720, window_flags);

  if (!main_window)
  {
    bfPlatformQuit();
    bta::dialogQuit();
    return ERROR_PLATFORM_WINDOW;
  }

  main_window->event_fn = &onEventReceived;
  main_window->frame_fn = &onRenderFrame;

  // Graphics Initialization

  bfGfxContextCreateParams graphic_params;
  graphic_params.app_name    = "Build Time Analyzer";
  graphic_params.app_version = bfGfxMakeVersion(1, 0, 0);

  const bfGfxContextHandle gfx_ctx = bfGfxContext_new(&graphic_params);

  // Graphics: Window Surface

  const bfWindowSurfaceHandle main_surface = bfGfxCreateWindow(gfx_ctx, main_window);

  main_window->renderer_data = main_surface;

  // Main Application

  MainApplication app{gfx_ctx, main_surface, {}};

  main_window->user_data = &app;

  bf::imgui::startup(gfx_ctx, main_window);

  // Main Loop

  bfPlatformDoMainLoop(main_window);

  // Clean Up
  bf::imgui::shutdown();
  bfGfxDestroyWindow(gfx_ctx, main_surface);
  bfGfxContext_delete(gfx_ctx);
  bfPlatformDestroyWindow(main_window);
  bfPlatformQuit();
  bta::dialogQuit();

  return ERROR_NONE;
}

static void onEventReceived(bfWindow* window, bfEvent* event)
{
  bf::imgui::onEvent(window, *event);
}

static void onRenderFrame(bfWindow* window)
{
  MainApplication* const app        = static_cast<MainApplication*>(window->user_data);
  const float            delta_time = 1.0f / 60.0f;

  if (bfGfxBeginFrame(app->gfx_ctx, app->main_window_surface))
  {
    const bfGfxCommandListHandle main_command_list = bfGfxRequestCommandList(app->gfx_ctx, app->main_window_surface, 0);

    if (main_command_list && bfGfxCmdList_begin(main_command_list))
    {
      const bfTextureHandle main_surface_tex = bfGfxDevice_requestSurface(app->main_window_surface);
      int                   window_width;
      int                   window_height;

      bfWindow_getSize(window, &window_width, &window_height);

      bf::imgui::beginFrame(
       main_surface_tex,
       float(window_width),
       float(window_height),
       delta_time);

      app->drawUI();

      bf::imgui::setupDefaultRenderPass(main_command_list, main_surface_tex);
      bf::imgui::endFrame();
      bfGfxCmdList_end(main_command_list);
      bfGfxCmdList_submit(main_command_list);
    }

    bfGfxEndFrame(app->gfx_ctx);
  }
}

//   Algorithm based on the one described in "Game Programming Gems 6"
static float stringMatchPercent(const char* str1, std::size_t str1_len, const char* str2, std::size_t str2_len);

void MainApplication::drawUI()
{
  drawMenubar();

  // ImPlot::ShowDemoWindow();

  // Dock Space
  {
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
    ImGuiViewport*   viewport     = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->GetWorkPos());
    ImGui::SetNextWindowSize(viewport->GetWorkSize());
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
      window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::Begin("Main DockSpace", nullptr, window_flags);

    const ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");

    // Initial layout
    if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
    {
      ImGui::DockBuilderRemoveNode(dockspace_id);                             // Clear out existing layout
      ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);  // Add empty node
      ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

      ImGuiID       dock_right  = dockspace_id;
      const ImGuiID dock_left   = ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Left, 0.3f, nullptr, &dock_right);
      const ImGuiID dock_middle = ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Left, 0.5f, nullptr, &dock_right);

      ImGui::DockBuilderDockWindow("Project View", dock_left);
      ImGui::DockBuilderDockWindow("Source View", dock_middle);
      ImGui::DockBuilderDockWindow("Event View", dock_right);

      ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    ImGui::End();

    ImGui::PopStyleVar(3);

    //SceneView& scene_window = getWindow<SceneView>();

    //ImGui::DockBuilderDockWindow(scene_window.fullImGuiTitle(engine.tempMemory()), dockspace_id);
  }

  // Window Panes

  if (ImGui::Begin("Project View"))
  {
    if (open_projects.empty() || !project_to_load.empty())
    {
      if (project_to_load.empty())
      {
        // float values[] = {0.0f, 1.0f, 2.0, 3.0f, 0.0f, 2.0f, 1.4f, 4.0f, 8.0f};
        //
        // ImGui::PlotHistogram(
        //  "Test Histogram",
        //  values,
        //  int(bfCArraySize(values)),
        //  0,
        //  "Overlay TXT");
        //
        // ImGui::PlotLines(
        //  "Test Lines",
        //  values,
        //  int(bfCArraySize(values)),
        //  0,
        //  "Overlay TXT");

        const float text_width = ImGui::CalcTextSize("(No Projects Loaded)").x;

        ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.5f - text_width * 0.5f);
        ImGui::Text("(No Projects Loaded)");

        ImGui::Separator();

        const float  window_region_width = ImGui::GetWindowContentRegionWidth();
        const ImVec2 btn_full_width      = ImVec2(window_region_width, 0.0f);

        if (ImGui::Button("Open Folder", btn_full_width))
        {
          cmdOpenFolder();
        }

        if (ImGui::IsItemHovered())
        {
          ImGui::SetTooltip("This will open a folder treating each sub directory as a project.");
        }

        if (ImGui::Button("Open Project", btn_full_width))
        {
          cmdOpenProject();
        }

        if (ImGui::IsItemHovered())
        {
          ImGui::SetTooltip("This will open a single folder as a project.");
        }

        char* rynda_root = std::getenv("RYNDA_ROOT");

        if (rynda_root)
        {
          ImGui::Separator();

          ImGui::Text("Autodetected folders:");

          const auto addPathButton = [this](const std::filesystem::path& p) {
            if (std::filesystem::exists(p))
            {
              const std::string path_as_str = p.u8string();

              if (ImGui::Selectable(path_as_str.c_str()))
              {
                loadFolder(path_as_str);
              }
            }
          };

          std::filesystem::path rynda_root_path = rynda_root;

          if (std::filesystem::exists(rynda_root_path))
          {
            rynda_root_path /= std::filesystem::path(".build-staged") / "aarch64-nintendo-nx-elf" / "llvm.stage1" / "tools";

            addPathButton(rynda_root_path);

            rynda_root_path /= "clang";
            rynda_root_path /= "tools";

            addPathButton(rynda_root_path);
          }

          ImGui::Text("Rynda Root: %s", rynda_root);
        }
      }
      else
      {
        addProject(project_to_load.back());
        project_to_load.pop_back();

        //
        // The label is calculated this way cuz otherwise it would be one project
        // behind since the screen freezes when loading so the display show the last result.
        //

        const std::string label = project_to_load.empty() ? "Almost Done..." : "Loading \"" + project_to_load.back().filename().string() + "\"...";

        ImGui::ProgressBar(1.0f - float(project_to_load.size()) / float(total_project_to_load), ImVec2(-1.0f, 0.0f), label.c_str());
      }
    }
    else
    {
      if (!search_buffer_length)
      {
        ImGui::SetNextItemWidth(-1.0f);
      }

      if (ImGui::InputTextWithHint("###Search", "Search...", search_buffer, sizeof(search_buffer), ImGuiInputTextFlags_AutoSelectAll))
      {
        search_buffer_length = std::strlen(search_buffer);
      }

      if (search_buffer_length)
      {
        ImGui::SameLine();

        if (ImGui::Button("Clear"))
        {
          search_buffer_length = 0;
          search_buffer[0]     = '\0';
        }
      }

      const ImGuiTableFlags project_list_flags = ImGuiTableFlags_Sortable | ImGuiTableFlags_BordersV | ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersHOuter | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg;

      if (ImGui::BeginTable("Project List", 2, project_list_flags))
      {
        enum
        {
          ProjectListColumn_Name,
          ProjectListColumn_Time,
        };

        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide, -1.0f, ProjectListColumn_Name);
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortAscending, -1.0f, ProjectListColumn_Time);

        if (const ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs())
        {
          if (sorts_specs->SpecsChanged)
          {
            std::sort(
             open_projects.begin(),
             open_projects.end(),
             [&sorts_specs](const ProjectPtr& a, const ProjectPtr& b) {
               for (int n = 0; n < sorts_specs->SpecsCount; ++n)
               {
                 const ImGuiTableSortSpecsColumn* sort_spec = &sorts_specs->Specs[n];
                 int                              delta     = 0;

                 switch (sort_spec->ColumnUserID)
                 {
                   case ProjectListColumn_Name:
                     delta = a->name.compare(b->name);
                     break;
                   case ProjectListColumn_Time:
                     delta = a->total_duration < b->total_duration ? -1 : 1;
                     break;
                   default:
                     IM_ASSERT(0);
                     break;
                 }

                 if (delta > 0)
                 {
                   return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? true : false;
                 }

                 if (delta < 0)
                 {
                   return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? false : true;
                 }
               }

               return a->name.compare(b->name) <= 0;
             });
          }
        }

        ImGui::TableAutoHeaders();

        for (ProjectPtr& project : open_projects)
        {
          const char* const project_name_cstr = project->name.c_str();
          const std::size_t project_name_len  = project->name.length();
          const bool        show_project      = !search_buffer_length || stringMatchPercent(search_buffer, search_buffer_length, project_name_cstr, project_name_len) > 0.34f;

          if (show_project)
          {
            ImGui::TableNextRow();

            if (ImGui::Selectable(project_name_cstr, project.get() == selected_project, ImGuiSelectableFlags_SpanAllColumns))
            {
              selected_project = project.get();
            }

            ImGui::TableNextCell();

            ImGui::Text("%i us", project->total_duration);
          }
        }

        ImGui::EndTable();
      }
    }
  }
  ImGui::End();

  if (ImGui::Begin("Source View"))
  {
    if (selected_project)
    {
#if 0
      const auto&              sources = selected_project->sources;
      std::vector<const char*> labels(sources.size());
      std::vector<float>       timing_data(sources.size());

      for (std::size_t i = 0; i < sources.size(); ++i)
      {
        labels[i]      = sources[i].name.c_str();
        timing_data[i] = float(sources[i].total_duration);
      }

      if (ImPlot::BeginPlot("##Source View", NULL, NULL, ImVec2(250, 250), ImPlotFlags_Highlight, 0, 0))
      {
        ImPlot::PlotPieChart(labels.data(), timing_data.data(), int(labels.size()), 1.0f, 1.0f, 0.4f, false, "%.2f");
        ImPlot::EndPlot();
      }
#endif

      const ImGuiTableFlags project_list_flags = ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersHOuter | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg;

      if (ImGui::BeginTable("Source List", 2, project_list_flags))
      {
        enum
        {
          ProjectListColumn_Name,
          ProjectListColumn_Time,
        };

        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide, -1.0f, ProjectListColumn_Name);
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, -1.0f, ProjectListColumn_Time);

        ImGui::TableAutoHeaders();

        for (SourceFile& source : selected_project->sources)
        {
          ImGui::TableNextRow();

          if (ImGui::Selectable(source.name.c_str(), &source == selected_source, ImGuiSelectableFlags_SpanAllColumns))
          {
            selected_source = &source;
          }

          ImGui::TableNextCell();

          ImGui::Text("%i us", source.total_duration);
        }

        ImGui::EndTable();
      }
    }
    else
    {
      ImGui::Text("(No Project Selected)");
    }
  }
  ImGui::End();

  if (ImGui::Begin("Event View"))
  {
    if (selected_source)
    {
      for (const SourceTimeEvent* evt : selected_source->hierarchical_events)
      {
        drawSourceTimeEvent(*evt);
      }
    }
    else
    {
      ImGui::Text("(No Source File Selected)");
    }
  }
  ImGui::End();
}

namespace bta_fs
{
  namespace detail
  {
    template<typename F>
    void recurseDirectoryImpl(const std::filesystem::path& directory, F&& callback, int level)
    {
      namespace std_fs = std::filesystem;

      for (const auto& entry : std_fs::directory_iterator(directory))
      {
        if (entry.is_directory())
        {
          recurseDirectoryImpl(entry, std::forward<F>(callback), level + 1);
        }
        else if (entry.is_regular_file())
        {
          callback(entry, level);
        }
        else
        {
        }
      }
    }
  }  // namespace detail

  // expected: F = void (*)(const std::filesystem::path& file, int level)
  template<typename F>
  void recurseDirectory(const std::filesystem::path& directory, F&& callback)
  {
    detail::recurseDirectoryImpl(directory, std::forward<F>(callback), 0);
  }
}  // namespace bta_fs

void MainApplication::loadFolder(const std::string& folder_path_str)
{
  namespace std_fs = std::filesystem;

  const std_fs::path folder_path = folder_path_str;

  for (const auto& entry : std_fs::directory_iterator(folder_path))
  {
    if (entry.is_directory())
    {
      project_to_load.push_back(entry);
    }
  }

  total_project_to_load = int(project_to_load.size());
}

void MainApplication::addProject(const std::filesystem::path& folder)
{
  std::vector<SourceFile> sources = {};

  bta_fs::recurseDirectory(folder, [&sources](const std::filesystem::path& file, int level) {
    // We only care about json files.
    if (file.extension() != ".json")
    {
      return;
    }

    //
    // TODO(SR):
    //   'LoadFileIntoMemory' is technically not part of the Public API
    //   (declared in "bifrost/graphics/bifrost_gfx_api.h")
    //   but is very convenient to use (hence why it's there in the first place ;)).
    //

    const std::string file_as_string = file.string();
    long              file_buffer_size;
    char*             file_buffer = LoadFileIntoMemory(file_as_string.c_str(), &file_buffer_size);

    if (file_buffer)
    {
      using namespace bf::json;

      const Value json_data = parse(file_buffer, file_buffer_size);

      if (json_data.isObject())
      {
        int                             total_source_time = 0;
        std::vector<SourceTimeEventPtr> events            = {};
        const Value* const              trace_events_data = json_data.at("traceEvents");

        if (trace_events_data && trace_events_data->isArray())
        {
          const Array& trace_events_array = trace_events_data->as<Array>();

          for (const Value& trace_evt : trace_events_array)
          {
            if (trace_evt.isObject())
            {
              const Value* const tid_data = trace_evt.at("tid");

              if (tid_data && tid_data->isNumber())
              {
                const Value* const evt_args_data = trace_evt.at("args");
                const int          tid_num       = int(tid_data->as<Number>());

                if (evt_args_data && evt_args_data->isObject())
                {
                  if (tid_num == 0)
                  {
                    const Value* const evt_name_data   = trace_evt.at("name");
                    const Value* const arg_detail_data = evt_args_data->at("detail");

                    if (evt_name_data && evt_name_data->isString() && arg_detail_data && arg_detail_data->isString())
                    {
                      const String& evt_name_str = evt_name_data->as<String>();

                      // We only care about Source events for now, maybe can look at other data in the future?

                      if (evt_name_str == "Source")
                      {
                        const String& arg_name_str   = arg_detail_data->as<String>();
                        const int     event_start    = int(trace_evt.get<Number>("ts", 0.0));
                        const int     event_duration = int(trace_evt.get<Number>("dur", 0.0));

                        events.emplace_back(std::make_unique<SourceTimeEvent>(arg_name_str.c_str(), event_start, event_duration));
                      }
                    }
                  }
                  else if (tid_num == 1)
                  {
                    const Value* const arg_avg_ms_data = evt_args_data->at("avg ms");

                    if (arg_avg_ms_data && arg_avg_ms_data->isNumber())
                    {
                      total_source_time += int(arg_avg_ms_data->as<Number>());
                    }
                  }
                  else
                  {
                    // The other data wasn't all that useful for the task at hand.
                  }
                }
              }
            }
          }
        }

        sources.emplace_back(SourceFile{file.filename().string(), file.string(), std::move(events), {}, total_source_time});
      }

      std::free(file_buffer);
    }
  });

  if (!sources.empty())
  {
    std::string name    = folder.filename().string();
    ProjectPtr& project = open_projects.emplace_back(std::make_unique<Project>(std::move(name), std::move(sources)));

    project->postProcessEvents();
  }
}

void MainApplication::cmdOpenFolder()
{
  std::string file_path;  // TODO(Shareef): This should be a member as to not create it each frame.

  if (bta::dialogOpenFolder(file_path))
  {
    loadFolder(file_path);
  }
}

void MainApplication::cmdOpenProject()
{
  std::string file_path;  // TODO(Shareef): This should be a member as to not create it each frame.

  if (bta::dialogOpenFolder(file_path))
  {
    project_to_load.emplace_back(file_path);
    ++total_project_to_load;
  }
}

void MainApplication::cmdArchiveBuild()
{
  std::string file_path;  // TODO(Shareef): This should be a member as to not create it each frame.

  if (bta::dialogOpenFolder(file_path))
  {
    const std::filesystem::path root_path = file_path;

    for (const ProjectPtr& project : open_projects)
    {
      const auto project_dir = root_path / project->name;

      std::error_code err;

      create_directory(project_dir, err);

      if (!err)
      {
        for (const SourceFile& source : project->sources)
        {
          const auto source_path = project_dir / source.name;

          long  file_buffer_size;
          char* file_buffer = LoadFileIntoMemory(source.path.c_str(), &file_buffer_size);

          if (file_buffer)
          {
            std::ofstream file_out{source_path};

            if (file_out)
            {
              file_out << std::string_view{file_buffer, std::size_t(file_buffer_size)};
              file_out.close();
            }

            std::free(file_buffer);
          }
        }
      }
    }
  }
}

void MainApplication::drawMenubar()
{
  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("Commands"))
    {
      if (ImGui::MenuItem("Open Folder"))
      {
        cmdOpenFolder();
      }

      if (ImGui::MenuItem("Open Project"))
      {
        cmdOpenProject();
      }

      if (ImGui::MenuItem("Archive Build (*warning takes a VERY long time*)"))
      {
        cmdArchiveBuild();
      }

      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
}

void MainApplication::drawSourceTimeEvent(const SourceTimeEvent& event) const
{
  const std::size_t find_last_of = event.name.find_last_of('/');

  std::size_t best_find = 0;

  if (find_last_of != std::string::npos)
  {
    best_find = find_last_of;

    const std::size_t find_last_2nd_of = event.name.find_last_of('/', find_last_of - 1);

    if (find_last_2nd_of != std::string::npos)
    {
      best_find = find_last_2nd_of;

      const std::size_t find_last_3rd_of = event.name.find_last_of('/', find_last_2nd_of - 1);

      if (find_last_3rd_of != std::string::npos)
      {
        best_find = find_last_3rd_of;
      }
    }
  }

  const bool is_open = ImGui::TreeNode(event.name.c_str() + best_find + 1);

  if (ImGui::BeginPopupContextItem(NULL, ImGuiPopupFlags_MouseButtonRight))
  {
    if (ImGui::Selectable("Copy Path"))
    {
      ImGui::SetClipboardText(event.name.c_str());
    }

    ImGui::EndPopup();
  }

  if (is_open)
  {
    ImGui::Text("Start %i", event.time_bgn);
    ImGui::Text("End %i", event.time_end);
    ImGui::Text("Duration %i", event.duration());

    for (const SourceTimeEvent* child : event.children)
    {
      drawSourceTimeEvent(*child);
    }

    ImGui::TreePop();
  }
}

static float stringMatchPercent(const char* str1, std::size_t str1_len, const char* str2, std::size_t str2_len)
{
  static constexpr float k_CapitalLetterMismatchCost = 0.93f;  // The penalty should not be super high for capitalization mismatch.

  const std::size_t total_size   = std::max(str1_len, str2_len);
  const char* const str1_end     = str1 + str1_len;
  const char* const str2_end     = str2 + str2_len;
  const float       cost_match   = 1.0f / total_size;
  const float       cost_capital = k_CapitalLetterMismatchCost / total_size;
  float             match_value  = 0.0f;

  while (str1 != str1_end && str2 != str2_end)
  {
    if (*str1 == *str2)
    {
      match_value += cost_match;
    }
    else if (std::tolower(*str1) == std::tolower(*str2))
    {
      match_value += cost_capital;
    }
    else
    {
      const char* left_best   = str1_end;
      const char* right_best  = str2_end;
      int         best_count  = std::numeric_limits<decltype(best_count)>::max();
      int         left_count  = 0;
      int         right_count = 0;

      for (const char* l = str1; (l != str1_end) && (left_count + right_count < best_count); ++l)
      {
        for (const char* r = str2; (r != str2_end) && (left_count + right_count < best_count); ++r)
        {
          if (std::tolower(*l) == std::tolower(*r))
          {
            const auto total_count = left_count + right_count;

            if (total_count < best_count)
            {
              best_count = total_count;
              left_best  = l;
              right_best = r;
            }
          }

          ++right_count;
        }

        ++left_count;
        right_count = 0;
      }

      str1 = left_best;
      str2 = right_best;
      continue;
    }

    // TODO(Shareef): Test to see if the if statement is really needed.
    if (str1 != str1_end) ++str1;
    if (str2 != str2_end) ++str2;
  }

  // NOTE(Shareef): Some floating point error adjustment.
  return match_value < 0.01f ? 0.0f : match_value > 0.99f ? 1.0f : match_value;
}
