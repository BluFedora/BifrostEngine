#include "bf/data_structures/bifrost_string.hpp"

#include "bf/IMemoryManager.hpp"  // IMemoryManager

#include <cassert>  // assert
#include <cctype>   // tolower
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

    std::va_list args_cpy;
    va_copy(args_cpy, args);

    char*       buffer = nullptr;
    std::size_t require_str_len;
    fmtBufferV(nullptr, 0u, &require_str_len, fmt, args);

    if (require_str_len != 0)
    {
      const std::size_t buffer_size = require_str_len + std::size_t(1u);
      buffer                        = static_cast<char*>(allocator.allocate(buffer_size));

      if (buffer)  // Only do work if the allocation did not fail.
      {
        fmtBufferV(buffer, buffer_size, &require_str_len, fmt, args_cpy);
      }
    }

    if (out_size)
    {
      *out_size = require_str_len;
    }

    va_end(args_cpy);

    return buffer;
  }

  void fmtFree(IMemoryManager& allocator, char* ptr)
  {
    // TODO(SR): I don't like this strlen here but this is for nice API.
    allocator.deallocate(ptr, std::strlen(ptr) + 1u);
  }

  bool fmtBuffer(char* buffer, const size_t buffer_size, std::size_t* out_size, const char* fmt, ...)
  {
    assert(fmt != nullptr && "A null format is not allowed.");

    std::va_list args;

    va_start(args, fmt);
    const bool result = fmtBufferV(buffer, buffer_size, out_size, fmt, args);
    va_end(args);

    return result;
  }

  bool fmtBufferV(char* buffer, size_t buffer_size, std::size_t* out_size, const char* fmt, std::va_list args)
  {
    assert(fmt != nullptr && "A null format is not allowed.");

    const int string_len = std::vsnprintf(buffer, buffer_size, fmt, args);

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
      return true;
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

  float stringMatchPercent(const char* str1, std::size_t str1_len, const char* str2, std::size_t str2_len, float capital_letter_mismatch_cost)
  {
    const float       max_size     = float(std::max(str1_len, str2_len));
    const char* const str1_end     = str1 + str1_len;
    const char* const str2_end     = str2 + str2_len;
    const float       cost_match   = 1.0f / max_size;
    const float       cost_capital = capital_letter_mismatch_cost / max_size;
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

  float stringMatchPercent(StringRange str1, StringRange str2, float capital_letter_mismatch_cost)
  {
    return stringMatchPercent(str1.begin(), str1.length(), str2.begin(), str2.length(), capital_letter_mismatch_cost);
  }

  StringRange findSubstringI(StringRange haystack, StringRange needle)
  {
    return findSubstring(haystack, needle, [](char hay_char, char needle_char) {
      return std::tolower(hay_char) == std::tolower(needle_char);
    });
  }

  BufferRange clone(IMemoryManager& allocator, StringRange str)
  {
    const std::size_t length = str.length();
    char*             buffer = static_cast<char*>(allocator.allocate(length + 1));

    std::strncpy(buffer, str.begin(), length);
    buffer[length] = '\0';

    return {buffer, length};
  }
}  // namespace bf::string_utils
