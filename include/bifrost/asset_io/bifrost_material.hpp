/******************************************************************************/
/*!
* @file   bifrost_material.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*
* @version 0.0.1
* @date    2020-03-10
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#ifndef BIFROST_MATERIAL_HPP
#define BIFROST_MATERIAL_HPP

#include "bifrost/graphics/bifrost_gfx_api.h"
#include "bifrost_asset_handle.hpp"  // AssetInfo<T>

namespace bifrost
{
  namespace detail
  {
    void releaseGfxHandle(Engine& engine, bfGfxBaseHandle handle);

    // clang-format off
    template<typename TSelf, typename THandle>
    class BaseGraphicsResource : public BaseObject<TSelf>, private bfNonCopyMoveable<TSelf>
    // clang-format on
    {
     protected:
      bfGfxDeviceHandle m_GraphicsDevice;
      THandle           m_Handle;

     protected:
      explicit BaseGraphicsResource(bfGfxDeviceHandle device) :
        m_GraphicsDevice{device},
        m_Handle{nullptr}
      {
      }
    };
  }  // namespace detail

  class Texture final : public detail::BaseGraphicsResource<Texture, bfTextureHandle>
  {
    friend class AssetTextureInfo;

   private:
    using BaseT = BaseGraphicsResource<Texture, bfTextureHandle>;

   public:
    explicit Texture(bfGfxDeviceHandle device) :
      BaseT(device)
    {
    }

    std::uint32_t width() const { return m_Handle ? bfTexture_width(m_Handle) : 0u; }
    std::uint32_t height() const { return m_Handle ? bfTexture_width(m_Handle) : 0u; }
  };

  class AssetTextureInfo final : public AssetInfo<Texture, AssetTextureInfo>
  {
   private:
    using BaseT = AssetInfo<Texture, AssetTextureInfo>;

   public:
    using BaseT::BaseT;

    bool load(Engine& engine) override;
  };

  using AssetTextureHandle = AssetHandle<Texture>;

  class ShaderModule final : public detail::BaseGraphicsResource<ShaderModule, bfShaderModuleHandle>
  {
    friend class AssetShaderModuleInfo;

   private:
    using BaseT = BaseGraphicsResource<ShaderModule, bfShaderModuleHandle>;

   public:
    explicit ShaderModule(bfGfxDeviceHandle device) :
      BaseT(device)
    {
    }
  };

  class AssetShaderModuleInfo final : public AssetInfo<ShaderModule, AssetShaderModuleInfo>
  {
   private:
    using BaseT = AssetInfo<ShaderModule, AssetShaderModuleInfo>;

   public:
    using BaseT::BaseT;
  };

  using AssetShaderModuleHandle = AssetHandle<ShaderModule>;

  class ShaderProgram final : public detail::BaseGraphicsResource<ShaderProgram, bfShaderProgramHandle>
  {
    BIFROST_META_FRIEND;
    friend class AssetShaderProgramInfo;

   private:
    using BaseT = BaseGraphicsResource<ShaderProgram, bfShaderProgramHandle>;

   private:
    AssetShaderModuleHandle m_VertexShader;
    AssetShaderModuleHandle m_FragmentShader;

   public:
    explicit ShaderProgram(bfGfxDeviceHandle device) :
      BaseT(device),
      m_VertexShader{nullptr},
      m_FragmentShader{nullptr}
    {
    }

   private:
    const BaseAssetHandle& metaVertexShader() const { return m_VertexShader; }
    void                   setMetaVertexShader(const BaseAssetHandle& value) { m_VertexShader = (AssetShaderModuleHandle&)value; }
    const BaseAssetHandle& metaFragmentShader() const { return m_FragmentShader; }
    void                   setMetaFragmentShader(const BaseAssetHandle& value) { m_FragmentShader = (AssetShaderModuleHandle&)value; }
  };

  class AssetShaderProgramInfo final : public AssetInfo<ShaderProgram, AssetShaderProgramInfo>
  {
    using BaseT = AssetInfo<ShaderProgram, AssetShaderProgramInfo>;

   public:
    using BaseT::BaseT;

    bool load(Engine& engine) override;
  };

  using AssetShaderProgramHandle = AssetHandle<ShaderProgram>;

  class Material final : public BaseObject<Material>
  {
   private:
    AssetShaderProgramHandle m_ShaderProgram;
    AssetTextureHandle       m_DiffuseTexture;

   public:
    explicit Material() :
      BaseObject<Material>(),
      m_ShaderProgram{nullptr},
      m_DiffuseTexture{nullptr}
    {
    }

    const AssetShaderProgramHandle& shaderProgram() const { return m_ShaderProgram; }
    const AssetTextureHandle&       diffuseTexture() const { return m_DiffuseTexture; }
  };

  using AssetMaterialHandle = AssetHandle<Material>;
}  // namespace bifrost

BIFROST_META_REGISTER(bifrost::Texture){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   class_info<ShaderProgram>("Texture"),  //
   ctor<bfGfxDeviceHandle>(),             //
   property("width", &Texture::width),    //
   property("height", &Texture::height)   //
   )
   BIFROST_META_END()}

BIFROST_META_REGISTER(bifrost::ShaderProgram)
{
  BIFROST_META_BEGIN()
    BIFROST_META_MEMBERS(
     class_info<ShaderProgram>("ShaderProgram"),                                                              //
     ctor<bfGfxDeviceHandle>(),                                                                               //
     property("m_VertexShader", &ShaderProgram::metaVertexShader, &ShaderProgram::setMetaVertexShader),       //
     property("m_FragmentShader", &ShaderProgram::metaFragmentShader, &ShaderProgram::setMetaFragmentShader)  //
    )
  BIFROST_META_END()
}

#endif /* BIFROST_MATERIAL_HPP */
