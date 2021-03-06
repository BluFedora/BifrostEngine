/******************************************************************************/
/*!
 * @file   bf_base_asset.cpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   Interface for creating asset types the engine can use.
 *
 * @version 0.0.1
 * @date    2020-12-19
 *
 * @copyright Copyright (c) 2019-2021
 */
/******************************************************************************/
#include "bf/asset_io/bf_base_asset.hpp"

#include "bf/asset_io/bf_document.hpp"  // IDocument

#include <cassert> /* assert */

namespace bf
{
  IBaseAsset::IBaseAsset() :
    IBaseObject(),
    m_Name{""},
    m_Document{nullptr},
    m_DocResourceListNode{}
  {
  }

  void IBaseAsset::acquire()
  {
    if (m_Document)
    {
      m_Document->acquire();
    }
  }

  void IBaseAsset::release()
  {
    if (m_Document)
    {
      m_Document->release();
    }
  }

  ResourceReference IBaseAsset::toRef() const
  {
    return ResourceReference{m_Document->uuid(), fileID()};
  }
}  // namespace bf

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2020-2021 Shareef Abdoul-Raheem

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
