/******************************************************************************/
/*!
 * @file   bf_gfx_assets.hpp
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
#ifndef BF_GFX_ASSETS_HPP
#define BF_GFX_ASSETS_HPP

#include "bf/bf_gfx_api.h"
#include "bf_base_asset.hpp"

namespace bf
{
  class TextureAsset final : public BaseAsset<TextureAsset>
  {
   private:
    bfGfxDeviceHandle m_ParentDevice;
    bfTextureHandle   m_TextureHandle;

   public:
    explicit TextureAsset(bfGfxDeviceHandle gfx_device);

    bfGfxDeviceHandle gfxDevice() const { return m_ParentDevice; }
    bfTextureHandle   handle() const { return m_TextureHandle; }
    std::uint32_t     width() const { return m_TextureHandle ? bfTexture_width(m_TextureHandle) : 0u; }
    std::uint32_t     height() const { return m_TextureHandle ? bfTexture_height(m_TextureHandle) : 0u; }

    // Low Level Control, Dont mess up

    void assignNewHandle(bfTextureHandle handle)
    {
      onUnload();
      m_TextureHandle = handle;
    }

   private:
    void onLoad() override;
    void onUnload() override;
    bool loadImpl();
  };
}  // namespace bf

/*
 BIFROST_META_REGISTER(bfShaderType){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   enum_info<bfShaderType>("bfShaderType"),                                                         //
   enum_element("BF_SHADER_TYPE_VERTEX", BF_SHADER_TYPE_VERTEX),                                    //
   enum_element("BF_SHADER_TYPE_TESSELLATION_CONTROL", BF_SHADER_TYPE_TESSELLATION_CONTROL),        //
   enum_element("BF_SHADER_TYPE_TESSELLATION_EVALUATION", BF_SHADER_TYPE_TESSELLATION_EVALUATION),  //
   enum_element("BF_SHADER_TYPE_GEOMETRY", BF_SHADER_TYPE_GEOMETRY),                                //
   enum_element("BF_SHADER_TYPE_FRAGMENT", BF_SHADER_TYPE_FRAGMENT),                                //
   enum_element("BF_SHADER_TYPE_COMPUTE", BF_SHADER_TYPE_COMPUTE)                                   //
   )
   BIFROST_META_END()}
*/

BIFROST_META_REGISTER(bf::TextureAsset)
{
  BIFROST_META_BEGIN()
    BIFROST_META_MEMBERS(
     class_info<TextureAsset>("Texture"),       //
     ctor<bfGfxDeviceHandle>(),                 //
     property("width", &TextureAsset::width),   //
     property("height", &TextureAsset::height)  //
    )
  BIFROST_META_END()
}

#endif /* BF_GFX_ASSETS_HPP */

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
