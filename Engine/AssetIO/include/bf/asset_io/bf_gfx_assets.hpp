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

#include "bf/bf_gfx_api.h"     /* Graphics Handles */
#include "bf_base_asset.hpp"   /* BaseAsset        */
#include "bf_model_loader.hpp" /* Mesh             */

namespace bf
{
  static constexpr std::uint8_t k_InvalidBoneID = static_cast<std::uint8_t>(-1);

  class TextureAsset final : public BaseAsset<TextureAsset>
  {
   public:
    bfGfxDeviceHandle m_ParentDevice;
    bfTextureHandle   m_TextureHandle;

   public:
    explicit TextureAsset(bfGfxDeviceHandle gfx_device = nullptr);

    bfGfxDeviceHandle gfxDevice() const { return m_ParentDevice; }
    bfTextureHandle   handle() const { return m_TextureHandle; }
    std::uint32_t     width() const { return m_TextureHandle ? bfTexture_width(m_TextureHandle) : 0u; }
    std::uint32_t     height() const { return m_TextureHandle ? bfTexture_height(m_TextureHandle) : 0u; }

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

  class MaterialAsset final : public BaseAsset<MaterialAsset>
  {
    BF_META_FRIEND;
    friend class ModelAsset;

   public:
    ARC<TextureAsset> m_AlbedoTexture;
    ARC<TextureAsset> m_NormalTexture;
    ARC<TextureAsset> m_MetallicTexture;
    ARC<TextureAsset> m_RoughnessTexture;
    ARC<TextureAsset> m_AmbientOcclusionTexture;

   public:
    MaterialAsset();

    const ARC<TextureAsset>& albedoTexture() const { return m_AlbedoTexture; }
    const ARC<TextureAsset>& normalTexture() const { return m_NormalTexture; }
    const ARC<TextureAsset>& metallicTexture() const { return m_MetallicTexture; }
    const ARC<TextureAsset>& roughnessTexture() const { return m_RoughnessTexture; }
    const ARC<TextureAsset>& ambientOcclusionTexture() const { return m_AmbientOcclusionTexture; }

   private:
    void onLoad() override;
    void onUnload() override;
  };

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

  template<>
  inline const auto& meta::Meta::registerMembers<MaterialAsset>()
  {
    static auto member_ptrs = members(
     class_info<MaterialAsset>("MaterialAsset"),
     ctor<>(),
     field<IARCHandle>("m_AlbedoTexture", &MaterialAsset::m_AlbedoTexture),
     field<IARCHandle>("m_NormalTexture", &MaterialAsset::m_NormalTexture),
     field<IARCHandle>("m_MetallicTexture", &MaterialAsset::m_MetallicTexture),
     field<IARCHandle>("m_RoughnessTexture", &MaterialAsset::m_RoughnessTexture),
     field<IARCHandle>("m_AmbientOcclusionTexture", &MaterialAsset::m_AmbientOcclusionTexture));

    return member_ptrs;
  }

  using AnimationTimeType = double;

  class Anim3DAsset : public BaseAsset<Anim3DAsset>
  {
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

      // Don`t call this function when only one key exists
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
    explicit Anim3DAsset(IMemoryManager& memory) :
      m_Memory{memory},
      m_Duration{AnimationTimeType(0)},
      m_TicksPerSecond{AnimationTimeType(0)},
      m_NumChannels{0u},
      m_Channels{nullptr},
      m_NameToChannel{}
    {
      markAsEngineAsset();
    }

    void create(std::uint8_t num_bones)
    {
      m_NumChannels = num_bones;
      m_Channels    = static_cast<Channel*>(m_Memory.allocate(num_bones * sizeof(Channel)));
    }

    void destroy()
    {
      std::for_each_n(
       m_Channels, m_NumChannels, [this](Channel& channel) {
         channel.destroy(m_Memory);
       });

      m_Memory.deallocate(m_Channels, m_NumChannels * sizeof(Channel));
    }

   private:
    void onLoad() override;
    void onUnload() override;
  };

  // TODO(SR): This should be declared in a better place.
  using Matrix4x4f = ::Mat4x4;

  struct ModelSkeleton;

  class ModelAsset : public BaseAsset<ModelAsset>
  {
   public:
    struct Node  // NOLINT(cppcoreguidelines-pro-type-member-init)
    {
      String        name;
      Matrix4x4f    transform;
      std::uint8_t  bone_idx;
      std::uint32_t first_child;
      std::uint32_t num_children;
    };

    struct NodeIDBone
    {
      std::uint32_t node_idx;
      Matrix4x4f    transform;
    };

   public:
    bfGfxDeviceHandle     m_GraphicsDevice;
    bfBufferHandle        m_VertexBuffer;
    bfBufferHandle        m_IndexBuffer;
    bfBufferHandle        m_VertexBoneData;
    Array<Mesh>           m_Meshes;
    Array<Node>           m_Nodes;
    Array<NodeIDBone>     m_BoneToModel;
    Array<MaterialAsset*> m_Materials;
    Matrix4x4f            m_GlobalInvTransform;

   public:
    ModelAsset(IMemoryManager& memory, bfGfxDeviceHandle device);

    std::size_t numBones() const { return m_BoneToModel.length(); }
    
   private:
    void onLoad() override;
    void loadSkeleton(const ModelSkeleton& skeleton);
    void onUnload() override;
  };

  BIFROST_META_REGISTER(bf::ModelAsset)
  {
    BIFROST_META_BEGIN()
      BIFROST_META_MEMBERS(
       class_info<ModelAsset>("Model"))
    BIFROST_META_END()
  }
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