#ifndef BF_ASSETIO_MODEL_LOADER_HPP
#define BF_ASSETIO_MODEL_LOADER_HPP

#include "bf/IMemoryManager.hpp"  // IMemoryManager
#include "bf/Math.hpp"            // Math Structs
#include "bf/StringRange.h"       // bfStringRange

#include <array>    // array
#include <cstdint>  // uint32_t
#include <memory>   // uninitialized_fill_n

namespace bf
{
  using AssetIndexType = std::uint32_t;

  struct Mesh
  {
    AssetIndexType index_offset;
    AssetIndexType num_indices;
    std::uint32_t  material_idx;
  };


  static constexpr std::size_t k_MaxVertexBones = 4;
  static constexpr std::size_t k_MaxBones       = 128;

  struct AssetModelLoadResult;
  struct AssetModelLoadSettings;

  AssetModelLoadResult loadModel(const AssetModelLoadSettings& load_settings) noexcept;

  // Lovely Simple Datastructures that lend themselves to a linear allocator.

  //
  // Due to this using a 'Variable Length Member' anytime you store an object of
  // this type it should be a pointer as it always needs to be dynamically allocated.
  // T must be default constructable.
  //
  template<typename T>
  struct AssetTempArray
  {
    std::size_t length;   //!< The number of elements in 'AssetTempArray::data'.
    T           data[1];  //!< Fake Variable Length Member, really 'AssetTempArray::length' in size.

    T* begin() noexcept { return data; }
    T* end() noexcept { return data + length; }
  };

  // using AssetTempString = AssetTempArray<char>;

  template<std::size_t kSize>
  struct AssetTempString
  {
    std::size_t length;
    char        data[kSize];

    operator bfStringRange() const noexcept
    {
      return {data, data + length};
    }

    operator bool() const noexcept
    {
      return length != 0;
    }

    void copyOverString(const char* src, std::size_t src_length) noexcept
    {
      src_length = std::min(kSize - 1, src_length);

      std::memcpy(data, src, src_length);

      length           = src_length;
      data[src_length] = '\0';
    }
  };

  using AssetTempLargeString = AssetTempString<1024>;
  using AssetTempSmallString = AssetTempString<256>;

  template<typename T>
  AssetTempArray<T>* allocateTempArray(IMemoryManager& mem, std::size_t num_elements, T default_value = T()) noexcept
  {
    static_assert(std::is_trivially_destructible_v<T>, "Destructors of T will not be called.");

    AssetTempArray<T>* result = new (mem.allocate(offsetof(AssetTempArray<T>, data) + sizeof(T) * num_elements)) AssetTempArray<T>;

    result->length = num_elements;

#if 0
    for (T& item : *result)
    {
      new (&item) T(default_value);
    }
#else
    std::uninitialized_fill_n(result->data, num_elements, default_value);
    // std::uninitialized_fill(result->begin(), result->end(), default_value);
#endif

    return result;
  }

  template<typename T>
  void deallocateTempArray(IMemoryManager& mem, AssetTempArray<T>* temp_array)
  {
    temp_array->~AssetTempArray<T>();
    mem.deallocate(temp_array, offsetof(AssetTempArray<T>, data) + sizeof(T) * temp_array->length);
  }

  // The Meats and Bones

  struct AssetModelLoadSettings
  {
   private:
    bfStringRange   file_path;
    IMemoryManager* memory;
    bool            import_animations;
    bool            import_lights;
    bool            import_cameras;
    bool            smooth_normals;
    bool            row_major;
    float           scale_factor;

   public:
    ///
    /// 'filename' is not required to be nul terminated :)
    ///
    AssetModelLoadSettings(bfStringRange filename, IMemoryManager& mem) noexcept :
      file_path{filename},
      memory{&mem},
      import_animations{true},
      import_lights{false},
      import_cameras{false},
      smooth_normals{true},
      row_major{false},
      scale_factor{1.0f}
    {
    }

    AssetModelLoadSettings& importAnimations(bool value)
    {
      import_animations = value;
      return *this;
    }

    friend AssetModelLoadResult bf::loadModel(const AssetModelLoadSettings&) noexcept;
  };

  namespace PBRTextureType
  {
    enum
    {
      DIFFUSE,
      NORMAL,
      METALLIC,
      ROUGHNESS,
      AO,
      MAX,
    };
  }

  struct AssetModelVertex
  {
    Vector3f     position;
    Vector3f     normal;
    Vector3f     tangent;
    Vector3f     bitangent;
    bfColor4f    color;
    Vector2f     uv;
    float        bone_weights[k_MaxVertexBones];
    std::uint8_t bone_indices[k_MaxVertexBones];
  };

  

  namespace detail
  {
    template<typename T>
    struct PtrDeleter
    {
      IMemoryManager* memory;

      void operator()(T* ptr) const noexcept
      {
        deallocateTempArray(*memory, ptr);
      }
    };

    template<typename T>
    using Ptr = std::unique_ptr<T, PtrDeleter<T>>;

    template<typename T>
    Ptr<T> makeUnique(T* ptr, IMemoryManager* owning_allocator) noexcept
    {
      return Ptr<T>(ptr, PtrDeleter<T>{owning_allocator});
    }

    template<typename T>
    Ptr<AssetTempArray<T>> makeUniqueTempArray(IMemoryManager& mem, std::size_t num_elements, T default_value = T())
    {
      return makeUnique(allocateTempArray(mem, num_elements, std::move(default_value)), &mem);
    }

    //
    // This pointer nulls itself out when moved.
    //
    template<typename T>
    struct MovablePtr
    {
      T* ptr = nullptr;

      MovablePtr() = default;

      MovablePtr(MovablePtr&& rhs) noexcept :
        ptr{std::exchange(rhs.ptr, nullptr)}
      {
      }

      MovablePtr& operator=(MovablePtr&& rhs) noexcept = delete;

      MovablePtr& operator=(T* rhs) noexcept
      {
        // assert(!ptr);
        ptr = rhs;

        return *this;
      }

      T& operator[](int i)
      {
        return ptr[i];
      }

      const T& operator[](int i) const
      {
        return ptr[i];
      }

      operator T*() const
      {
        return ptr;
      }
    };
  }  // namespace detail

  struct AssetPBRMaterial
  {
    AssetTempLargeString textures[PBRTextureType::MAX];
    float                diffuse_color[4];

    bool isOpaque() const { return diffuse_color[3] == 0.0f; }
  };

  struct AnimationKey
  {
    double time;
    float  data[4];
  };

  // all_keys = [pos, rot, scale]
  struct ModelAnimationChannel
  {
    AssetTempSmallString          name                = {};
    AssetTempArray<AnimationKey>* all_keys            = {};
    std::uint32_t                 rotation_key_offset = 0u;
    std::uint32_t                 scale_key_offset    = 0u;
    std::uint32_t                 num_position_keys   = 0u;
    std::uint32_t                 num_rotation_keys   = 0u;
    std::uint32_t                 num_scale_keys      = 0u;
  };

  struct ModelAnimation
  {
    AssetTempSmallString                   name             = {};
    double                                 duration         = 0.0f;  // Duration in ticks.
    double                                 ticks_per_second = 0.0f;  // Ticks per second. 0 if not specified in the imported file
    AssetTempArray<ModelAnimationChannel>* channels         = {};
  };

  using Matrix4x4f = ::Mat4x4;

  struct AssetNode
  {
    AssetTempSmallString name              = {};
    Matrix4x4f           transform         = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    std::uint8_t         model_to_bone_idx = static_cast<std::uint8_t>(-1);
    unsigned int         first_child       = static_cast<unsigned int>(-1);
    unsigned int         num_children      = 0;
  };

  struct ModelSkeleton
  {
    Matrix4x4f                          global_inv_transform;
    unsigned int                        num_nodes;
    detail::MovablePtr<AssetNode>       nodes;
    std::uint8_t                        num_bones;
    std::pair<unsigned int, Matrix4x4f> bones[k_MaxBones];  // <node index, transform>
  };

  using AssetMeshArray      = AssetTempArray<Mesh>;
  using AssetVertexArray    = AssetTempArray<AssetModelVertex>;
  using AssetIndexArray     = AssetTempArray<AssetIndexType>;
  using AssetMaterialArray  = AssetTempArray<AssetPBRMaterial>;
  using AssetAnimationArray = AssetTempArray<ModelAnimation>;

  struct AssetModelLoadResult
  {
    IMemoryManager*                  memory       = nullptr;
    detail::Ptr<AssetMeshArray>      mesh_list    = nullptr;
    detail::Ptr<AssetVertexArray>    vertices     = nullptr;
    detail::Ptr<AssetIndexArray>     indices      = nullptr;
    detail::Ptr<AssetMaterialArray>  materials    = nullptr;
    detail::Ptr<AssetAnimationArray> animations   = nullptr;
    ModelSkeleton                    skeleton     = {};
    bfStringRange                    error        = {nullptr, nullptr};
    std::array<char, 128>            error_buffer = {'\0'};  //!< Private Do not use. Read from 'AssetModelLoadResult::error' instead.

    AssetModelLoadResult()                                         = default;
    AssetModelLoadResult(const AssetModelLoadResult& rhs) noexcept = delete;
    AssetModelLoadResult(AssetModelLoadResult&& rhs) noexcept      = default;
    AssetModelLoadResult& operator=(const AssetModelLoadResult& rhs) noexcept = delete;
    AssetModelLoadResult& operator=(AssetModelLoadResult&& rhs) noexcept = default;

    operator bool() noexcept
    {
      return error.bgn == nullptr;
    }

    // Private API

    void setError(const char* err_message) noexcept;

    ~AssetModelLoadResult()
    {
      if (skeleton.nodes)
      {
        for (ModelAnimation& animation : *animations)
        {
          for (ModelAnimationChannel& channel : *animation.channels)
          {
            deallocateTempArray(*memory, channel.all_keys);
          }

          deallocateTempArray(*memory, animation.channels);
        }

        memory->deallocateArray<AssetNode>(skeleton.nodes);
      }
    }
  };
}  // namespace bf

#endif /* BF_ASSETIO_MODEL_LOADER_HPP */