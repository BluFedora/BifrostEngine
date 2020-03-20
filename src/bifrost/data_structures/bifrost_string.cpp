#include "bifrost/data_structures/bifrost_string.hpp"

#include "bifrost/memory/bifrost_imemory_manager.hpp"  // IMemoryManager

#include <cstdarg>  // va_list
#include <cstdio>   // vsnprintf

namespace bifrost::string_utils
{
  char* fmtAlloc(IMemoryManager& allocator, std::size_t* out_size, const char* fmt, ...)
  {
    // TIDE_ASSERT(fmt != nullptr, "A null format is not allowed.");

    std::va_list args, args_cpy;
    char*        buffer = nullptr;

    va_start(args, fmt);
    va_copy(args_cpy, args);
    const int string_len = std::vsnprintf(nullptr, 0, fmt, args);
    va_end(args);

    if (string_len > 0)
    {
      buffer = static_cast<char*>(allocator.allocate(sizeof(char) * (string_len + std::size_t(1u))));

      if (buffer)
      {
        std::vsprintf(buffer, fmt, args_cpy);

        buffer[string_len] = '\0';
      }
    }

    va_end(args_cpy);

    if (out_size)
    {
      *out_size = buffer ? string_len : 0u;
    }

    return buffer;
  }

  void fmtFree(IMemoryManager& allocator, char* ptr)
  {
    allocator.deallocate(ptr);
  }

  bool fmtBuffer(char* buffer, const size_t buffer_size, std::size_t* out_size, const char* fmt, ...)
  {
    std::va_list args;

    va_start(args, fmt);
    const int string_len = std::vsnprintf(buffer, buffer_size, fmt, args);
    va_end(args);

    if (out_size)
    {
      *out_size = string_len < 0 ? 0 : string_len;
    }

    return 0 <= string_len && string_len < int(buffer_size);
  }
}  // namespace bifrost::string_utils
