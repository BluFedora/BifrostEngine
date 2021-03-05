/******************************************************************************/
/*!
 * @file   bf_document.cpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   Abstraction over a file with a set of resources to allow for easily
 *   refering to multiple assets that may be stored in a single file.
 *
 * @version 0.0.1
 * @date    2021-03-02
 *
 * @copyright Copyright (c) 2021
 */
/******************************************************************************/
#include "bf/asset_io/bf_document.hpp"

#include "bf/asset_io/bf_base_asset.hpp"            // IBaseAsset
#include "bf/asset_io/bf_gfx_assets.hpp"            // ModelAsset
#include "bf/asset_io/bf_iserializer.hpp"           // ISerializer
#include "bf/asset_io/bf_path_manip.hpp"            // path::*
#include "bf/asset_io/bifrost_assets.hpp"           // Assets*
#include "bf/asset_io/bifrost_json_serializer.hpp"  // JsonSerializerWriter

namespace bf
{
  // TODO(SR): We dont want these. \/
  LinearAllocator& ENGINE_TEMP_MEM(Engine& engine);
  // TODO(SR): We dont want these. /\


  IDocument::IDocument() :
    m_Version{k_DocumentFileFormatVersion},
    m_UUID{bfUUID_makeEmpty().as_number},
    m_FilePathAbs{},
    m_FilePathRel{},
    m_AssetList{&IBaseAsset::m_DocResourceListNode},
    m_AssetListCount{0u},
    m_AssetManager{nullptr},
    m_DirtyListNode{},
    m_Flags{ATOMIC_VAR_INIT(AssetFlags::DEFAULT)},
    m_RefCount{ATOMIC_VAR_INIT(0u)}
  {
  }

  IDocument::~IDocument()
  {
    auto& memory = m_AssetManager->memory();

    // Must be done this way since this is an intrusive linked list.
    while (!m_AssetList.isEmpty())
    {
      IBaseAsset& asset = m_AssetList.back();
      m_AssetList.popBack();
      memory.deallocateT(&asset);
    }
  }

  StringRange IDocument::name() const { return path::nameWithoutExtension(relativePath()); }
  StringRange IDocument::nameWithExt() const { return path::name(relativePath()); }

  AssetStatus IDocument::status() const
  {
    const std::uint16_t current_flags = m_Flags;

    if (refCount() == 0)
    {
      assert((m_Flags & AssetFlags::IS_LOADED) == 0 && "This flag should not be set if the ref count is 0.");

      if (current_flags & AssetFlags::FAILED_TO_LOAD)
      {
        return AssetStatus::FAILED;
      }

      return AssetStatus::UNLOADED;
    }

    // The Ref Count is non-zero.

    if (current_flags & AssetFlags::IS_LOADED)
    {
      return AssetStatus::LOADED;
    }

    return AssetStatus::LOADING;
  }

  void IDocument::acquire()
  {
    assert((m_RefCount + 1) != 0 && "Unsigned overflow check, I suspect that we will never have more than 0xFFFF live references but just to be safe.");

    const std::uint16_t old_ref_count = m_RefCount++;
    const std::uint16_t flags         = m_Flags;

    // Do not continuously try to load an asset that could not be loaded.
    if (!(flags & AssetFlags::FAILED_TO_LOAD))
    {
      if (old_ref_count == 0)  // The ref count WAS zero.
      {
        assert((flags & AssetFlags::IS_LOADED) == 0 && "If the ref count was zero then this asset should not have been loaded.");

        switch (onLoad())
        {
          case AssetStatus::FAILED:
          {
            m_Flags |= AssetFlags::FAILED_TO_LOAD;
            break;
          }
          case AssetStatus::LOADED:
          {
            m_Flags |= AssetFlags::IS_LOADED;
            break;
          }
          default:
            break;
        }
      }
    }
  }

  void IDocument::release()
  {
    assert(m_RefCount > 0 && "To many release calls for the number of acquire calls.");

    if (--m_RefCount == 0)  // This is the last use of this asset.
    {
      std::atomic_uint16_t& flags = m_Flags;

      assert((flags & (AssetFlags::IS_LOADED | AssetFlags::FAILED_TO_LOAD)) != 0 && "The asset should have been loaded (or atleast attempted) if we are now unloading it.");

      onUnload();
      flags &= ~(AssetFlags::IS_LOADED | AssetFlags::FAILED_TO_LOAD);

      if (flags & AssetFlags::DESTROY_ON_RELEASE)
      {
        // TODO(SR):
        __debugbreak();
        // destroy(m_References.memory());
      }
    }
  }

  void IDocument::reload()
  {
    onReload();
  }

  void IDocument::save()
  {
    onSaveAsset();
  }

  void IDocument::serializeMetaInfo(ISerializer& serializer)
  {
    if (serializer.pushObject("Header"))
    {
      serializer.serialize("m_Version", m_Version);
      serializer.serialize("m_UUID", m_UUID);

      serializer.popObject();
    }

    std::size_t num_asset_infos = m_AssetListCount;
    if (serializer.pushArray("AssetInfos", num_asset_infos))
    {
      if (serializer.mode() == SerializerMode::LOADING)
      {
        for (std::size_t i = 0; i < num_asset_infos; ++i)
        {
          if (serializer.pushObject(nullptr))
          {
            std::uint32_t class_id;
            serializer.serialize("m_ClassID", class_id);

            if (IsAsset(ClassID::Type(class_id)))
            {
              auto type_info = ClassID::Retreive(ClassID::Type(class_id));

              if (type_info.create)
              {
                IBaseAsset* const asset = static_cast<IBaseAsset*>(type_info.create(assetMemory()));

                if (asset)
                {
                  String     name;
                  ResourceID id;

                  serializer.serialize("m_Name", name);
                  serializer.serialize("m_FileID", id.id);

                  addAssetImpl(asset, id, name);
                }
              }
              else
              {
                std::printf("[IDocument::serializeMetaInfo] No create registered for for '%s'.\n", type_info.name.begin());
              }
            }

            serializer.popObject();
          }
        }
      }
      else
      {
        for (IBaseAsset& asset : m_AssetList)
        {
          if (serializer.pushObject(nullptr))
          {
            std::uint32_t class_id = asset.classID();

            serializer.serialize("m_ClassID", class_id);
            serializer.serialize("m_Name", asset.m_Name);
            serializer.serialize("m_FileID", asset.m_FileID.id);

            serializer.popObject();
          }
        }
      }

      serializer.popArray();
    }

    if (serializer.pushObject("Document"))
    {
      onSaveMeta(serializer);
      serializer.popObject();
    }
  }

  IBaseAsset* IDocument::findResource(ResourceID file_id)
  {
    if (file_id.id != 0u)
    {
      for (IBaseAsset& asset : m_AssetList)
      {
        if (asset.m_FileID == file_id)
        {
          return &asset;
        }
      }
    }

    return nullptr;
  }

  void IDocument::onSaveMeta(ISerializer& serializer) {} /* NO-OP */
  void IDocument::onSaveAsset() {}                       /* NO-OP */

  void IDocument::onReload()
  {
    // We only need to reload if this document is live otherwise just wait til we are referenced again.
    if (refCount() != 0u)
    {
      // We only need to unload if we were already loaded.
      if (status() == AssetStatus::LOADED)
      {
        onUnload();
      }

      onLoad();
    }
  }

  Assets& IDocument::assets() const
  {
    return *m_AssetManager;
  }

  IMemoryManager& bf::IDocument::assetMemory() const
  {
    return m_AssetManager->memory();
  }

  void IDocument::addAssetImpl(IBaseAsset* asset, ResourceID id, StringRange name)
  {
    asset->m_Name     = name;
    asset->m_FileID   = id;
    asset->m_Document = this;

    m_AssetList.pushBack(*asset);
    ++m_AssetListCount;
  }

  FileSaveCtx IDocument::defaultSaveBegin()
  {
    LinearAllocator& temp_alloc = ENGINE_TEMP_MEM(assets().engine());
    FileSaveCtx      ctx        = {};

    ctx.memory_scope.save(temp_alloc);
    ctx.serializer         = temp_alloc.allocateT<JsonSerializerWriter>(temp_alloc);
    ctx.has_document_began = ctx.serializer->beginDocument();

    return ctx;
  }

  void IDocument::defaultSaveEnd(FileSaveCtx& ctx)
  {
    JsonSerializerWriter* const json_writer = static_cast<JsonSerializerWriter*>(ctx.serializer);

    if (ctx.has_document_began)
    {
      const String& full_path = fullPath();

      ctx.serializer->endDocument();
      assets().writeJsonToFile(full_path, json_writer->document());
    }

    json_writer->~JsonSerializerWriter();
    ctx.memory_scope.restore();
  }
}  // namespace bf

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
