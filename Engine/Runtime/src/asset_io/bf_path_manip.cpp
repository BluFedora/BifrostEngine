#include "bf/asset_io/bf_path_manip.hpp"

#include "bf/asset_io/bf_file.hpp"  // File, file::directoryOfFile

#include <cassert>  // assert

namespace bf::path
{
  StringRange relative(StringRange abs_root_path, StringRange abs_sub_path)
  {
    assert(abs_root_path.length() <= abs_sub_path.length());

    static constexpr std::size_t k_OffsetFromSlash = 1;

    const std::size_t root_path_length = abs_root_path.length();
    const std::size_t full_path_length = abs_sub_path.length();
    const char* const path_bgn         = abs_sub_path.begin() + root_path_length + k_OffsetFromSlash;
    const char* const path_end         = path_bgn + (full_path_length - root_path_length - k_OffsetFromSlash);

    return {path_bgn, path_end};
  }

  String append(StringRange directory, StringRange rel_path)
  {
    String ret = directory;

    ret.append(k_Separator);
    ret.append(rel_path);

    return ret;
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

      const std::size_t bytes_left_over     = out_path_usable_size - dir_bytes_to_write - 1; // Minus one for the 'k_Separator'
      const std::size_t file_bytes_to_write = std::min(bytes_left_over, file_name.length());

      std::memcpy(end_of_path, file_name.begin(), file_bytes_to_write);

      end_of_path += file_bytes_to_write;
    }

    end_of_path[0] = '\0';

    return {std::size_t(end_of_path - out_path), (total_length + std::size_t(1)) > out_path_usable_size};  // Plus one for the k_Separator
  }

  StringRange directory(StringRange file_path)
  {
    return file::directoryOfFile(file_path);
  }

  StringRange extensionEx(StringRange file_path)
  {
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
    const char* dot_start = file_path.end();

    while (dot_start != file_path.begin())
    {
      if (*dot_start == '/')
      {
        return {dot_start + 1, file_path.end()};
      }

      --dot_start;
    }

    return StringRange{};
  }

  StringRange nameWithoutExtension(StringRange file_path)
  {
    const auto last_slash = std::find(file_path.rbegin(), file_path.rend(), '/');
    const auto last_dot   = std::find(file_path.begin(), file_path.end(), '.');

    if (last_dot < last_slash.base())
    {
      return {last_slash.base(), file_path.end()};
    }

    return {last_slash.base(), last_dot};
  }
}  // namespace bf::path
