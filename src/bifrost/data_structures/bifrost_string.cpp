#include "bifrost/data_structures/bifrost_string.hpp"

#include "bifrost/memory/bifrost_imemory_manager.hpp"  // IMemoryManager

#include <cstdarg>  // va_list
#include <cstdio>   // vsnprintf

namespace bifrost::string_utils
{
  char* alloc_fmt(IMemoryManager& allocator, std::size_t* out, const char* fmt, ...)
  {
    // TIDE_ASSERT(fmt != nullptr, "A null format is not allowed.");

    std::va_list args, args_cpy;
    char*        buffer = nullptr;

    va_start(args, fmt);
    va_copy(args_cpy, args);
    const int string_len = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);

    if (string_len > 0)
    {
      buffer = static_cast<char*>(allocator.allocate(sizeof(char) * (string_len + std::size_t(1u))));

      if (buffer)
      {
        vsprintf(buffer, fmt, args_cpy);

        buffer[string_len] = '\0';
      }
    }

    va_end(args_cpy);

    if (out)
    {
      *out = buffer ? string_len : 0u;
    }

    return buffer;
  }

  void free_fmt(IMemoryManager& allocator, char* ptr)
  {
    allocator.deallocate(ptr);
  }
}  // namespace bifrost::string_utils
