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
 * @copyright Copyright (c) 2019
 */
#include "bifrost/asset_io/bifrost_assets.hpp"

#include "bifrost/asset_io/bifrost_asset_handle.hpp"     //
#include "bifrost/asset_io/bifrost_file.hpp"             // File
#include "bifrost/asset_io/bifrost_json_serializer.hpp"  //
#include "bifrost/asset_io/bifrost_json_value.hpp"       // JsonValue

#include "bifrost/memory/bifrost_linear_allocator.hpp" /* LinearAllocator */
#include "bifrost/meta/bifrost_meta_runtime.hpp"       //
#include "bifrost/platform/bifrost_platform.h"         //

#include <filesystem> /* filesystem::* */

#if BIFROST_PLATFORM_WINDOWS
#define NOMINMAX
#include <Windows.h>  // HANDLE
#else
#include <dirent.h>
#endif

#include "bifrost/core/bifrost_engine.hpp"  // THIS HAS AN GLFW HEADER WHICH FUCKS WITH WINDOWS.h

namespace bifrost
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
      const StringRange base_path = file::directoryOfFile(full_path);
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
      const StringRange src_base_path  = file::directoryOfFile(src_path);
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
      char        null_terminated_path[MAX_PATH] = {'\0'};
      std::size_t null_terminated_path_length    = std::min(path.length(), bfCArraySize(null_terminated_path));

      std::strncpy(null_terminated_path, path.bgn, null_terminated_path_length);
      null_terminated_path[null_terminated_path_length++] = '\\';
      null_terminated_path[null_terminated_path_length++] = '*';
      null_terminated_path[null_terminated_path_length]   = '\0';

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

      struct dirent* const = readdir(file_handle);

#endif

      DirectoryEntry* const entry = memory.allocateT<DirectoryEntry>();

      entry->memory      = &memory;
      entry->file_handle = file_handle;
      entry->out_data    = out_data;

      return entry;
    }

    bool isDirectory(const DirectoryEntry* entry)
    {
      return entry->out_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    }

    bool isFile(const DirectoryEntry* entry)
    {
      return !isDirectory(entry);
    }

    const char* entryFilename(const DirectoryEntry* entry)
    {
      return entry->out_data.cFileName;
    }

    bool readNextEntry(DirectoryEntry* entry)
    {
      return ::FindNextFileA(entry->file_handle, &entry->out_data) != 0;
    }

    void closeDirectory(DirectoryEntry* entry)
    {
      ::FindClose(entry->file_handle);
      entry->memory->deallocateT(entry);
    }
  }  // namespace path

  bool Assets::isHandleCompatible(const BaseAssetHandle& handle, const BaseAssetInfo* info)
  {
    meta::BaseClassMetaInfo* const handle_type       = handle.typeInfo();
    meta::BaseClassMetaInfo* const info_payload_type = info->payloadType();

    return handle_type == info_payload_type;
  }

  Assets::Assets(Engine& engine, IMemoryManager& memory) :
    m_Engine{engine},
    m_Memory{memory},
    m_NameToGUID{},
    m_AssetMap{},
    m_RootPath{nullptr},
    m_MetaPath{},
    m_DirtyAssetList{memory}
  {
  }

  BifrostUUID Assets::indexAssetImpl(const StringRange relative_path, bool& create_new, meta::BaseClassMetaInfo* type_info)
  {
    const auto it_name = m_NameToGUID.find(relative_path);

    if (it_name != m_NameToGUID.end())
    {
      const auto it_asset = m_AssetMap.find(it_name->value());

      if (it_asset != m_AssetMap.end())
      {
        // Do Updates?
        create_new = false;
        return it_name->value();
      }
    }

    const BifrostUUID uuid = bfUUID_generate();

    m_NameToGUID.emplace(relative_path, uuid);

    create_new = true;
    return uuid;
  }

  BaseAssetInfo* Assets::findAssetInfo(const BifrostUUID& uuid)
  {
    const auto it = m_AssetMap.find(uuid);

    if (it != m_AssetMap.end())
    {
      return it->value();
    }

    return nullptr;
  }

  bool Assets::tryAssignHandle(BaseAssetHandle& handle, BaseAssetInfo* info) const
  {
    if (isHandleCompatible(handle, info))
    {
      handle = BaseAssetHandle(m_Engine, info, handle.m_TypeInfo);
      return true;
    }

    return false;
  }

  BaseAssetHandle Assets::makeHandle(BaseAssetInfo& info) const
  {
    return BaseAssetHandle(m_Engine, &info, info.payloadType());
  }

  String Assets::fullPath(const BaseAssetInfo& info) const
  {
    String full_path = m_RootPath;

    full_path.reserve(full_path.size() + 1 + info.path().size());

    full_path.append('/');
    full_path.append(info.path());

    return full_path;
  }

  char* Assets::metaFileName(IMemoryManager& allocator, StringRange relative_path, std::size_t& out_string_length) const
  {
    char* const buffer     = string_utils::fmtAlloc(allocator, &out_string_length, "%.*s%s", int(relative_path.length()), relative_path.begin(), META_FILE_EXTENSION);
    char* const buffer_end = buffer + out_string_length;

    std::transform(buffer, buffer_end, buffer, [](const char character) {
      return character == '/' ? '.' : character;
    });

    return buffer;
  }

  TempBuffer Assets::metaFullPath(IMemoryManager& allocator, StringRange meta_file_name) const
  {
    std::size_t buffer_size;
    char*       buffer = string_utils::fmtAlloc(allocator, &buffer_size, "%s/%s/%.*s", String_cstr(m_RootPath), META_PATH_NAME, meta_file_name.length(), meta_file_name.bgn);

    return {allocator, buffer, buffer_size};
  }

  void Assets::loadMeta(StringRange meta_file_name)
  {
    char             path_buffer[path::MAX_LENGTH];
    LinearAllocator  alloc_path{path_buffer, sizeof(path_buffer)};
    NoFreeAllocator  alloc_path_adapter{alloc_path};
    const TempBuffer meta_file_path = metaFullPath(alloc_path_adapter, meta_file_name);

    File project_file{meta_file_path.buffer(), file::FILE_MODE_READ};

    if (project_file)
    {
      std::size_t buffer_size;
      char*       buffer = project_file.readAll(m_Memory, buffer_size);

      json::Value loaded_data = json::fromString(buffer, buffer_size);

      m_Memory.deallocate(buffer);

      const String             path      = loaded_data.get<String>("Path");
      const String             uuid_str  = loaded_data.get<String>("UUID");
      const String             type      = loaded_data.get<String>("Type");
      meta::BaseClassMetaInfo* type_info = meta::TypeInfoFromName(std::string_view{type.begin(), type.length()});

      if (!type_info || uuid_str.isEmpty())
      {
        std::printf("[Assets::loadMeta]: WARNING could not find asset datatype: %s\n", type.c_str());
        return;
      }

      BifrostUUID uuid = bfUUID_fromString(uuid_str.c_str());

      if (bfUUID_isEmpty(&uuid))
      {
        std::printf("[Assets::loadMeta]: WARNING failed to load UUID.");
        return;
      }

      Any            asset_handle   = type_info->instantiate(m_Memory, StringRange(path), uuid);
      BaseAssetInfo* asset_handle_p = asset_handle.as<BaseAssetInfo*>();

      m_AssetMap.emplace(uuid, asset_handle_p);
      m_NameToGUID.emplace(path.c_str(), uuid);

      JsonSerializerReader reader{*this, m_Memory, loaded_data};

      reader.beginDocument(false);
      asset_handle_p->serialize(m_Engine, reader);
      reader.endDocument();

      project_file.close();
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

    m_MetaPath.clear();
    m_MetaPath.append(m_RootPath, path_length);
    m_MetaPath.append(META_PATH_NAME, sizeof(META_PATH_NAME) - 1);

    // TODO: Canonicalize Paths

    return AssetError::NONE;
  }

  void Assets::setRootPath(std::nullptr_t)
  {
    m_NameToGUID.clear();
    m_AssetMap.clear();

    if (m_RootPath)
    {
      String_clear(&m_RootPath);
    }
  }

  void Assets::markDirty(const BaseAssetHandle& asset)
  {
    if (asset && !asset.info()->m_IsDirty)
    {
      m_DirtyAssetList.push(asset);
      asset.info()->m_IsDirty = true;
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
    for (const auto& asset : m_DirtyAssetList)
    {
      const LinearAllocatorScope asset_scope           = {m_Engine.tempMemory()};
      BaseAssetInfo* const       info                  = asset.info();
      std::size_t                meta_file_name_length = 0u;
      char*                      meta_file_name        = metaFileName(m_Engine.tempMemoryNoFree(), info->path(), meta_file_name_length);
      const TempBuffer           meta_file_path        = metaFullPath(m_Engine.tempMemoryNoFree(), {meta_file_name, meta_file_name + meta_file_name_length});

      {
        const LinearAllocatorScope json_writer_scope = {m_Engine.tempMemory()};
        JsonSerializerWriter       json_writer       = JsonSerializerWriter{m_Engine.tempMemoryNoFree()};

        json_writer.beginDocument(false);
        const bool is_engine_asset = info->save(m_Engine, json_writer);
        json_writer.endDocument();

        if (is_engine_asset)
        {
          const String full_path = fullPath(*info);

          writeJsonToFile(full_path, json_writer.document());
        }
      }
      {
        const LinearAllocatorScope json_writer_scope = {m_Engine.tempMemory()};
        JsonSerializerWriter       json_writer       = JsonSerializerWriter{m_Engine.tempMemoryNoFree()};

        json_writer.beginDocument(false);
        info->serialize(m_Engine, json_writer);

        String type_info_name = {info->m_TypeInfo->name().data(), info->m_TypeInfo->name().size()};

        json_writer.serialize("Path", const_cast<String&>(info->path()));
        json_writer.serialize("UUID", const_cast<BifrostUUID&>(info->uuid()));
        json_writer.serialize("Type", type_info_name);

        json_writer.endDocument();

        writeJsonToFile({meta_file_path.buffer(), meta_file_path.size()}, json_writer.document());
      }

      info->m_IsDirty = false;
    }

    m_DirtyAssetList.clear();
  }

  Assets::~Assets()
  {
    if (m_RootPath)
    {
      String_delete(m_RootPath);
    }
  }
}  // namespace bifrost
