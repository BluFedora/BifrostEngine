/*!
 * @file   bifrost_assets.cpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Asset / Resource manager for this engine.
 *
 *   References:
 *     [https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file#short-vs-long-names]
 *
 * @version 0.0.1
 * @date    2019-12-28
 *
 * @copyright Copyright (c) 2019-2020
 */
#include "bf/asset_io/bifrost_assets.hpp"

#include "bf/LinearAllocator.hpp"                   // LinearAllocator
#include "bf/asset_io/bifrost_file.hpp"             // File
#include "bf/asset_io/bifrost_json_serializer.hpp"  //
#include "bf/meta/bifrost_meta_runtime.hpp"         //

#include "bf/debug/bifrost_dbg_logger.h"

#include "bf/Platform.h"
#include "bf/asset_io/bf_path_manip.hpp"

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
      char        null_terminated_path[path::k_MaxLength] = {'\0'};
      std::size_t null_terminated_path_length             = std::min(path.length(), bfCArraySize(null_terminated_path) - 3);

      std::strncpy(null_terminated_path, path.bgn, null_terminated_path_length);

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
      DIR* const     file_handle = opendir(null_terminated_path);

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
    m_FileExtReg{},
    m_DirtyAssets{&IBaseAsset::m_DirtyListNode}
  {
  }

  void Assets::registerFileExtensions(std::initializer_list<StringRange> exts, AssetCreationFn create_fn)
  {
    for (const StringRange& ext : exts)
    {
      m_FileExtReg.emplace(ext, create_fn);
    }
  }

  IBaseAsset* Assets::findAsset(const bfUUIDNumber& uuid) const
  {
    return m_AssetSet.find(uuid);
  }

  IBaseAsset* Assets::findAsset(AbsPath abs_path, AssetFindOption load_option)
  {
    const StringRange rel_path = absPathToRelPath(abs_path.path);
    IBaseAsset*       result   = m_AssetSet.find(rel_path);

    if (!result && load_option == AssetFindOption::TRY_LOAD_ASSET)
    {
      result = loadAsset(abs_path.path);
    }

    return result;
  }

  IBaseAsset* Assets::findAsset(RelPath rel_path, AssetFindOption load_option)
  {
    IBaseAsset* result = m_AssetSet.find(rel_path.path);

    if (!result && load_option == AssetFindOption::TRY_LOAD_ASSET)
    {
      const String abs_path = relPathToAbsPath(rel_path.path);

      result = loadAsset(abs_path);
    }

    return result;
  }

  IBaseAsset* Assets::loadAsset(const StringRange& abs_path)
  {
    assert(!path::startWith(abs_path, path::k_SubAssetsRoot));

    LinearAllocator&     alloc     = m_Engine.tempMemory();
    LinearAllocatorScope scope     = {alloc};
    const String         meta_path = absPathToMetaPath(abs_path);
    AssetMetaInfo* const meta_info = loadMetaInfo(alloc, meta_path);
    IBaseAsset*          result;

    if (meta_info)
    {
      result = createAssetFromPath(abs_path, meta_info->uuid);
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

    if (result)
    {
      m_AssetSet.insert(result);
    }

    return result;
  }

  void Assets::markDirty(IBaseAsset* asset)
  {
    // TODO(SR): This is not super thread safe, a CAS would be needed.
    //   THis is because multiple threads can pass the check at the same time
    //   before the first thread is able to set the flag to true.

    if (!(asset->m_Flags & AssetFlags::IS_DIRTY))
    {
      asset->m_Flags |= AssetFlags::IS_DIRTY;
      m_DirtyAssets.pushBack(*asset);
    }
  }

  AssetMetaInfo* Assets::loadMetaInfo(LinearAllocator& temp_allocator, StringRange abs_path_to_meta_file)
  {
    AssetMetaInfo* result = nullptr;

    File meta_file_in{abs_path_to_meta_file, file::FILE_MODE_READ};

    if (meta_file_in)
    {
      const BufferLen      json_data_str = meta_file_in.readEntireFile(temp_allocator);
      json::Value          json_value    = json::fromString(json_data_str.buffer, json_data_str.length);
      JsonSerializerReader reader        = {*this, temp_allocator, json_value};

      if (reader.beginDocument(false))
      {
        result = temp_allocator.allocateT<AssetMetaInfo>();

        result->serialize(temp_allocator, reader);
        reader.endDocument();
      }
    }

    return result;
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
    // All top level assets must be destroyed first so that subassets can be removed from the lists.
    m_AssetSet.forEach([this](IBaseAsset* asset) {
      if (!asset->isSubAsset())
      {
        m_Memory.deallocateT(asset);
      }
    });

    m_AssetSet.forEach([this](IBaseAsset* asset) {
      if (asset->isSubAsset())
      {
        m_Memory.deallocateT(asset);
      }
    });

    m_AssetSet.clear();

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
    LinearAllocator& temp_alloc         = m_Engine.tempMemory();
    IMemoryManager&  temp_alloc_no_free = m_Engine.tempMemoryNoFree();

    for (auto& asset : m_DirtyAssets)
    {
      saveAssetInfo(temp_alloc, temp_alloc_no_free, &asset);
    }

    clearDirtyList();
  }

  void Assets::saveAssetInfo(LinearAllocator& temp_alloc, IMemoryManager& temp_alloc_no_free, IBaseAsset* asset) const
  {
    const LinearAllocatorScope asset_mem_scope = {temp_alloc};
    const String&              full_path       = asset->fullPath();
    const String               meta_file_path  = absPathToMetaPath(asset->fullPath());

    // Save Engine Asset

    if (asset->m_Flags & AssetFlags::IS_ENGINE_ASSET)
    {
      const LinearAllocatorScope json_writer_scope = {temp_alloc};
      JsonSerializerWriter       json_writer       = JsonSerializerWriter{temp_alloc_no_free};

      if (json_writer.beginDocument(false))
      {
        asset->saveAssetContent(json_writer);
        json_writer.endDocument();

        writeJsonToFile(full_path, json_writer.document());
      }
    }

    // Save Meta Info
    {
      const LinearAllocatorScope json_writer_scope = {temp_alloc};
      JsonSerializerWriter       json_writer       = JsonSerializerWriter{temp_alloc_no_free};
      AssetMetaInfo* const       meta_info         = asset->generateMetaInfo(temp_alloc);

      if (meta_info)
      {
        if (json_writer.beginDocument(false))
        {
          meta_info->serialize(temp_alloc, json_writer);
          json_writer.endDocument();

          writeJsonToFile(meta_file_path, json_writer.document());
        }
      }
    }
  }

  void Assets::saveAssetInfo(Engine& engine, IBaseAsset* asset) const
  {
    saveAssetInfo(engine.tempMemory(), engine.tempMemoryNoFree(), asset);
  }

  void Assets::clearDirtyList()
  {
    for (auto& asset : m_DirtyAssets)
    {
      asset.m_Flags &= ~AssetFlags::IS_DIRTY;
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

    return path::append(resolved_path, k_MetaFileExtension);
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

  IBaseAsset* Assets::createAssetFromPath(StringRange path, const bfUUIDNumber& uuid)
  {
    const StringRange file_ext = path::extensionEx(path);
    IBaseAsset*       result   = nullptr;

    if (file_ext.length() > 0)
    {
      const auto ext_handle = m_FileExtReg.find(file_ext);

      if (ext_handle != m_FileExtReg.end())
      {
        result = ext_handle->value()(m_Memory, m_Engine);

        if (result)
        {
          const bool is_sub_asset = path::startWith(path, path::k_SubAssetsRoot);

          result->setup(path, is_sub_asset ? 0 : String_length(m_RootPath), uuid, *this);
        }
      }
      else
      {
        bfLogWarn("[Assets::loadAsset] Failed to find extension handler for \"%.*s\".", int(path.length()), path.begin());
      }
    }

    return result;
  }

  IBaseAsset* Assets::createAssetFromPath(StringRange path)
  {
    const bfUUID uuid = bfUUID_generate();

    return createAssetFromPath(path, uuid.as_number);
  }
}  // namespace bf
