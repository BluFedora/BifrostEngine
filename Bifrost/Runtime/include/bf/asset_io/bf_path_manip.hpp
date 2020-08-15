#ifndef BF_PATH_MANIP_HPP
#define BF_PATH_MANIP_HPP

#include "bifrost/data_structures/bifrost_string.hpp"

namespace bf::path
{
  StringRange relative(StringRange abs_root_path, StringRange abs_sub_path);
}

#endif /*  */
