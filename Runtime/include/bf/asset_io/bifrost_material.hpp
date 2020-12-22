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

#include "bf/BaseObject.hpp"         // BaseObject<TSelf>
#include "bf/Quaternion.h"           // bfQuaternionf
#include "bf/bf_gfx_api.h"           //
#include "bifrost_asset_handle.hpp"  // AssetHandle<T>
#include "bifrost_asset_info.hpp"    // AssetInfo<T>
#include "bf/asset_io/bf_base_asset.hpp"
#include "bf/asset_io/bf_gfx_assets.hpp"

namespace bf
{
  using Matrix4x4f        = ::Mat4x4;
  using AnimationTimeType = double;

  class Material final : public BaseObject<Material>
  {
    BF_META_FRIEND;
    friend class AssetModelInfo;

   private:
    ARC<TextureAsset> m_AlbedoTexture;
    ARC<TextureAsset> m_NormalTexture;
    ARC<TextureAsset> m_MetallicTexture;
    ARC<TextureAsset> m_RoughnessTexture;
    ARC<TextureAsset> m_AmbientOcclusionTexture;

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

    const ARC<TextureAsset>& albedoTexture() const { return m_AlbedoTexture; }
    const ARC<TextureAsset>& normalTexture() const { return m_NormalTexture; }
    const ARC<TextureAsset>& metallicTexture() const { return m_MetallicTexture; }
    const ARC<TextureAsset>& roughnessTexture() const { return m_RoughnessTexture; }
    const ARC<TextureAsset>& ambientOcclusionTexture() const { return m_AmbientOcclusionTexture; }
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
      m_Duration{AnimationTimeType(0)},
      m_TicksPerSecond{AnimationTimeType(0)},
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

  class Model final : public BaseObject<Model>
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
    bfGfxDeviceHandle          m_GraphicsDevice;
    bfBufferHandle             m_Handle;
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

      bfGfxDevice_release(m_GraphicsDevice, m_Handle);
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



BIFROST_META_REGISTER(bf::Material){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   class_info<Material>("Material"),                                                          //
   ctor<>(),                                                                                  //
   field<IARCHandle>("m_AlbedoTexture", &Material::m_AlbedoTexture),                          //
   field<IARCHandle>("m_NormalTexture", &Material::m_NormalTexture),                          //
   field<IARCHandle>("m_MetallicTexture", &Material::m_MetallicTexture),                      //
   field<IARCHandle>("m_RoughnessTexture", &Material::m_RoughnessTexture),                    //
   field<IARCHandle>("m_AmbientOcclusionTexture", &Material::m_AmbientOcclusionTexture)  //
   )
   BIFROST_META_END()}

BIFROST_META_REGISTER(bf::AssetMaterialInfo){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   class_info<AssetMaterialInfo>("AssetMaterialInfo"),  //
   ctor<String, std::size_t, bfUUID>())
   BIFROST_META_END()}

BIFROST_META_REGISTER(bf::AssetAnimation3DInfo){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   class_info<AssetAnimation3DInfo>("AssetAnimation3DInfo"),  //
   ctor<String, std::size_t, bfUUID>())
   BIFROST_META_END()}

BIFROST_META_REGISTER(bf::Model){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   class_info<Model>("Model")
   )
   BIFROST_META_END()}

BIFROST_META_REGISTER(bf::AssetModelInfo)
{
  BIFROST_META_BEGIN()
    BIFROST_META_MEMBERS(
     class_info<AssetModelInfo>("AssetModelInfo"),  //
     ctor<String, std::size_t, bfUUID>()            //
    )
  BIFROST_META_END()
}

#endif /* BF_MATERIAL_HPP */
