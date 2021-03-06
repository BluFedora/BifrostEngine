/******************************************************************************/
/*!
 * @file   bf_class_id.hpp
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
#ifndef BF_CLASS_ID_HPP
#define BF_CLASS_ID_HPP

#include "bf/StringRange.hpp"  // StringRange

#include <cstdint>  // uint32_t

namespace bf
{
  class IBaseObject;
  class IMemoryManager;

  using BaseObjectCreateFn = IBaseObject* (*)(IMemoryManager& memory);

  struct BaseObjectTypeInfo
  {
    StringRange        name   = {};
    BaseObjectCreateFn create = nullptr;
  };

  namespace ClassID
  {
    //
    // [0    - 1023]: Core Object Range
    // [1024 - 2047]: Asset Type Range
    // [2048 - 3071]: Component Type Range
    // 
    // When editing this enum rember to change `ClassID::Init` to register the type.
    //
    enum Type
    {
      CORE_OBJECT_RANGE_START = 0u,
      BASE_OBJECT             = CORE_OBJECT_RANGE_START + 0u,
      ENTITY                  = CORE_OBJECT_RANGE_START + 1u,
      BASE_COMPONENT          = CORE_OBJECT_RANGE_START + 2u,
      BASE_BEHAVIOR           = CORE_OBJECT_RANGE_START + 3u,
      CORE_OBJECT_RANGE_END,
      CORE_OBJECT_RANGE_LENGTH = CORE_OBJECT_RANGE_END - CORE_OBJECT_RANGE_START,

      ASSET_RANGE_START = 1024u,
      TEXTURE_ASSET     = ASSET_RANGE_START + 0u,
      MATERIAL_ASSET    = ASSET_RANGE_START + 1u,
      ANIMATION3D_ASSET = ASSET_RANGE_START + 2u,
      SPRITESHEET_ASSET = ASSET_RANGE_START + 3u,
      MODEL_ASSET       = ASSET_RANGE_START + 4u,
      SCENE_ASSET       = ASSET_RANGE_START + 5u,
      ASSET_RANGE_END,
      ASSET_RANGE_RANGE_LENGTH = ASSET_RANGE_END - ASSET_RANGE_START,

      COMPONENT_RANGE_START = 2048u,
      MESH_RENDERER         = COMPONENT_RANGE_START + 0u,
      SKINNED_MESH_RENDERER = COMPONENT_RANGE_START + 1u,
      SPRITE_RENDERER       = COMPONENT_RANGE_START + 2u,
      SPRITE_ANIMATOR       = COMPONENT_RANGE_START + 3u,
      LIGHT                 = COMPONENT_RANGE_START + 4u,
      PARTICLE_SYSTEM       = COMPONENT_RANGE_START + 5u,
      COMPONENT_RANGE_END,
      COMPONENT_RANGE_LENGTH = COMPONENT_RANGE_END - COMPONENT_RANGE_START,
    };

    void               Init();
    bool               IsBaseObject(ClassID::Type type);
    bool               IsAsset(ClassID::Type type);
    bool               IsComponent(ClassID::Type type);
    void               Register(ClassID::Type type, const BaseObjectTypeInfo& info);
    BaseObjectTypeInfo Retreive(ClassID::Type type);
  }  // namespace ClassID
}  // namespace bf

#endif /* BF_CLASS_ID_HPP */

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
