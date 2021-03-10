/******************************************************************************/
/*!
 * @file   bifrost_assets.cpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   Asset / Resource manager for this engine.
 *
 *   References:
 *     [https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file#short-vs-long-names]
 *
 * @version 0.0.1
 * @date    2019-12-28
 *
 * @copyright Copyright (c) 2019-2021
 */
/******************************************************************************/
#include "bf/asset_io/bifrost_assets.hpp"

#include "bf/LinearAllocator.hpp"  // LinearAllocator
#include "bf/Platform.h"
#include "bf/asset_io/bf_path_manip.hpp"
#include "bf/asset_io/bifrost_file.hpp"             // File
#include "bf/asset_io/bifrost_json_serializer.hpp"  //
#include "bf/bf_dbg_logger.h"

#include <filesystem> /* filesystem::* */

#if BIFROST_PLATFORM_WINDOWS
#define NOMINMAX
#include <Windows.h>  // HANDLE
#else
#include <dirent.h>
#endif

#include "bf/core/bifrost_engine.hpp"  // THIS HAS A GLFW HEADER WHICH FUCKS WITH WINDOWS.h

namespace bf
{
  void IBaseObject::reflect(ISerializer& serializer)
  {
    serializer.serialize(*this);
  }

  LinearAllocator&  ENGINE_TEMP_MEM(Engine& engine) { return engine.tempMemory(); }
  bfGfxDeviceHandle ENGINE_GFX_DEVICE(Engine& engine) { return engine.renderer().device(); }

  namespace files = std::filesystem;

  namespace path
  {
    bool doesExist(const char* path)
    {
      return files::exists(path);
    }

    bool createDirectory(const char* path)
    {
      return files::create_directory(path);
    }

    bool renameDirectory(const char* full_path, const char* new_name)
    {
      const StringRange base_path = path::directory(full_path);
      files::path       old_path  = {base_path.begin(), base_path.end()};
      files::path       new_path  = old_path;
      std::error_code   err;

      old_path /= base_path.end() + 1;
      new_path /= new_name;

      files::rename(old_path, new_path, err);
      return !err;
    }

    bool moveDirectory(const char* dst_path, const char* src_path)
    {
      const StringRange src_base_path  = path::directory(src_path);
      const char*       src_name_start = src_base_path.end() + 1;
      const files::path old_path       = src_path;
      const files::path new_path       = files::path(dst_path) / src_name_start;
      std::error_code   err;

      files::rename(old_path, new_path, err);
      return !err;
    }

    bool deleteDirectory(const char* path)
    {
      std::error_code err;
      return files::remove_all(path, err);
    }

    struct DirectoryEntry
    {
      IMemoryManager* memory;
#if BIFROST_PLATFORM_WINDOWS
      HANDLE           file_handle;
      WIN32_FIND_DATAA out_data;
#else
      DIR*           file_handle;
      struct dirent* out_data;
#endif
    };

    DirectoryEntry* openDirectory(IMemoryManager& memory, const StringRange& path)
    {
      char        null_terminated_path[k_MaxLength] = {'\0'};
      std::size_t null_terminated_path_length       = std::min(path.length(), bfCArraySize(null_terminated_path) - 3);

      std::strncpy(null_terminated_path, path.str_bgn, null_terminated_path_length);

#if BIFROST_PLATFORM_WINDOWS
      null_terminated_path[null_terminated_path_length++] = '\\';
      null_terminated_path[null_terminated_path_length++] = '*';
#endif

      null_terminated_path[null_terminated_path_length] = '\0';

#if BIFROST_PLATFORM_WINDOWS
      WIN32_FIND_DATA out_data;
      const HANDLE    file_handle = ::FindFirstFileA(null_terminated_path, &out_data);

      if (file_handle == INVALID_HANDLE_VALUE)
      {
        return nullptr;
      }
#else
      DIR* const file_handle = opendir(null_terminated_path);

      if (!file_handle)
      {
        return nullptr;
      }

      struct dirent* const out_data = readdir(file_handle);

      // Empty Directory.
      if (!out_data)
      {
        closedir(file_handle);
        return nullptr;
      }
#endif

      DirectoryEntry* const entry = memory.allocateT<DirectoryEntry>();

      entry->memory      = &memory;
      entry->file_handle = file_handle;
      entry->out_data    = out_data;

      return entry;
    }

    bool isDirectory(const DirectoryEntry* entry)
    {
#if BIFROST_PLATFORM_WINDOWS
      return entry->out_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
#else
      return entry->out_data->d_type == DT_DIR;
#endif
    }

    bool isFile(const DirectoryEntry* entry)
    {
      return !isDirectory(entry);
    }

    const char* entryFilename(const DirectoryEntry* entry)
    {
#if BIFROST_PLATFORM_WINDOWS
      return entry->out_data.cFileName;
#else
      return entry->out_data->d_name;
#endif
    }

    bool readNextEntry(DirectoryEntry* entry)
    {
#if BIFROST_PLATFORM_WINDOWS
      return ::FindNextFileA(entry->file_handle, &entry->out_data) != 0;
#else
      entry->out_data = readdir(entry->file_handle);
      return entry->out_data != nullptr;
#endif
    }

    void closeDirectory(DirectoryEntry* entry)
    {
#if BIFROST_PLATFORM_WINDOWS
      ::FindClose(entry->file_handle);
#else
      closedir(entry->file_handle);
#endif

      entry->memory->deallocateT(entry);
    }

    bool renameFile(const StringRange& old_name, const StringRange& new_name)
    {
      const files::path old_path = {old_name.begin(), old_name.end()};
      const files::path new_path = {new_name.begin(), new_name.end()};
      std::error_code   err;

      files::rename(old_path, new_path, err);

      return !err;
    }
  }  // namespace path

  Assets::Assets(Engine& engine, IMemoryManager& memory) :
    m_Engine{engine},
    m_Memory{memory},
    m_RootPath{nullptr},
    m_AssetSet{memory},
    m_Importers{},
    m_DirtyAssets{&IDocument::m_DirtyListNode},
    m_DirtyAssetsMutex{}
  {
  }

  void Assets::registerFileExtensions(std::initializer_list<StringRange> exts, AssetImporterFn create_fn, void* user_data)
  {
    for (const StringRange& ext : exts)
    {
      m_Importers.emplace(ext, create_fn, user_data);
    }
  }

  IDocument* Assets::findDocument(const bfUUIDNumber& uuid)
  {
    return m_AssetSet.find(uuid);
  }

  IDocument* Assets::findDocument(AbsPath abs_path, AssetFindOption load_option)
  {
    const StringRange rel_path = absPathToRelPath(abs_path.path);
    IDocument*        result   = m_AssetSet.find(rel_path);

    if (!result && load_option == AssetFindOption::TRY_LOAD_ASSET)
    {
      result = loadDocument(abs_path.path);
    }

    return result;
  }

  IDocument* Assets::findDocument(RelPath rel_path, AssetFindOption load_option)
  {
    IDocument* result = m_AssetSet.find(rel_path.path);

    if (!result && load_option == AssetFindOption::TRY_LOAD_ASSET)
    {
      const String abs_path = relPathToAbsPath(rel_path.path);

      result = loadDocument(abs_path);
    }

    return result;
  }

  IDocument* Assets::loadDocument(const StringRange& abs_path)
  {
    assert(!path::startWith(abs_path, path::k_SubAssetsRoot));

    LinearAllocator&     temp_allocator = m_Engine.tempMemory();
    LinearAllocatorScope scope          = {temp_allocator};
    const String         meta_path      = absPathToMetaPath(abs_path);
    AssetImporter        importer       = findAssetImporter(abs_path);

    if (importer.callback)
    {
      AssetImportCtx import_ctx;
      import_ctx.document           = nullptr;
      import_ctx.asset_full_path    = abs_path;
      import_ctx.meta_full_path     = meta_path;
      import_ctx.importer_user_data = importer.user_data;
      import_ctx.asset_memory       = &m_Memory;
      import_ctx.engine             = &m_Engine;

      importer.callback(import_ctx);

      IDocument* const doc = import_ctx.document;

      if (doc)
      {
        doc->m_AssetManager = this;
        doc->setPath(abs_path, String_length(m_RootPath));

        bool needs_reimport = false;

        File meta_file_in{meta_path, file::FILE_MODE_READ};

        if (meta_file_in)
        {
          const BufferLen      json_data_str = meta_file_in.readEntireFile(temp_allocator);
          json::Value          json_value    = json::fromString(json_data_str.buffer, json_data_str.length);
          JsonSerializerReader reader        = {*this, temp_allocator, json_value};

          if (reader.beginDocument())
          {
            doc->serializeMetaInfo(reader);
            reader.endDocument();
          }
        }
        else  // If we could not find meta file then mark asset as dirty to be saved later.
        {
          needs_reimport = true;
        }

        if (bfUUID_numberIsEmpty(&doc->m_UUID))
        {
          doc->m_UUID    = bfUUID_generate().as_number;
          needs_reimport = true;
        }

        m_AssetSet.insert(doc);

        if (needs_reimport)
        {
          markDirty(doc);
        }
      }
      else
      {
        bfLogWarn("[Assets::loadDocument] Importer failed to create document for \"%.*s\".", int(abs_path.length()), abs_path.begin());
      }

      return doc;
    }
    else
    {
      bfLogWarn("[Assets::loadDocument] Failed to find extension handler for \"%.*s\".", int(abs_path.length()), abs_path.begin());
    }

    return nullptr;
  }

  void Assets::markDirty(IBaseAsset* asset)
  {
    markDirty(&asset->document());
  }

  void Assets::markDirty(IDocument* asset)
  {
    std::uint16_t flags = asset->m_Flags;

    if (!(flags & AssetFlags::IS_DIRTY))
    {
      if (std::atomic_compare_exchange_strong(&asset->m_Flags, &flags, flags | AssetFlags::IS_DIRTY))
      {
        asset->acquire();

        std::lock_guard<std::mutex> lock{m_DirtyAssetsMutex};

        m_DirtyAssets.pushBack(*asset);
      }
    }
  }

  AssetError Assets::setRootPath(std::string_view path)
  {
    using fs_string_type = files::path::string_type;

    files::path     fs_path = path;
    std::error_code err;

    if (!exists(fs_path, err))
    {
      return AssetError::PATH_DOES_NOT_EXIST;
    }

    if (err)
    {
      return AssetError::UNKNOWN_STL_ERROR;
    }

    fs_path = canonical(fs_path, err);

    if (err)
    {
      return AssetError::UNKNOWN_STL_ERROR;
    }

    setRootPath(nullptr);

    if (!m_RootPath)
    {
      m_RootPath = String_newLen(nullptr, 0);
    }

    const fs_string_type& fs_path_str = fs_path.native();

    String_resize(&m_RootPath, fs_path_str.length());

    int path_length = 0;

    for (const auto path_char : fs_path_str)
    {
      m_RootPath[path_length++] = static_cast<char>(path_char);
    }

    const std::size_t new_root_path_length = file::canonicalizePath(m_RootPath);
    String_resize(&m_RootPath, new_root_path_length);

    return AssetError::NONE;
  }

  void Assets::setRootPath(std::nullptr_t)
  {
    static constexpr std::int32_t k_MaxIterations = 20;

    // Always just save out the meta files.
    {
      LinearAllocator& temp_alloc = m_Engine.tempMemory();

      m_AssetSet.forEach([this, &temp_alloc](IDocument* document) {
        LinearAllocatorScope mem_scope = temp_alloc;

        saveDocumentMetaInfo(temp_alloc, document);
      });
    }

    //
    // This loops through each assets and frees any with a ref count of zero.
    //
    // The reason for this design is so that assets gets freed in order
    // since assets can reference other assets.
    //
    // Loop Exit Conditions:
    //  - (SUCCESS)            All assets have been freed.                        OR
    //  - (SUCCESS OR FAILURE) No progress towards freeing assets have been made. OR
    //  - (FAILURE)            The max iteration count has been hit.              OR
    //

    std::int32_t iteration_count = 0;

    bool has_made_progress;
    do
    {
      has_made_progress = m_AssetSet.removeIf(
       [](IDocument* document) { return document->refCount() == 0; },
       [this](IDocument* document) { m_Memory.deallocateT(document); });

      ++iteration_count;

    } while (!m_AssetSet.isEmpty() && iteration_count < k_MaxIterations && has_made_progress);

    // Log error and just leak the asset memory.
    if (!m_AssetSet.isEmpty())
    {
      bfLogPush("Count not free all assets in %i iterations.", iteration_count);

      m_AssetSet.forEach([this](IDocument* asset) {
        const StringRange name      = asset->fullPath();
        const int         ref_count = asset->refCount();

        bfLogWarn("[%.*s]: RefCount(%i).", int(name.length()), name.begin(), ref_count);
      });
      m_AssetSet.clear();

      bfLogPop();
    }

    if (m_RootPath)
    {
      String_clear(&m_RootPath);
    }
  }

  bool Assets::writeJsonToFile(const StringRange& path, const json::Value& value) const
  {
    File file_out{path, file::FILE_MODE_WRITE};

    if (file_out)
    {
      String json_string;
      toString(value, json_string);
      file_out.write(json_string);
      file_out.close();

      return true;
    }

    return false;
  }

  void Assets::saveAssets()
  {
    LinearAllocator&            temp_alloc = m_Engine.tempMemory();
    std::lock_guard<std::mutex> lock{m_DirtyAssetsMutex};

    for (auto& document : m_DirtyAssets)
    {
      document.save();
      saveDocumentMetaInfo(temp_alloc, &document);
    }

    clearDirtyList();
  }

  void Assets::clearDirtyList()
  {
    for (auto& asset : m_DirtyAssets)
    {
      asset.m_Flags &= ~AssetFlags::IS_DIRTY;
      asset.release();
    }

    m_DirtyAssets.clear();
  }

  String Assets::relPathToAbsPath(const StringRange& rel_path) const
  {
    return path::append({m_RootPath, String_length(m_RootPath)}, rel_path);
  }

  StringRange Assets::absPathToRelPath(const StringRange& abs_path) const
  {
    return path::relative(m_RootPath, abs_path);
  }

  String Assets::absPathToMetaPath(const StringRange& abs_path) const
  {
    const String resolved_path = resolvePath(abs_path);

    return resolved_path + k_MetaFileExtension;
  }

  String Assets::resolvePath(const StringRange& abs_or_asset_path) const
  {
    String result;

    if (path::startWith(abs_or_asset_path, path::k_AssetsRoot))
    {
      result = path::append({m_RootPath, String_length(m_RootPath)},
                            {abs_or_asset_path.begin() + path::k_AssetsRoot.length(), abs_or_asset_path.end()});
    }
    else
    {
      result = abs_or_asset_path;
    }

    return result;
  }

  Assets::~Assets()
  {
    if (m_RootPath)
    {
      String_delete(m_RootPath);
    }
  }

  IBaseAsset* Assets::loadAsset(const StringRange& abs_path)
  {
#if 0
    assert(!path::startWith(abs_path, path::k_SubAssetsRoot));

    LinearAllocator&     alloc     = m_Engine.tempMemory();
    LinearAllocatorScope scope     = {alloc};
    const String         meta_path = absPathToMetaPath(abs_path);
    AssetMetaInfo* const meta_info = loadMetaInfo(alloc, meta_path);
    IBaseAsset*          result;

    if (meta_info)
    {
      result = createAssetFromPath(abs_path, meta_info->uuid);

      if (result)
      {
        // NOTE(SR): This loop assumes that only one level nesting of sub assets will happen.

        AssetMetaInfo* child_iterator = meta_info->first_child;

        while (child_iterator)
        {
          result->findOrCreateSubAsset(child_iterator->name.toStringRange(), child_iterator->uuid);

          child_iterator = child_iterator->next_sibling;
        }
      }
    }
    else
    {
      // If we could not find meta file then mark asset as dirty to be saved later.

      result = createAssetFromPath(abs_path);

      if (result)
      {
        markDirty(result);
      }
    }

    return result;
#endif

    __debugbreak();
    return nullptr;
  }

  AssetImporter Assets::findAssetImporter(StringRange path) const
  {
    const StringRange file_ext = path::extensionEx(path);

    if (file_ext.length() > 0)
    {
      const auto ext_handle = m_Importers.find(file_ext);

      if (ext_handle != m_Importers.end())
      {
        return ext_handle->value();
      }
    }

    return {nullptr, nullptr};
  }

  void Assets::saveDocumentMetaInfo(LinearAllocator& temp_alloc, IDocument* document)
  {
    // This is so we do not save meta files for assets that failed to load.

    const String& full_path = document->fullPath();

    if (File::exists(full_path.cstr()))
    {
      const LinearAllocatorScope json_writer_scope = {temp_alloc};
      JsonSerializerWriter       json_writer       = JsonSerializerWriter{temp_alloc};

      if (json_writer.beginDocument())
      {
        document->serializeMetaInfo(json_writer);
        json_writer.endDocument();

        const String meta_file_path = absPathToMetaPath(full_path);

        writeJsonToFile(meta_file_path, json_writer.document());
      }
    }
  }

}  // namespace bf
