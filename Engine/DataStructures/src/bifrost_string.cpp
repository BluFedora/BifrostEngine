#include "bifrost/data_structures/bifrost_string.hpp"

#include "bifrost/memory/bifrost_imemory_manager.hpp"  // IMemoryManager

#include <cassert>  // assert
#include <cstdio>   // vsnprintf

namespace bf
{
  StringLink::StringLink(StringRange data, StringLink*& head, StringLink*& tail) :
    string{data},
    next{nullptr}
  {
    {
      if (!head)
      {
        head = this;
      }

      if (tail)
      {
        tail->next = this;
      }

      tail = this;
    }
  }
}  // namespace bf

namespace bf::string_utils
{
  char* fmtAlloc(IMemoryManager& allocator, std::size_t* out_size, const char* fmt, ...)
  {
    assert(fmt != nullptr && "A null format is not allowed.");

    std::va_list args;

    va_start(args, fmt);
    char* const buffer = fmtAllocV(allocator, out_size, fmt, args);
    va_end(args);

    return buffer;
  }

  char* fmtAllocV(IMemoryManager& allocator, std::size_t* out_size, const char* fmt, std::va_list args)
  {
    assert(fmt != nullptr && "A null format is not allowed.");

    char*        buffer = nullptr;
    std::va_list args_cpy;

    va_copy(args_cpy, args);
    const int string_len = std::vsnprintf(nullptr, 0, fmt, args);

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
    // TODO(SR): I don't like this strlen here but this is for nice API.
    allocator.deallocate(ptr, std::strlen(ptr) + 1u);
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

  TokenizeResult tokenizeAlloc(IMemoryManager& allocator, const StringRange& string, char delimiter)
  {
    TokenizeResult result{nullptr, nullptr, 0u};

    tokenize(string, delimiter, [&result, &allocator](const StringRange& token) {
      allocator.allocateT<StringLink>(token, result.head, result.tail);
      ++result.size;
    });

    return result;
  }

  void tokenizeFree(IMemoryManager& allocator, const TokenizeResult& tokenized_list)
  {
    StringLink* link_head = tokenized_list.head;

    while (link_head)
    {
      StringLink* const next = link_head->next;

      allocator.deallocateT(link_head);
      link_head = next;
    }
  }

  StringRange clone(IMemoryManager& allocator, const StringRange& string)
  {
    const std::size_t length = string.length();
    char*             buffer = static_cast<char*>(allocator.allocate(length + 1));

    std::strncpy(buffer, string.begin(), length);
    buffer[length] = '\0';

    return {buffer, length};
  }
}  // namespace bf::string_utils
