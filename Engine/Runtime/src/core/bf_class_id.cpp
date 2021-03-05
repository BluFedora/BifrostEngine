/******************************************************************************/
/*!
 * @file   bf_class_id.cpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   Needed to allow for creating of objects from a serialized unique id.
 *   All subclasses of IBaseObject should get it's own id added to the
 *   correct section of the enum.
 * 
 *   `ClassID::Init` will need to be edited to account for the new type.
 *
 * @version 0.0.1
 * @date    2021-03-03
 *
 * @copyright Copyright (c) 2021
 */
/******************************************************************************/
#include "bf/asset_io/bf_class_id.hpp"

#include "bf/IMemoryManager.hpp"  // IMemoryManager

#include "bf/BaseObject.hpp"  // IBaseObject
#include "bf/asset_io/bf_gfx_assets.hpp"
#include "bf/asset_io/bifrost_scene.hpp"
#include "bf/ecs/bf_entity.hpp"

#include <array>    // array<T>
#include <cassert>  // assert
#include <utility>  // pair<F, S>

namespace bf::ClassID
{
  using CoreObjectInfoArray = std::array<BaseObjectTypeInfo, std::size_t(CORE_OBJECT_RANGE_LENGTH)>;
  using AssetInfoArray      = std::array<BaseObjectTypeInfo, std::size_t(ASSET_RANGE_RANGE_LENGTH)>;
  using ComponentInfoArray  = std::array<BaseObjectTypeInfo, std::size_t(COMPONENT_RANGE_LENGTH)>;

  namespace
  {
    CoreObjectInfoArray s_CoreObjectInfo = {};
    AssetInfoArray      s_AssetInfoInfo  = {};
    ComponentInfoArray  s_ComponentInfo  = {};

    bool isInRange(ClassID::Type value, ClassID::Type start, ClassID::Type end)
    {
      return std::uint32_t(start) <= std::uint32_t(value) && std::uint32_t(value) < std::uint32_t(end);
    }

#define IS_IN_RANGE(value, name) isInRange(value, name##_RANGE_START, name##_RANGE_END)
#define TO_INDEX(value, name) std::uint32_t(value - name##_RANGE_START)

    BaseObjectTypeInfo& getInfo(ClassID::Type type)
    {
      if (IsBaseObject(type)) { return s_CoreObjectInfo[TO_INDEX(type, CORE_OBJECT)]; }
      if (IsAsset(type)) { return s_AssetInfoInfo[TO_INDEX(type, ASSET)]; }
      if (IsComponent(type)) { return s_ComponentInfo[TO_INDEX(type, COMPONENT)]; }

      assert(!"[ClassID]: Invalid type ID passed in.");
      throw "";
    }
  }  // namespace

  template<typename T>
  static IBaseObject* defaultCreate(IMemoryManager& memory)
  {
    return memory.allocateT<T>();
  }

  template<typename T>
  static IBaseObject* defaultCreateWithAllocatorParam(IMemoryManager& memory)
  {
    return memory.allocateT<T>(memory);
  }

  void Init()
  {
    static const std::pair<Type, BaseObjectTypeInfo> s_Registry[] =
     {
      // Core Object //

      // {BASE_OBJECT, {"BaseObject", &defaultCreate<IBaseObject>}},
      // {ENTITY, {"Entity", &defaultCreate<Entity>}},
      // {BASE_COMPONENT, {"BaseComponent", &defaultCreate<BaseComponent>}},
      // {BASE_BEHAVIOR, {"BaseBehavior", &defaultCreate<BaseBehavior>}},

      // Asset //

      {TEXTURE_ASSET, {"TextureAsset", &defaultCreate<TextureAsset>}},
      {MATERIAL_ASSET, {"MaterialAsset", &defaultCreate<MaterialAsset>}},
      {ANIMATION3D_ASSET, {"Anim3DAsset", &defaultCreateWithAllocatorParam<Anim3DAsset>}},
      {SPRITESHEET_ASSET, {"SpritesheetAsset", &defaultCreate<SpritesheetAsset>}},
      {MODEL_ASSET, {"ModelAsset", &defaultCreateWithAllocatorParam<ModelAsset>}},
      // {SCENE_ASSET, {"SceneAsset", &defaultCreate<SceneAsset>}},

      // Component //

      // {MESH_RENDERER, {"MeshRenderer", &defaultCreate<MeshRenderer>}},
      // {SKINNED_MESH_RENDERER, {"SkinnedMeshRenderer", &defaultCreate<SkinnedMeshRenderer>}},
      // {SPRITE_RENDERER, {"SpriteRenderer", &defaultCreate<SpriteRenderer>}},
      // {SPRITE_ANIMATOR, {"SpriteAnimator", &defaultCreate<SpriteAnimator>}},
      // {LIGHT, {"Light", &defaultCreate<Light>}},
      // {PARTICLE_SYSTEM, {"ParticleSystem", &defaultCreate<ParticleSystem>}},
     };

    for (const auto& reg : s_Registry)
    {
      Register(reg.first, reg.second);
    }
  }

  bool IsBaseObject(ClassID::Type type)
  {
    return IS_IN_RANGE(type, CORE_OBJECT);
  }

  bool IsAsset(ClassID::Type type)
  {
    return IS_IN_RANGE(type, ASSET);
  }

  bool IsComponent(ClassID::Type type)
  {
    return IS_IN_RANGE(type, COMPONENT);
  }

  void Register(ClassID::Type type, const BaseObjectTypeInfo& info)
  {
    getInfo(type) = info;
  }

  BaseObjectTypeInfo Retreive(ClassID::Type type)
  {
    return getInfo(type);
  }
}  // namespace bf::ClassID

#undef TO_INDEX
#undef IS_IN_RANGE

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2021 Shareef Abdoul-Raheem

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
