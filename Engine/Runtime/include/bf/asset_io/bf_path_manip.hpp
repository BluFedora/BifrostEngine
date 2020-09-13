#ifndef BF_PATH_MANIP_HPP
#define BF_PATH_MANIP_HPP

#include "bf/StringRange.hpp"  // StringRange, size_t

/*!
 * @brief
 *  Basic abstraction over a file / folder path.
 *
 *  Glorified string utilities with some extras to make
 *  working with paths cross-platform and less painful.
 */
namespace bf::path
{
  static constexpr std::size_t k_MaxLength = 512;  //!< The maximum allowed length for a single path.
  static constexpr char        k_Separator = '/';

  //
  // All of these functions assume a 'canonicalized' path
  //

  StringRange relative(StringRange abs_root_path, StringRange abs_sub_path);
  String      append(StringRange directory, StringRange rel_path);

  /// `out_path_size` must be at least 1.
  std::size_t append(char* out_path, std::size_t out_path_size, StringRange directory, StringRange file_name);  // Out will always be nul terminated wven if we were not able to fit.
  StringRange directory(StringRange file_path);

  /*!
   * @brief
   *   This is a slower version of 'bf::path::extension' but will include
   *   a file extension with mutiple dots.
   *   
   * @param file_path
   *   The path to search through for an extension.
   * 
   * @return
   *   Includes the (dot) e.g '.ext.ext'.
   *   if no extension was found then a null path is returned.
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

#endif /* BF_PATH_MANIP_HPP */
