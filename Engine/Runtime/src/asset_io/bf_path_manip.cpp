/******************************************************************************/
/*!
 * @file   bf_path_manip.cpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   String manipulation functions with a focus on file paths.
 *
 * @version 0.0.1
 * @date    2020-12-19
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#include "bf/asset_io/bf_path_manip.hpp"

#include "bf/asset_io/bf_file.hpp"  // File, file::directoryOfFile

#include <cassert>  // assert

namespace bf::path
{
  static constexpr std::size_t k_OffsetFromSlash = 1;  //!< Use of this is to indicate a calculation that needs to account for a k_Separator character.

  StringRange relative(StringRange abs_root_path, StringRange abs_sub_path)
  {
    assert(abs_root_path.length() <= abs_sub_path.length());

    const std::size_t root_path_length = abs_root_path.length();
    const std::size_t full_path_length = abs_sub_path.length();
    const char* const path_bgn         = abs_sub_path.begin() + root_path_length + k_OffsetFromSlash;
    const char* const path_end         = path_bgn + (full_path_length - root_path_length - k_OffsetFromSlash);

    return {path_bgn, path_end};
  }

  String append(StringRange directory, StringRange rel_path)
  {
    String result = directory;

    result.append(k_Separator);
    result.append(rel_path);

    return result;
  }

  AppendResult append(char* const out_path, std::size_t out_path_size, StringRange directory, StringRange file_name)
  {
    const std::size_t total_length = directory.length() + file_name.length();

    assert(out_path_size >= 1);
    assert(total_length > std::max(directory.length(), file_name.length()) && "Length overflow detection.");

    const std::size_t out_path_usable_size = out_path_size - 1;
    const std::size_t dir_bytes_to_write   = std::min(out_path_usable_size, directory.length());
    char*             end_of_path          = out_path;

    std::memcpy(end_of_path, directory.begin(), dir_bytes_to_write);

    end_of_path += dir_bytes_to_write;

    // This being a '<' rather than a '<=' allows for space for the '/'
    if (dir_bytes_to_write < out_path_usable_size)
    {
      *end_of_path++ = k_Separator;

      const std::size_t bytes_left_over     = out_path_usable_size - dir_bytes_to_write - k_OffsetFromSlash;  // Minus k_OffsetFromSlash for the 'k_Separator'
      const std::size_t file_bytes_to_write = std::min(bytes_left_over, file_name.length());

      std::memcpy(end_of_path, file_name.begin(), file_bytes_to_write);

      end_of_path += file_bytes_to_write;
    }

    end_of_path[0] = '\0';

    const std::size_t path_length  = end_of_path - out_path;
    const bool        is_truncated = (total_length + k_OffsetFromSlash) > out_path_usable_size;  // Plus k_OffsetFromSlash for the k_Separator

    return {path_length, is_truncated};
  }

  StringRange directory(StringRange file_path)
  {
    const char* end = file_path.end();

    while (end != file_path.str_bgn)
    {
      if (*end == '/')
      {
        break;
      }

      --end;
    }

    return {file_path.str_bgn, end};
  }

  StringRange extensionEx(StringRange file_path)
  {
    // TODO(SR):
    //    This can be optimized for the common case of going backward through the string.
    //    The cost of doing that is a more complicated implementation.
    //    Probably would include use of std::find_end??

    const char*       dot_start = file_path.begin();
    const char* const path_end  = file_path.end();

    while (dot_start != path_end)
    {
      if (*dot_start == '.')
      {
        return {dot_start, path_end};
      }

      ++dot_start;
    }

    return StringRange{};
  }

  StringRange name(StringRange file_path)
  {
    const auto* const last_slash = std::find(file_path.rbegin(), file_path.rend(), k_Separator).base();

    return StringRange{last_slash, file_path.end()};
  }

  StringRange nameWithoutExtension(StringRange file_path)
  {
    const auto* const last_slash = std::find(file_path.rbegin(), file_path.rend(), k_Separator).base();
    const auto* const last_dot   = std::find(last_slash, file_path.end(), '.');

    return {last_slash, last_dot};
  }

  bool startWith(StringRange file_path, StringRange prefix)
  {
    const std::size_t prefix_len = prefix.length();

    if (file_path.length() >= prefix_len)
    {
      return std::strncmp(file_path.begin(), prefix.begin(), prefix_len) == 0;
    }

    return false;
  }
}  // namespace bf::path

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2020 Shareef Abdoul-Raheem

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
/******************************************************************************/
