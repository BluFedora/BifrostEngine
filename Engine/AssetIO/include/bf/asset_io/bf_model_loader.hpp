#ifndef BF_ASSETIO_MODEL_LOADER_HPP
#define BF_ASSETIO_MODEL_LOADER_HPP

#include "bf/IMemoryManager.hpp"  // IMemoryManager
#include "bf/Math.hpp"            // Math Structs
#include "bf/bf_core.h"           // bfStringRange

#include <array>    // array
#include <cstdint>  // uint32_t
#include <memory>   // uninitialized_fill_n

namespace bf
{
  using AssetIndexType = std::uint32_t;

  ////////////////////

  struct AABB final
  {
    float min[3];
    float max[3];

    AABB() = default;

    AABB(const bfTransform& transform);

    AABB(const Vector3f& vmin, const Vector3f& vmax)
    {
      min[0] = vmin.x;
      min[1] = vmin.y;
      min[2] = vmin.z;
      max[0] = vmax.x;
      max[1] = vmax.y;
      max[2] = vmax.z;
    }

    Vector3f center() const
    {
      return {
       (max[0] + min[0]) * 0.5f,
       (max[1] + min[1]) * 0.5f,
       (max[2] + min[2]) * 0.5f,
       1.0f,
      };
    }

    Vector3f dimensions() const
    {
      return {
       (max[0] - min[0]),
       (max[1] - min[1]),
       (max[2] - min[2]),
       0.0f,
      };
    }

    Vector3f extents() const
    {
      return dimensions() * 0.5f;
    }

    bool canContain(const AABB& rhs) const
    {
      for (int i = 0; i < 3; ++i)
      {
        if (rhs.min[i] < min[i] && !math::isAlmostEqual(min[i], rhs.min[i]) ||
            rhs.max[i] > max[i] && !math::isAlmostEqual(max[i], rhs.max[i]))
        {
          return false;
        }
      }

      return true;
    }

    bool operator==(const AABB& rhs) const
    {
      for (int i = 0; i < 3; ++i)
      {
        if (!math::isAlmostEqual(min[i], rhs.min[i]) || !math::isAlmostEqual(max[i], rhs.max[i]))
        {
          return false;
        }
      }

      return true;
    }

    bool operator!=(const AABB& rhs) const
    {
      return !(*this == rhs);
    }
  };

  namespace aabb
  {
    /*!
     * @brief
     *   Creates a new bounding box that contains both \p a and \p b.
     *
     * @param out
     *   The result of this function.
     *
     * @param a
     *   The first AABB to merge.
     * 
     * @param b
     *   The second AABB to merge.
     */
    inline void mergeBounds(AABB& out, const AABB& a, const AABB& b)
    {
      for (int i = 0; i < 3; ++i)
      {
        out.min[i] = a.min[i] < b.min[i] ? a.min[i] : b.min[i];
        out.max[i] = a.max[i] > b.max[i] ? a.max[i] : b.max[i];
      }
    }

    inline AABB mergeBounds(const AABB& a, const AABB& b)
    {
      AABB out;  // NOLINT
      mergeBounds(out, a, b);
      return out;
    }

    inline void expandBy(AABB& self, float amount)
    {
      for (int i = 0; i < 3; ++i)
      {
        self.min[i] = self.min[i] - amount;
        self.max[i] = self.max[i] + amount;
      }
    }

    inline AABB expandedBy(const AABB& self, float amount)
    {
      AABB clone = self;
      expandBy(clone, amount);
      return clone;
    }

    inline float surfaceArea(const AABB& self)
    {
      const float d[3] =
       {
        self.max[0] - self.min[0],
        self.max[1] - self.min[1],
        self.max[2] - self.min[2],
       };

      return 2.0f * (d[0] * d[1] + d[1] * d[2] + d[2] * d[0]);
    }

    inline AABB fromPoints(const Vector3f* points, std::size_t num_points)
    {
      AABB result;

      result.min[0] = result.max[0] = points[0].x;
      result.min[1] = result.max[1] = points[0].y;
      result.min[2] = result.max[2] = points[0].z;

      for (std::size_t i = 1; i < num_points; ++i)
      {
        const Vector3f& point = points[i];

        result.min[0] = std::min(result.min[0], point.x);
        result.min[1] = std::min(result.min[1], point.y);
        result.min[2] = std::min(result.min[2], point.z);
        result.max[0] = std::max(result.max[0], point.x);
        result.max[1] = std::max(result.max[1], point.y);
        result.max[2] = std::max(result.max[2], point.z);
      }

      return result;
    }

    AABB transform(const AABB& aabb, const Mat4x4& matrix);
  }  // namespace aabb

  ////////////////////

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

  // Lovely Simple DataStructures that lend themselves to a linear allocator.

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
    double                                 duration         = 0.0;  // Duration in ticks.
    double                                 ticks_per_second = 0.0;  // Ticks per second. 0 if not specified in the imported file
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
    IMemoryManager*                  memory              = nullptr;
    detail::Ptr<AssetMeshArray>      mesh_list           = nullptr;
    detail::Ptr<AssetVertexArray>    vertices            = nullptr;
    detail::Ptr<AssetIndexArray>     indices             = nullptr;
    detail::Ptr<AssetMaterialArray>  materials           = nullptr;
    detail::Ptr<AssetAnimationArray> animations          = nullptr;
    ModelSkeleton                    skeleton            = {};
    AABB                             object_space_bounds = {};
    bfStringRange                    error               = {nullptr, nullptr};
    std::array<char, 128>            error_buffer        = {'\0'};  //!< Private Do not use. Read from 'AssetModelLoadResult::error' instead.

    AssetModelLoadResult()                                         = default;
    AssetModelLoadResult(const AssetModelLoadResult& rhs) noexcept = delete;
    AssetModelLoadResult(AssetModelLoadResult&& rhs) noexcept      = default;
    AssetModelLoadResult& operator=(const AssetModelLoadResult& rhs) noexcept = delete;
    AssetModelLoadResult& operator=(AssetModelLoadResult&& rhs) noexcept = delete;

    operator bool() const noexcept
    {
      return error.str_bgn == nullptr;
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