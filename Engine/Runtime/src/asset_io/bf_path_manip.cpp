#include "bf/asset_io/bf_path_manip.hpp"

#include "bf/asset_io/bf_file.hpp"

namespace bf::path
{
  StringRange relative(StringRange abs_root_path, StringRange abs_sub_path)
  {
    static constexpr std::size_t k_OffsetFromSlash = 1;

    const std::size_t root_path_length = abs_root_path.length();
    const std::size_t full_path_length = abs_sub_path.length();
    const char* const path_bgn         = abs_sub_path.begin() + root_path_length + k_OffsetFromSlash;
    const char* const path_end         = path_bgn + (full_path_length - root_path_length - k_OffsetFromSlash);

    return {path_bgn, path_end};

    return StringRange();
  }

  String append(StringRange directory, StringRange rel_path)
  {
    String ret = directory;

    ret.append('/');
    ret.append(rel_path);

    return ret;
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
