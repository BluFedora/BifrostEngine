/******************************************************************************/
/*!
 * @file   sr_meta_config.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   This is the place where you can provide specializations for the Meta system
 *   such as Array like data types.
 *
 * @date 2021-26-01
 *
 * @copyright Copyright (c) 2021
 */
/******************************************************************************/
#ifndef SR_META_CONFIG_HPP
#define SR_META_CONFIG_HPP

#include "sr_meta_interface.hpp"

#include <vector>

namespace sr::Meta
{
  //
  // Example of specializing an array data type.
  //
  template<typename T>
  struct TypeResolver<std::vector<T>>
  {
    static Type* get()
    {
      static sr::Meta::ArrayT<std::vector<T>, T> s_Type = {"std::vector<T>"};
      return &s_Type;
    }
  };
}  // namespace sr::Meta

#endif /* SR_META_CONFIG_HPP */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2021 Shareef Abdoul-Raheem

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
