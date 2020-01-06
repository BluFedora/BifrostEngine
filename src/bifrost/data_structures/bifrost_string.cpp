#include "bifrost/data_structures/bifrost_string.hpp"

#include "bifrost/memory/bifrost_imemory_manager.hpp"  // IMemoryManager

#include <cstdarg>  // va_list
#include <cstdio>   // vsnprintf

namespace bifrost::string_utils
{
  // NOTE(Shareef):
  //   The caller is responsible for freeing any memory this allocates.
  char* alloc_fmt(IMemoryManager& allocator, std::size_t* out, const char* fmt, ...)
  {
    // TIDE_ASSERT(fmt != nullptr, "A null format is not allowed.");

    std::va_list args, args_cpy;

    va_start(args, fmt);
    va_copy(args_cpy, args);
    const int string_len = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);

    if (string_len > 0)
    {
      const auto buffer = static_cast<char*>(allocator.alloc(sizeof(char) * (string_len + 1), alignof(char)));

      if (buffer)
      {
        vsprintf(buffer, fmt, args_cpy);
        va_end(args_cpy);

        if (out)
        {
          *out = string_len;
        }

        buffer[string_len] = '\0';
        return buffer;
      }
      // On failure to allocate we still call "va_end".
    }

    va_end(args_cpy);

    if (out)
    {
      *out = 0u;
    }

    return nullptr;
  }
}  // namespace bifrost::string_utils
