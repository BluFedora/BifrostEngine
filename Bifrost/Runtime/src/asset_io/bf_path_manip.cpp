#include "bf/asset_io/bf_path_manip.hpp"

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
}  // namespace bf::path
