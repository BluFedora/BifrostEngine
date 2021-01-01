#include "bf/gfx/bf_render_queue.hpp"

#include "bf/bf_hash.hpp"
#include "bf/core/bifrost_engine.hpp"

namespace bf
{
  using OpaqueDepthBits     = BitRange<0, 16>;
  using OpaqueMaterialBits  = BitRange<OpaqueDepthBits::k_LastBit, 16>;
  using OpaqueVertexFmtBits = BitRange<OpaqueMaterialBits::k_LastBit, 16>;
  using OpaqueShaderBits    = BitRange<OpaqueVertexFmtBits::k_LastBit, 16>;

  using AlphaBlendMaterialBits  = BitRange<0, 8>;
  using AlphaBlendVertexFmtBits = BitRange<AlphaBlendMaterialBits::k_LastBit, 16>;
  using AlphaBlendShaderBits    = BitRange<AlphaBlendVertexFmtBits::k_LastBit, 16>;
  using AlphaBlendDepthBits     = BitRange<AlphaBlendShaderBits::k_LastBit, 24>;

  std::uint64_t material_to_bits(const DescSetBind& material_state)
  {
    std::uint64_t result;

    if (material_state.mode == DescSetBind::IMMEDIATE)
    {
      const bfDescriptorSetInfo& info = material_state.immediate_mode_set;

      result = hash::addU32(0x0, info.num_bindings);

      for (std::uint32_t i = 0; i < info.num_bindings; ++i)
      {
        const bfDescriptorElementInfo& element = info.bindings[i];

        result = hash::addU32(result, element.type);
        result = hash::addU32(result, element.binding);
        result = hash::addU32(result, element.array_element_start);
        result = hash::addU32(result, element.num_handles);

        for (std::uint32_t j = 0; j < info.num_bindings; ++j)
        {
          const bfGfxBaseHandle handle = element.handles[j];
          const std::uint64_t   offset = element.offsets[j];
          const std::uint64_t   size   = element.sizes[j];

          result = hash::addPointer(result, handle);
          result = hash::addU64(result, offset);
          result = hash::addU64(result, size);
        }
      }
    }
    else
    {
      result = Bits::basic_pointer_hash(material_state.retained_mode_set);
    }

    return result;
  }

  void DescSetBind::bind(bfGfxCommandListHandle command_list, std::uint32_t index)
  {
    switch (mode)
    {
      case IMMEDIATE:
      {
        bfGfxCmdList_bindDescriptorSet(command_list, index, &immediate_mode_set);
        break;
      }
      case RETAINED:
      {
        if (retained_mode_set)
        {
          bfGfxCmdList_bindDescriptorSets(command_list, index, &retained_mode_set, 1u);
        }
        break;
      }
    }
  }

  RenderQueue::RenderQueue(RenderQueueType type, RenderView& view) :
    type{type},
    render_view{view},
    key_stream_memory{},
    command_stream_memory{},
    num_keys{0}
  {
  }

  static const RenderSortKey* find_max_key(const RenderSortKey* keys, std::size_t num_keys)
  {
    const RenderSortKey* best_key = keys;

    for (std::size_t i = 1; i < num_keys; ++i)
    {
      const RenderSortKey* const key = keys + i;

      if (key->key > best_key->key)
      {
        best_key = key;
      }
    }

    return best_key;
  }

  static void counting_sort(LinearAllocator& alloc, RenderSortKey* keys, std::size_t num_keys, std::uint64_t place)
  {
    static const std::size_t k_Max = 10;

    LinearAllocatorScope mem_scope     = alloc;
    RenderSortKey* const result        = alloc.allocateArrayTrivial<RenderSortKey>(num_keys);
    std::uint64_t* const count_indices = alloc.allocateArrayTrivial<std::uint64_t>(num_keys);
    std::uint64_t* const count         = alloc.allocateArrayTrivial<std::uint64_t>(k_Max);

    // Initialize counts to 0,
    // `result` and `count_indices` do not need initialization since all of there elements gets written to before reads.
    std::memset(count, 0, k_Max * sizeof(std::uint64_t));

    for (std::size_t i = 0; i < num_keys; ++i)
    {
      const std::uint64_t key_index = (keys[i].key / place) % k_Max;

      // Cache this calculation for later
      count_indices[i] = key_index;

      // Count each element
      ++count[key_index];
    }

    // Cumulative count
    for (std::size_t i = 1; i < k_Max; ++i)
    {
      count[i] += count[i - 1];
    }

    // !!Evil Backwards Loop!! placing the elements in the correct order
    for (std::size_t i = num_keys; i-- > 0;)
    {
      result[--count[count_indices[i]]] = keys[i];
    }

    // Copy result back out to the input array.
    std::memcpy(keys, result, sizeof(RenderSortKey) * num_keys);
  }

  static void radix_sort(LinearAllocator& alloc, RenderSortKey* keys, std::size_t num_keys)
  {
    if (num_keys > 1)
    {
      const std::uint64_t max_key = find_max_key(keys, num_keys)->key;

      for (std::uint64_t place = 1; (max_key / place) > 0;)
      {
        counting_sort(alloc, keys, num_keys, place);

        const std::uint64_t old_place = place;

        place *= 10;

        if (old_place > place)  // Overflow
        {
          break;
        }
      }
    }
  }

  void RenderQueue::execute(bfGfxCommandListHandle command_list, const bfGfxFrameInfo& frame_info)
  {
    if (!num_keys)
    {
      return;
    }

    bfShaderProgramHandle last_program = nullptr;
    RenderSortKey* const  keys_bgn     = firstKey();

    radix_sort(key_stream_memory, keys_bgn, num_keys);

    /*
    assert(std::is_sorted(keys_bgn, keys_bgn + num_keys, [](const RenderSortKey& a, const RenderSortKey& b) {
             return a.key < b.key;
           }) == true);
    */

    for (std::size_t i = 0; i < num_keys; ++i)
    {
      RenderSortKey* const sort_key    = keys_bgn + i;
      BaseRenderCommand*   current_cmd = sort_key->command;

      while (current_cmd)
      {
        switch (current_cmd->type)
        {
          case RenderCommandType::DrawArrays:
          {
            RC_DrawArrays* const draw_arrays = static_cast<RC_DrawArrays*>(current_cmd);

            bfGfxCmdList_bindDrawCallPipeline(command_list, &draw_arrays->pipeline);

            if (last_program != draw_arrays->pipeline.program)
            {
              render_view.gpu_camera.bindDescriptorSet(command_list, frame_info);
              last_program = draw_arrays->pipeline.program;
            }

            draw_arrays->material_binding.bind(command_list, k_GfxMaterialSetIndex);
            draw_arrays->object_binding.bind(command_list, k_GfxObjectSetIndex);

            bfGfxCmdList_bindVertexBuffers(
             command_list,
             0,
             draw_arrays->vertex_buffers,
             draw_arrays->num_vertex_buffers,
             draw_arrays->vertex_binding_offsets);
            bfGfxCmdList_draw(command_list, draw_arrays->first_vertex, draw_arrays->num_vertices);
            break;
          }
          case RenderCommandType::DrawIndexed:
          {
            RC_DrawIndexed* const draw_elements = static_cast<RC_DrawIndexed*>(current_cmd);

            bfGfxCmdList_bindDrawCallPipeline(command_list, &draw_elements->pipeline);

            if (last_program != draw_elements->pipeline.program)
            {
              render_view.gpu_camera.bindDescriptorSet(command_list, frame_info);
              last_program = draw_elements->pipeline.program;
            }

            draw_elements->material_binding.bind(command_list, k_GfxMaterialSetIndex);
            draw_elements->object_binding.bind(command_list, k_GfxObjectSetIndex);

            bfGfxCmdList_bindVertexBuffers(
             command_list,
             0,
             draw_elements->vertex_buffers,
             draw_elements->num_vertex_buffers,
             draw_elements->vertex_binding_offsets);
            bfGfxCmdList_bindIndexBuffer(command_list, draw_elements->index_buffer, draw_elements->index_buffer_binding_offset, draw_elements->index_type);
            bfGfxCmdList_drawIndexed(command_list, draw_elements->num_indices, draw_elements->index_offset, draw_elements->vertex_offset);
            break;
          }
        }

        current_cmd = current_cmd->next;
      }
    }
  }

  std::uint64_t makeKey(RenderQueueType type, const DescSetBind& material_state, const bfDrawCallPipeline& pipeline, float depth)
  {
    using namespace Bits;

    switch (type)
    {
      case RenderQueueType::NO_BLENDING:
      {
        const std::uint64_t material_bits   = material_to_bits(material_state);
        const std::uint64_t vertex_fmt_bits = hash::reducePointer<std::uint16_t>(pipeline.vertex_layout);
        const std::uint64_t shader_bits     = hash::reducePointer<std::uint16_t>(pipeline.program);
        const std::uint64_t depth_bits      = depth_to_bits(depth, OpaqueDepthBits::k_NumBits);

        return set(
         set(
          set(
           set(
            std::uint64_t(0x0),
            material_bits & max_value<std::uint64_t, OpaqueMaterialBits::k_NumBits>(),
            OpaqueMaterialBits{}),
           vertex_fmt_bits,
           OpaqueVertexFmtBits{}),
          shader_bits,
          OpaqueShaderBits{}),
         depth_bits,
         OpaqueDepthBits{});
      }
      case RenderQueueType::ALPHA_BLENDING:
      {
        const std::uint64_t material_bits   = material_to_bits(material_state);
        const std::uint64_t vertex_fmt_bits = hash::reducePointer<std::uint16_t>(pipeline.vertex_layout);
        const std::uint64_t shader_bits     = hash::reducePointer<std::uint16_t>(pipeline.program);
        const std::uint64_t depth_bits      = depth_to_bits(depth, AlphaBlendDepthBits::k_NumBits);

        return set(
         set(
          set(
           set(
            std::uint64_t(0x0),
            material_bits & max_value<std::uint64_t, AlphaBlendMaterialBits::k_NumBits>(),
            AlphaBlendMaterialBits{}),
           vertex_fmt_bits,
           AlphaBlendVertexFmtBits{}),
          shader_bits,
          AlphaBlendShaderBits{}),
         depth_bits,
         AlphaBlendDepthBits{});
      }
    }

    assert(!"Should never get here.");  // NOLINT(clang-diagnostic-string-conversion)
    return 0x0;
  }

  RC_DrawArrays* RenderQueue::drawArrays(const bfDrawCallPipeline& pipeline, std::uint32_t num_vertex_buffers)
  {
    RC_DrawArrays* const cmd = pushAlloc<RC_DrawArrays>();

    cmd->pipeline               = pipeline;
    cmd->num_vertex_buffers     = num_vertex_buffers;
    cmd->vertex_buffers         = pushAllocArray<bfBufferHandle>(num_vertex_buffers);
    cmd->vertex_binding_offsets = pushAllocArray<bfBufferSize>(num_vertex_buffers);

    for (std::size_t i = 0; i < num_vertex_buffers; ++i)
    {
      cmd->vertex_buffers[i]         = nullptr;
      cmd->vertex_binding_offsets[i] = 0u;
    }

    return cmd;
  }

  RC_DrawIndexed* RenderQueue::drawIndexed(const bfDrawCallPipeline& pipeline, std::uint32_t num_vertex_buffers, bfBufferHandle index_buffer)
  {
    RC_DrawIndexed* const cmd = pushAlloc<RC_DrawIndexed>();

    cmd->pipeline               = pipeline;
    cmd->num_vertex_buffers     = num_vertex_buffers;
    cmd->vertex_buffers         = pushAllocArray<bfBufferHandle>(num_vertex_buffers);
    cmd->vertex_binding_offsets = pushAllocArray<bfBufferSize>(num_vertex_buffers);
    cmd->index_buffer           = index_buffer;

    for (std::size_t i = 0; i < num_vertex_buffers; ++i)
    {
      cmd->vertex_buffers[i]         = nullptr;
      cmd->vertex_binding_offsets[i] = 0u;
    }

    return cmd;
  }

  void RenderQueue::submit(RC_DrawArrays* command, float distance_to_camera)
  {
    pushKey(makeKey(type, command->material_binding, command->pipeline, distance_to_camera), command);
  }

  void RenderQueue::submit(RC_DrawIndexed* command, float distance_to_camera)
  {
    pushKey(makeKey(type, command->material_binding, command->pipeline, distance_to_camera), command);
  }
}  // namespace bf
