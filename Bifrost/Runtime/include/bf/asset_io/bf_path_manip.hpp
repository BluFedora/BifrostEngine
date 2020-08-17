#ifndef BF_PATH_MANIP_HPP
#define BF_PATH_MANIP_HPP

#include "bifrost/data_structures/bifrost_string.hpp"

namespace bf::path
{
  //
  // All of these functions assume a 'canonocalized' path
  //

  StringRange relative(StringRange abs_root_path, StringRange abs_sub_path);
  String      append(StringRange directory, StringRange rel_path);
}

#endif /*  */
