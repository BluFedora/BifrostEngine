//
// Shareef Abdoul-Raheem
//
// References:
//   [https://blog.molecular-matters.com/2014/11/06/stateless-layered-multi-threaded-rendering-part-1/]
//   [https://realtimecollisiondetection.net/blog/?p=86]
//   [https://www.fluentcpp.com/2016/12/08/strong-types-for-strong-interfaces/]
//
#ifndef BF_RENDER_QUEUE_HPP
#define BF_RENDER_QUEUE_HPP

#include "bf/Array.hpp"            // Array<T>
#include "bf/LinearAllocator.hpp"  // LinearAllocator
#include "bf/MemoryUtils.h"        // bfMegabytes
#include "bf/bf_gfx_api.h"         // Graphics

#include <cassert>      // assert
#include <cstdint>      // uint64_t
#include <limits>       // numeric_limits
#include <type_traits>  // is_unsigned_v

namespace bf
{
  //
  // Forward Declarations
  //
  enum class RenderCommandType;
  struct BaseRenderCommand;

  //
  // Tag type for the bit manipulation functions.
  //
  template<std::size_t k_Offset_, std::size_t k_NumBits_>
  struct BitRange
  {
    static constexpr std::size_t k_Offset  = k_Offset_;
    static constexpr std::size_t k_NumBits = k_NumBits_;
    static constexpr std::size_t k_LastBit = k_Offset + k_NumBits;
  };

  namespace Bits
  {
    //
    // All of these functions return the new values and do not manipulate it's input.
    //

    template<typename T, std::size_t k_NumBits>
    constexpr T max_value() noexcept
    {
      constexpr int k_MaxNumBitsInT = std::numeric_limits<T>::digits;

      static_assert(std::is_unsigned_v<T>, "T must be an unsigned arithmetic type.");
      static_assert(k_NumBits <= k_MaxNumBitsInT, "The num bits overflows the range of T.");

      return T((1 << k_NumBits) - 1);
    }

    template<typename T, std::size_t k_Offset, std::size_t k_NumBits>
    constexpr T mask(BitRange<k_Offset, k_NumBits>) noexcept
    {
      constexpr int         k_MaxNumBitsInT  = std::numeric_limits<T>::digits;
      constexpr std::size_t k_LastBitInRange = k_Offset + k_NumBits;

      static_assert(std::is_unsigned_v<T>, "T must be an unsigned arithmetic type.");
      static_assert(k_LastBitInRange <= k_MaxNumBitsInT, "The range overflows the range of T.");

      return max_value<T, k_NumBits>() << k_Offset;
    }

    // Clears the bits to 0 in the specified range.
    template<typename T, std::size_t k_Offset, std::size_t k_NumBits>
    T cleared(T bits, BitRange<k_Offset, k_NumBits> range) noexcept
    {
      constexpr int         k_MaxNumBitsInT  = std::numeric_limits<T>::digits;
      constexpr std::size_t k_LastBitInRange = k_Offset + k_NumBits;
      constexpr T           k_Mask           = mask<T>(range);

      static_assert(std::is_unsigned_v<T>, "T must be an unsigned arithmetic type.");
      static_assert(k_LastBitInRange <= k_MaxNumBitsInT, "The range overflows the range of T.");

      return bits & ~k_Mask;
    }

    // This function does NOT clear the range of what was previously there
    // Use 'Bits::clearAndSet' for that purpose.
    template<typename T, typename U, std::size_t k_Offset, std::size_t k_NumBits>
    T set(T bits, U value, BitRange<k_Offset, k_NumBits> range) noexcept
    {
      constexpr int         k_MaxNumBitsInT  = std::numeric_limits<T>::digits;
      constexpr std::size_t k_LastBitInRange = k_Offset + k_NumBits;
      constexpr T           k_Mask           = mask<T>(range);

      static_assert(std::is_unsigned_v<T>, "T must be an unsigned arithmetic type.");
      static_assert(k_LastBitInRange <= k_MaxNumBitsInT, "The range overflows the range of T.");

      // NOTE(SR):
      //   This static_assert is annoying to have since by default all integers are signed in C/++.
      // static_assert(std::is_unsigned_v<U>, "U must be an unsigned arithmetic type.");

      assert((T(value) <= max_value<T, k_NumBits>()) && "Value out of range.");

      return bits | ((static_cast<std::size_t>(value) << k_Offset) & k_Mask);
    }

    // Sets the values to the specified range in bits making sure to clear the range prior.
    template<typename T, typename U, std::size_t k_Offset, std::size_t k_NumBits>
    T cleared_set(T bits, U value, BitRange<k_Offset, k_NumBits> range) noexcept
    {
      return set(cleared(bits, range), value, range);
    }

    template<typename To, typename From>
    To cast(const From& from) noexcept
    {
      static_assert(sizeof(From) == sizeof(To), "The two types must but the same size.");
      static_assert(std::is_trivially_copyable_v<From> && std::is_trivially_copyable_v<To>, "memcpy is used to copy.");
      static_assert(std::is_trivially_constructible_v<To>, "'To' must be constructable without side effects.");

      To result;

      std::memcpy(&result, &from, sizeof(To));

      return result;
    }

    inline std::uint32_t depth_to_bits(float value, int num_hi_bits) noexcept
    {
      constexpr std::uint32_t k_NumBitsInFloat = 32;
      constexpr std::uint32_t k_HiBitIndex     = 31;
      constexpr std::uint32_t k_HiBit          = (1u << k_HiBitIndex);

      assert(std::uint32_t(num_hi_bits) <= k_NumBitsInFloat);

      const std::uint32_t float_bits = cast<std::uint32_t>(value);

      // Handle negative numbers, making it correctly sortable.
      // Adapted from [http://stereopsis.com/radix.html]

      const std::uint32_t float_flip_mask = -std::int32_t(float_bits >> k_HiBitIndex) | k_HiBit;

      return (float_bits ^ float_flip_mask) >> (k_NumBitsInFloat - num_hi_bits);
    }

    //
    // For 64bit pointers only 47 of the bits are used
    // so for sorting we get most of the important bits so this is good enough.
    //
    inline std::uint32_t basic_pointer_hash(const void* value)
    {
      constexpr int k_NumBitsInPtr     = std::numeric_limits<std::uintptr_t>::digits;
      constexpr int k_HalfNumBitsInPtr = k_NumBitsInPtr / 2;

      const std::uintptr_t value_bits = cast<std::uintptr_t>(value);

      return static_cast<std::uint32_t>(value_bits ^ (value_bits >> k_HalfNumBitsInPtr));
    }
  }  // namespace Bits

  struct RenderSortKey
  {
    std::uint64_t      key;
    BaseRenderCommand* command;
  };

  struct DescSetBind
  {
    enum
    {
      IMMEDIATE,
      RETAINED,

    } mode;

    union
    {
      bfDescriptorSetHandle retained_mode_set;
      bfDescriptorSetInfo   immediate_mode_set;
    };

    DescSetBind() :
      mode{RETAINED},
      retained_mode_set{nullptr}
    {
    }

    void set(const bfDescriptorSetInfo& info)
    {
      mode               = IMMEDIATE;
      immediate_mode_set = info;
    }

    void set(bfDescriptorSetHandle handle)
    {
      mode              = RETAINED;
      retained_mode_set = handle;
    }

    void bind(bfGfxCommandListHandle command_list, std::uint32_t index);
  };

  enum class RenderCommandType
  {
    DrawIndexed,
    DrawArrays,
  };

  struct BaseRenderCommand
  {
    RenderCommandType  type;
    BaseRenderCommand* next;  // optional continuation command that will not be sorted globally.

    BaseRenderCommand(RenderCommandType type) :
      type{type},
      next{nullptr}
    {
    }
  };

  template<RenderCommandType k_Type>
  struct RenderCommandT : public BaseRenderCommand
  {
    RenderCommandT() :
      BaseRenderCommand(k_Type)
    {
    }
  };

#define DECLARE_RENDER_CMD(name) struct RC_##name : public RenderCommandT<RenderCommandType::name>

  DECLARE_RENDER_CMD(DrawArrays)
  {
    bfDrawCallPipeline pipeline               = {};
    DescSetBind        material_binding       = {};
    DescSetBind        object_binding         = {};
    std::uint32_t      num_vertex_buffers     = 0u;
    bfBufferHandle*    vertex_buffers         = nullptr;
    bfBufferSize*      vertex_binding_offsets = nullptr;
    std::uint32_t      first_vertex           = 0u;
    std::uint32_t      num_vertices           = 0u;
  };

  DECLARE_RENDER_CMD(DrawIndexed)
  {
    bfDrawCallPipeline pipeline                    = {};
    DescSetBind        material_binding            = {};
    DescSetBind        object_binding              = {};
    std::uint32_t      num_vertex_buffers          = 0u;
    bfBufferHandle*    vertex_buffers              = nullptr;
    bfBufferSize*      vertex_binding_offsets      = nullptr;
    bfBufferHandle     index_buffer                = nullptr;
    std::uint32_t      vertex_offset               = 0u;
    std::uint32_t      index_offset                = 0u;
    std::uint32_t      num_indices                 = 0u;
    std::uint64_t      index_buffer_binding_offset = 0u;
    bfGfxIndexType     index_type                  = BF_INDEX_TYPE_UINT32;
  };

#undef DECLARE_RENDER_CMD

  //
  // TODO(SR): A user sort key would be nice to have.
  //
  // The sort keys are very basic and need tuning.
  //   Opaque       Sort Key: [shader(16bit), vertex-format(16bit), material(16bit), depth f-to-b(16bit)]
  //   Transparency Sort Key: [depth b-to-f(24bit), shader(16bit), vertex-format(16bit), material(8bit)]
  //
  // *material here means texture bindings change through descriptor set.
  //

  enum class RenderQueueType
  {
    NO_BLENDING,
    ALPHA_BLENDING,
    SCREEN_OVERLAY,
  };

  struct RenderView;

  struct RenderQueue
  {
    static constexpr std::size_t k_KeyBufferSize     = bfMegabytes(1);
    static constexpr std::size_t k_CommandBufferSize = bfMegabytes(2);

    RenderQueueType                           type;
    RenderView&                               render_view;
    FixedLinearAllocator<k_KeyBufferSize>     key_stream_memory;
    FixedLinearAllocator<k_CommandBufferSize> command_stream_memory;
    std::size_t                               num_keys;

    RenderQueue(RenderQueueType type, RenderView& view);

    void            clear();
    void            execute(bfGfxCommandListHandle command_list, const bfGfxFrameInfo& frame_info);
    RC_DrawArrays*  drawArrays(const bfDrawCallPipeline& pipeline, std::uint32_t num_vertex_buffers);
    RC_DrawIndexed* drawIndexed(const bfDrawCallPipeline& pipeline, std::uint32_t num_vertex_buffers, bfBufferHandle index_buffer);
    void            submit(RC_DrawIndexed* command, float distance_to_camera);
    void            submit(RC_DrawArrays* command, float distance_to_camera);

   private:
    RenderSortKey* firstKey() const { return reinterpret_cast<RenderSortKey*>(key_stream_memory.begin()); }

    void pushKey(std::uint64_t key, BaseRenderCommand* command)
    {
      RenderSortKey* const sort_key = key_stream_memory.allocateT<RenderSortKey>();

      sort_key->key     = key;
      sort_key->command = command;

      ++num_keys;
    }

    template<typename T, typename... Args>
    T* pushAlloc(Args&&... args)
    {
      return command_stream_memory.allocateT<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    T* pushAllocArray(std::size_t num_items)
    {
      return command_stream_memory.allocateArrayTrivial<T>(num_items);
    }
  };
}  // namespace bf

#endif /* BF_RENDER_QUEUE_HPP */
