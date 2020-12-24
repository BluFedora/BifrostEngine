/******************************************************************************/
/*!
 * @file   bf_path_manip.hpp
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
#ifndef BF_PATH_MANIP_HPP
#define BF_PATH_MANIP_HPP

#include "bf/StringRange.hpp"  // StringRange, size_t

/*!
 * @brief
 *  Basic abstraction over a file / folder path.
 *
 *  Glorified string utilities with some extras to make
 *  working with paths semi portable and less painful.
 */
namespace bf::path
{
  //
  // `Canonical` Path Definition:
  //
  //   - Path separators use UNIX-style '/' even on Windows for portability.
  //   - A Path to a folder does NOT end in a separator just the name of the folder.
  //   - Cannot contain these characters in an identifier: [NUL, '<', '>', ':', QUOTE, k_Separator, '\\', '|', '?', '*', '.']
  //     * This can be checked with the 'path::isValidName' function.
  //
  // Special Path Conventions:
  //
  //   "assets://"         - Refers to the root project folder.
  //   "internal://<uuid>" - Refers to an asset with <uuid> for sub asset access.
  //   otherwise           - It is assumed you are using a path native to the OS, support it not guaranteed by the engine.
  //

  static constexpr std::size_t k_MaxLength     = 512;  //!< The maximum allowed length for a single path.
  static constexpr char        k_Separator     = '/';
  static constexpr StringRange k_AssetsRoot    = "assets://";
  static constexpr StringRange k_SubAssetsRoot = "internal://";

  //
  // All of these functions assume 'canonicalized' paths
  //

  StringRange relative(StringRange abs_root_path, StringRange abs_sub_path);

  /// Really simple append with a `k_Separator` in between.
  String append(StringRange directory, StringRange rel_path);

  /*!
   * @brief
   *   Bundle of information from a call to append.
   */
  struct AppendResult
  {
    std::size_t path_length;   //!< The length of the path not including the nul terminator.
    bool        is_truncated;  //!< Whether or not the full path was not able to fit within the 'out_path' buffer.
  };

  /// Preconditions:
  ///   The lengths of `directory` and `file_name` added together must not overflow.
  ///   Both `directory` and `file_name` must be of non zero size.
  ///   `out_path_size` must be at least 1.
  ///
  /// out_path will always be nul terminated even if truncated.
  ///
  AppendResult append(char* out_path, std::size_t out_path_size, StringRange directory, StringRange file_name);

  StringRange directory(StringRange file_path);

  /*!
   * @brief
   *   This is a slower version of 'bf::path::extension' but will include
   *   a file extension with multiple dots.
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

  /// Ex:
  ///   'hello this is a normal name' => 'hello this is a normal name'
  ///   '.ThisShouldBeAnEmptyName' = > ''
  ///   '/ThisISTheCommonCase.dsadas.dasdsa.adssa.dddd' = > 'ThisISTheCommonCase'
  ///   '.ThisISThe/CommonCase' = > 'CommonCase'
  StringRange nameWithoutExtension(StringRange file_path);

  bool startWith(StringRange file_path, StringRange prefix);

}  // namespace bf::path

#endif /* BF_PATH_MANIP_HPP */

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
