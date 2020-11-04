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
#ifndef BF_MATERIAL_HPP
#define BF_MATERIAL_HPP

#include "bifrost/graphics/bifrost_gfx_api.h"  //
#include "bifrost_asset_handle.hpp"            // AssetHandle<T>
#include "bifrost_asset_info.hpp"              // AssetInfo<T>

#include "bf/BaseObject.hpp"  // BaseObject<TSelf>
#include "bf/Quaternion.h"    // bfQuaternionf

namespace bf
{
  using Matrix4x4f        = ::Mat4x4;
  using AnimationTimeType = double;

  namespace detail
  {
    // clang-format off
    template<typename TSelf, typename THandle>
    class BaseGraphicsResource : public BaseObject<TSelf>, private bfNonCopyMoveable<TSelf>
    // clang-format on
    {
     protected:
      bfGfxDeviceHandle m_GraphicsDevice;
      THandle           m_Handle;

     protected:
      using Base = BaseGraphicsResource<TSelf, THandle>;

      explicit BaseGraphicsResource(bfGfxDeviceHandle device) :
        m_GraphicsDevice{device},
        m_Handle{nullptr}
      {
      }

      void destroyHandle()
      {
        if (m_Handle)
        {
          // TODO: This probably will not scale well.
          bfGfxDevice_flush(m_GraphicsDevice);
          bfGfxDevice_release(m_GraphicsDevice, m_Handle);
          m_Handle = nullptr;
        }
      }

      virtual ~BaseGraphicsResource()
      {
        destroyHandle();
      }

     public:
      bfGfxDeviceHandle gfxDevice() const { return m_GraphicsDevice; }
      THandle           handle() const { return m_Handle; }

      void setHandle(THandle h)
      {
        m_Handle = h;
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

    using BaseT::destroyHandle;

    std::uint32_t width() const { return m_Handle ? bfTexture_width(m_Handle) : 0u; }
    std::uint32_t height() const { return m_Handle ? bfTexture_height(m_Handle) : 0u; }
  };

  class AssetTextureInfo final : public AssetInfo<Texture, AssetTextureInfo>
  {
   private:
    using BaseT = AssetInfo<Texture, AssetTextureInfo>;

   public:
    using BaseT::BaseT;

    bool load(Engine& engine) override;
    bool reload(Engine& engine) override;

   private:
    bool loadImpl(Engine& engine, Texture& texture);
  };

  using AssetTextureHandle = AssetHandle<Texture>;

  class ShaderModule final : public detail::BaseGraphicsResource<ShaderModule, bfShaderModuleHandle>
  {
    friend class AssetShaderModuleInfo;

   private:
    using BaseT = BaseGraphicsResource<ShaderModule, bfShaderModuleHandle>;

   public:
    explicit ShaderModule(bfGfxDeviceHandle device);
  };

  class AssetShaderModuleInfo final : public AssetInfo<ShaderModule, AssetShaderModuleInfo>
  {
    BF_META_FRIEND;

   private:
    using BaseT = AssetInfo<ShaderModule, AssetShaderModuleInfo>;

   private:
    BifrostShaderType m_Type = BIFROST_SHADER_TYPE_VERTEX;

   public:
    using BaseT::BaseT;

    bool load(Engine& engine) override;
    void serialize(Engine& engine, ISerializer& serializer) override;
  };

  using AssetShaderModuleHandle = AssetHandle<ShaderModule>;

  class ShaderProgram final : public detail::BaseGraphicsResource<ShaderProgram, bfShaderProgramHandle>
  {
    BF_META_FRIEND;
    friend class AssetShaderProgramInfo;

   private:
    using BaseT = BaseGraphicsResource<ShaderProgram, bfShaderProgramHandle>;

   private:
    AssetShaderModuleHandle m_VertexShader;
    AssetShaderModuleHandle m_FragmentShader;
    std::uint32_t           m_NumDescriptorSets;

   public:
    explicit ShaderProgram(bfGfxDeviceHandle device);

    std::uint32_t numDescriptorSets() const { return m_NumDescriptorSets; }
    void          setNumDescriptorSets(std::uint32_t value);

   private:
    void createImpl();
  };

  class AssetShaderProgramInfo final : public AssetInfo<ShaderProgram, AssetShaderProgramInfo>
  {
   public:
    using Base::Base;

    bool load(Engine& engine) override;
    bool save(Engine& engine, ISerializer& serializer) override;
  };

  using AssetShaderProgramHandle = AssetHandle<ShaderProgram>;

  class Material final : public BaseObject<Material>
  {
    BF_META_FRIEND;
    friend class AssetModelInfo;

   private:
    AssetTextureHandle m_AlbedoTexture;
    AssetTextureHandle m_NormalTexture;
    AssetTextureHandle m_MetallicTexture;
    AssetTextureHandle m_RoughnessTexture;
    AssetTextureHandle m_AmbientOcclusionTexture;

   public:
    explicit Material() :
      Base(),
      m_AlbedoTexture{nullptr},
      m_NormalTexture{nullptr},
      m_MetallicTexture{nullptr},
      m_RoughnessTexture{nullptr},
      m_AmbientOcclusionTexture{nullptr}
    {
    }

    const AssetTextureHandle& albedoTexture() const { return m_AlbedoTexture; }
    const AssetTextureHandle& normalTexture() const { return m_NormalTexture; }
    const AssetTextureHandle& metallicTexture() const { return m_MetallicTexture; }
    const AssetTextureHandle& roughnessTexture() const { return m_RoughnessTexture; }
    const AssetTextureHandle& ambientOcclusionTexture() const { return m_AmbientOcclusionTexture; }
  };

  class AssetMaterialInfo final : public AssetInfo<Material, AssetMaterialInfo>
  {
   public:
    using Base::Base;

    bool load(Engine& engine) override;
    bool save(Engine& engine, ISerializer& serializer) override;
  };

  using AssetMaterialHandle = AssetHandle<Material>;

  class Animation3D final : public BaseObject<Animation3D>
  {
    friend class AssetModelInfo;

   public:
    template<typename T>
    struct Track
    {
      struct Key
      {
        AnimationTimeType time;
        T                 value;
      };

      Key* keys = {};

      std::size_t numKeys(IMemoryManager& mem) const noexcept { return mem.arraySize(keys); }

      Key* create(IMemoryManager& mem, std::size_t num_keys)
      {
        return keys = mem.allocateArrayTrivial<Key>(num_keys);
      }

      // Dont call this fuction when only one key exists
      std::size_t findKey(AnimationTimeType time, IMemoryManager& mem) const
      {
        const std::size_t num_keys = numKeys(mem);

        assert(num_keys > 1);

        for (std::size_t i = 0; i < num_keys - 1; ++i)
        {
          if (time < keys[i + 1].time)
          {
            return i;
          }
        }

        assert(!"Invalid time passed in.");
        return -1;
      }

      void destroy(IMemoryManager& mem)
      {
        mem.deallocateArray(keys);
      }
    };

    struct TripleTrack
    {
      Track<float> x = {};
      Track<float> y = {};
      Track<float> z = {};

      void create(
       IMemoryManager& mem,
       std::size_t     num_keys_x,
       std::size_t     num_keys_y,
       std::size_t     num_keys_z)
      {
        x.create(mem, num_keys_x);
        y.create(mem, num_keys_y);
        z.create(mem, num_keys_z);
      }

      void destroy(IMemoryManager& mem)
      {
        x.destroy(mem);
        y.destroy(mem);
        z.destroy(mem);
      }
    };

    struct Channel
    {
      Track<bfQuaternionf> rotation;
      TripleTrack          translation;
      TripleTrack          scale;

      void create(
       IMemoryManager& mem,
       std::size_t     num_rot_keys,
       std::size_t     num_translate_x_keys,
       std::size_t     num_translate_y_keys,
       std::size_t     num_translate_z_keys,
       std::size_t     num_scale_x_keys,
       std::size_t     num_scale_y_keys,
       std::size_t     num_scale_z_keys)
      {
        rotation.create(mem, num_rot_keys);
        translation.create(mem, num_translate_x_keys, num_translate_y_keys, num_translate_z_keys);
        scale.create(mem, num_scale_x_keys, num_scale_y_keys, num_scale_z_keys);
      }

      void destroy(IMemoryManager& mem)
      {
        rotation.destroy(mem);
        translation.destroy(mem);
        scale.destroy(mem);
      }
    };

   public:
    IMemoryManager&                 m_Memory;
    AnimationTimeType               m_Duration;
    AnimationTimeType               m_TicksPerSecond;
    std::uint8_t                    m_NumChannels;
    Channel*                        m_Channels;
    HashTable<String, std::uint8_t> m_NameToChannel;

   public:
    Animation3D(IMemoryManager& memory) :
      m_Memory{memory},
      m_Duration{0.0f},
      m_TicksPerSecond{0.0f},
      m_NumChannels{0u},
      m_Channels{nullptr},
      m_NameToChannel{}
    {
    }

    void create(std::uint8_t num_bones)
    {
      m_NumChannels = num_bones;
      m_Channels    = static_cast<Channel*>(m_Memory.allocate(num_bones * sizeof(Channel)));
    }

    ~Animation3D()
    {
      std::for_each_n(
       m_Channels, m_NumChannels, [this](Channel& channel) {
         channel.destroy(m_Memory);
       });

      m_Memory.deallocate(m_Channels, m_NumChannels * sizeof(Channel));
    }
  };

  class AssetAnimation3DInfo final : public AssetInfo<Animation3D, AssetAnimation3DInfo>
  {
   public:
    using Base::Base;

    bool load(Engine& engine) override;
  };

  using AssetAnimation3DHandle = AssetHandle<Animation3D>;

  struct ModelSkeleton;

  static constexpr std::uint8_t k_InvalidBoneID = static_cast<std::uint8_t>(-1);

  class Model final : public detail::BaseGraphicsResource<Model, bfBufferHandle>
  {
    BF_META_FRIEND;
    friend class AssetModelInfo;

   public:
    struct Mesh
    {
      std::uint32_t       index_offset;
      std::uint32_t       num_indices;
      AssetMaterialHandle material;
    };

    struct Node
    {
      String       name;
      Matrix4x4f   transform;
      std::uint8_t bone_idx;
      unsigned int first_child;
      unsigned int num_children;
    };

    struct NodeIDBone
    {
      unsigned int node_idx;
      Matrix4x4f   transform;
    };

   public:
    Array<AssetMaterialHandle> m_EmbeddedMaterials;
    Array<Mesh>                m_Meshes;
    Array<Node>                m_Nodes;
    Array<NodeIDBone>          m_BoneToModel;
    bfBufferHandle             m_IndexBuffer;
    bfBufferHandle             m_VertexBoneData;
    Matrix4x4f                 m_GlobalInvTransform;

   public:
    explicit Model(IMemoryManager& memory, bfGfxDeviceHandle device);

    void loadAssetSkeleton(const ModelSkeleton& skeleton);

    void draw(bfGfxCommandListHandle cmd_list);

    ~Model()
    {
      bfGfxDevice_flush(m_GraphicsDevice);

      bfGfxDevice_release(m_GraphicsDevice, m_IndexBuffer);
      bfGfxDevice_release(m_GraphicsDevice, m_VertexBoneData);
    }
  };

  class AssetModelInfo final : public AssetInfo<Model, AssetModelInfo>
  {
   public:
    using Base::Base;

    bool load(Engine& engine) override;
  };

  using AssetModelHandle = AssetHandle<Model>;
}  // namespace bf

BIFROST_META_REGISTER(BifrostShaderType){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   enum_info<BifrostShaderType>("BifrostShaderType"),                                                         //
   enum_element("BIFROST_SHADER_TYPE_VERTEX", BIFROST_SHADER_TYPE_VERTEX),                                    //
   enum_element("BIFROST_SHADER_TYPE_TESSELLATION_CONTROL", BIFROST_SHADER_TYPE_TESSELLATION_CONTROL),        //
   enum_element("BIFROST_SHADER_TYPE_TESSELLATION_EVALUATION", BIFROST_SHADER_TYPE_TESSELLATION_EVALUATION),  //
   enum_element("BIFROST_SHADER_TYPE_GEOMETRY", BIFROST_SHADER_TYPE_GEOMETRY),                                //
   enum_element("BIFROST_SHADER_TYPE_FRAGMENT", BIFROST_SHADER_TYPE_FRAGMENT),                                //
   enum_element("BIFROST_SHADER_TYPE_COMPUTE", BIFROST_SHADER_TYPE_COMPUTE),                                  //
   enum_element("BIFROST_SHADER_TYPE_MAX", BIFROST_SHADER_TYPE_MAX)                                           //
   )
   BIFROST_META_END()}

BIFROST_META_REGISTER(bf::Texture){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   class_info<Texture>("Texture"),       //
   ctor<bfGfxDeviceHandle>(),            //
   property("width", &Texture::width),   //
   property("height", &Texture::height)  //
   )
   BIFROST_META_END()}

BIFROST_META_REGISTER(bf::AssetTextureInfo){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   class_info<AssetTextureInfo>("AssetTextureInfo"),  //
   ctor<String, std::size_t, BifrostUUID>()           //
   )
   BIFROST_META_END()}

BIFROST_META_REGISTER(bf::AssetShaderModuleInfo){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   class_info<AssetShaderModuleInfo>("AssetShaderModuleInfo"),  //
   ctor<String, std::size_t, BifrostUUID>(),
   field("m_Type", &AssetShaderModuleInfo::m_Type))
   BIFROST_META_END()}

BIFROST_META_REGISTER(bf::ShaderProgram){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   class_info<ShaderProgram>("ShaderProgram"),                                                                //
   ctor<bfGfxDeviceHandle>(),                                                                                 //
   property("m_NumDescriptorSets", &ShaderProgram::numDescriptorSets, &ShaderProgram::setNumDescriptorSets),  //
   field<BaseAssetHandle>("m_VertexShader", &ShaderProgram::m_VertexShader),                                  //
   field<BaseAssetHandle>("m_FragmentShader", &ShaderProgram::m_FragmentShader)                               //
   )
   BIFROST_META_END()}

BIFROST_META_REGISTER(bf::AssetShaderProgramInfo){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   class_info<AssetShaderModuleInfo>("AssetShaderProgramInfo"),  //
   ctor<String, std::size_t, BifrostUUID>())
   BIFROST_META_END()}

BIFROST_META_REGISTER(bf::Material){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   class_info<Material>("Material"),                                                          //
   ctor<>(),                                                                                  //
   field<BaseAssetHandle>("m_AlbedoTexture", &Material::m_AlbedoTexture),                     //
   field<BaseAssetHandle>("m_NormalTexture", &Material::m_NormalTexture),                     //
   field<BaseAssetHandle>("m_MetallicTexture", &Material::m_MetallicTexture),                 //
   field<BaseAssetHandle>("m_RoughnessTexture", &Material::m_RoughnessTexture),               //
   field<BaseAssetHandle>("m_AmbientOcclusionTexture", &Material::m_AmbientOcclusionTexture)  //
   )
   BIFROST_META_END()}

BIFROST_META_REGISTER(bf::AssetMaterialInfo){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   class_info<AssetMaterialInfo>("AssetMaterialInfo"),  //
   ctor<String, std::size_t, BifrostUUID>())
   BIFROST_META_END()}

BIFROST_META_REGISTER(bf::AssetAnimation3DInfo){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   class_info<AssetAnimation3DInfo>("AssetAnimation3DInfo"),  //
   ctor<String, std::size_t, BifrostUUID>())
   BIFROST_META_END()}

BIFROST_META_REGISTER(bf::Model){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   class_info<Model>("Model")  // ,  //
   // ctor<bfGfxDeviceHandle>(),                              //
   // field_readonly("m_NumVertices", &Model::m_NumVertices)  //
   )
   BIFROST_META_END()}

BIFROST_META_REGISTER(bf::AssetModelInfo)
{
  BIFROST_META_BEGIN()
    BIFROST_META_MEMBERS(
     class_info<AssetModelInfo>("AssetModelInfo"),  //
     ctor<String, std::size_t, BifrostUUID>()       //
    )
  BIFROST_META_END()
}

#endif /* BF_MATERIAL_HPP */
