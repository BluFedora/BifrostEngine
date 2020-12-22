/******************************************************************************/
/*!
 * @file   bf_gfx_assets.cpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Defines some built in Asset Types used mainly by the graphics subsystem(s).
 *
 * @version 0.0.1
 * @date    2020-12-19
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#include "bf/asset_io/bf_gfx_assets.hpp"

namespace bf
{
  // TODO(SR): This is copied from "bifrost_standard_renderer.cpp"
  static const bfTextureSamplerProperties k_SamplerNearestRepeat = bfTextureSamplerProperties_init(BF_SFM_NEAREST, BF_SAM_REPEAT);

  TextureAsset::TextureAsset(bfGfxDeviceHandle gfx_device) :
    BaseAsset<TextureAsset>(),
    m_ParentDevice{gfx_device},
    m_TextureHandle{nullptr}
  {
  }

  void TextureAsset::onLoad()
  {
    if (loadImpl())
    {
      markIsLoaded();
    }
    else
    {
      markFailedToLoad();
    }
  }

  void TextureAsset::onUnload()
  {
    if (m_TextureHandle)
    {
      // TODO(SR): This will not scale well.
      bfGfxDevice_flush(m_ParentDevice);

      bfGfxDevice_release(m_ParentDevice, m_TextureHandle);
    }
  }

  bool TextureAsset::loadImpl()
  {
    const String&               full_path         = fullPath();
    const bfTextureCreateParams tex_create_params = bfTextureCreateParams_init2D(
     BF_IMAGE_FORMAT_R8G8B8A8_UNORM,
     k_bfTextureUnknownSize,
     k_bfTextureUnknownSize);

    m_TextureHandle = bfGfxDevice_newTexture(m_ParentDevice, &tex_create_params);

    if (bfTexture_loadFile(m_TextureHandle, full_path.c_str()))
    {
      bfTexture_setSampler(m_TextureHandle, &k_SamplerNearestRepeat);

      return true;
    }

    return false;
  }
}  // namespace bf

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
