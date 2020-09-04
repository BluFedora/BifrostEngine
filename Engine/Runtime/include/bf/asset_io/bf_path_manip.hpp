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
  StringRange directory(StringRange file_path);

  /*!
   * @brief
   *   This is a slower version of 'bf::path::extension' but will include
   *   a file entension with mutiple dots.
   *   
   * @param file_path
   *   The path to search for.
   * 
   * @return
   *   Includes the (dot) e.g '.ext.ext', if no extension was found a null path is returned.
   */
  StringRange extensionEx(StringRange file_path);

  StringRange name(StringRange file_path);

  // Ex:
  //   'hello this is a normal name' => 'hello this is a normal name'
  //   '.ThisShouldBeAnEmpryName' = > '' 
  //   '/ThisISTheCommonCase.dsadas.dasdsa.adssa.dddd' = > 'ThisISTheCommonCase' 
  //   '.ThisISThe/CommonCase' = > 'CommonCase'
  StringRange nameWithoutExtension(StringRange file_path);
}  // namespace bf::path

#endif /*  */
