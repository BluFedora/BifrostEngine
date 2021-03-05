/******************************************************************************/
/*!
 * @file   bf_spritesheet_asset.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Asset for a spritesheet animation.
 *
 * @version 0.0.1
 * @date    2020-12-19
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#ifndef BF_SPRITESHEET_ASSET_HPP
#define BF_SPRITESHEET_ASSET_HPP

#include "bf/Animation2D.h"   // bfSpritesheet
#include "bf_base_asset.hpp"  // BaseAsset<T>

namespace bf
{
  class SpritesheetAsset : public BaseAsset<SpritesheetAsset>
  {
   private:
    bfSpritesheet* m_Anim2DSpritesheet;

   public:
    SpritesheetAsset();

    bfSpritesheet* spritesheet() const { return m_Anim2DSpritesheet; }

    ClassID::Type classID() const override
    {
      return ClassID::SPRITESHEET_ASSET;
    }
  };

  //
  // Importers
  //
  struct AssetImportCtx;

  void assetImportSpritesheet(AssetImportCtx& ctx);

}  // namespace bf

#endif /* BF_SPRITESHEET_ASSET_HPP */

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
