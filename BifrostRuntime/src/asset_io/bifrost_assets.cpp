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
#include "bifrost/asset_io/bifrost_assets.hpp"

#include "bifrost/asset_io/bifrost_asset_handle.hpp"     //
#include "bifrost/asset_io/bifrost_file.hpp"             // File
#include "bifrost/asset_io/bifrost_json_serializer.hpp"  //
#include "bifrost/memory/bifrost_linear_allocator.hpp"   /* LinearAllocator */
#include "bifrost/meta/bifrost_meta_runtime.hpp"         //

#include "bf/Platform.h"
#include "bf/asset_io/bf_path_manip.hpp"

#include <filesystem> /* filesystem::* */

#if BIFROST_PLATFORM_WINDOWS
#define NOMINMAX
#include <Windows.h>  // HANDLE
#else
#include <dirent.h>
#endif

#include "bifrost/core/bifrost_engine.hpp"  // THIS HAS A GLFW HEADER WHICH FUCKS WITH WINDOWS.h

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
    if (info && isHandleCompatible(handle, info))
    {
      handle = makeHandle(*info);
      return true;
    }

    return false;
  }

  BaseAssetHandle Assets::makeHandle(BaseAssetInfo& info) const
  {
    return BaseAssetHandle(m_Engine, &info, info.payloadType());
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

      m_Memory.deallocate(buffer, buffer_size);

      JsonSerializerReader reader{*this, m_Memory, loaded_data};

      if (reader.beginDocument(false))
      {
        readMetaInfo(reader);

        reader.endDocument();
      }

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

    const std::size_t new_root_path_length = file::canonicalizePath(m_RootPath);
    String_resize(&m_RootPath, new_root_path_length);

    m_MetaPath.clear();
    m_MetaPath.append(m_RootPath, new_root_path_length);
    m_MetaPath.append('/');
    m_MetaPath.append(META_PATH_NAME, sizeof(META_PATH_NAME) - 1);

    return AssetError::NONE;
  }

  void Assets::setRootPath(std::nullptr_t)
  {
    m_NameToGUID.clear();

    for (const auto& entry : m_AssetMap)
    {
      m_Memory.deallocateT(entry.value());
    }

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
    LinearAllocator& temp_alloc         = m_Engine.tempMemory();
    IMemoryManager&  temp_alloc_no_free = m_Engine.tempMemoryNoFree();

    for (const auto& asset : m_DirtyAssetList)
    {
      saveAssetInfo(temp_alloc, temp_alloc_no_free, asset.info());
    }

    clearDirtyList();
  }

  void Assets::saveAssetInfo(LinearAllocator& temp_alloc, IMemoryManager& temp_alloc_no_free, BaseAssetInfo* info)
  {
    const LinearAllocatorScope asset_mem_scope       = {temp_alloc};
    std::size_t                meta_file_name_length = 0u;
    char*                      meta_file_name        = metaFileName(temp_alloc_no_free, info->filePathRel(), meta_file_name_length);
    const TempBuffer           meta_file_path        = metaFullPath(temp_alloc_no_free, {meta_file_name, meta_file_name + meta_file_name_length});

    // Save Engine Asset
    {
      const LinearAllocatorScope json_writer_scope = {temp_alloc};
      JsonSerializerWriter       json_writer       = JsonSerializerWriter{temp_alloc_no_free};

      if (json_writer.beginDocument(false))
      {
        const bool is_engine_asset = info->save(m_Engine, json_writer);
        json_writer.endDocument();

        if (is_engine_asset)
        {
          writeJsonToFile(info->filePathAbs(), json_writer.document());
        }
      }
    }

    // Save Meta Info
    {
      const LinearAllocatorScope json_writer_scope = {temp_alloc};
      JsonSerializerWriter       json_writer       = JsonSerializerWriter{temp_alloc_no_free};

      if (json_writer.beginDocument(false))
      {
        writeMetaInfo(json_writer, info);
        json_writer.endDocument();

        writeJsonToFile({meta_file_path.buffer(), meta_file_path.size()}, json_writer.document());
      }
    }
  }

  void Assets::saveAssetInfo(Engine& engine, BaseAssetInfo* info)
  {
    saveAssetInfo(engine.tempMemory(), engine.tempMemoryNoFree(), info);
  }

  void Assets::clearDirtyList()
  {
    for (const auto& asset : m_DirtyAssetList)
    {
      BaseAssetInfo* const info = asset.info();

      info->m_IsDirty = false;
    }

    m_DirtyAssetList.clear();
  }

  String Assets::relPathToAbsPath(const StringRange& rel_path) const
  {
    return path::append({m_RootPath, String_length(m_RootPath)}, rel_path);
  }

#if 0  // TODO(SR): This function cannot work as intended since it does not know the diff between a '.' for a dir separator vs in the actual file name...
  String Assets::metaPathToRelPath(const StringRange& meta_path)
  {
    String rel_path = {meta_path.begin(), meta_path.length() - sizeof(META_FILE_EXTENSION) + 1};  // The + 1 accounts for the nul terminator.

    std::transform(rel_path.begin(), rel_path.end(), rel_path.begin(), [](const char character) {
      return character == '.' ? '/' : character;
    });

    return rel_path;
  }
#endif

  Assets::~Assets()
  {
    if (m_RootPath)
    {
      String_delete(m_RootPath);
    }
  }

  std::pair<BifrostUUID, bool> Assets::indexAssetImpl(const StringRange abs_path)
  {
    const StringRange relative_path = path::relative(m_RootPath, abs_path);
    const auto        it_name       = m_NameToGUID.find(relative_path);

    if (it_name != m_NameToGUID.end())
    {
      const auto it_asset = m_AssetMap.find(it_name->value());

      if (it_asset != m_AssetMap.end())
      {
        return {it_name->value(), false};
      }
    }

    const BifrostUUID uuid = bfUUID_generate();

    m_NameToGUID.emplace(relative_path, uuid);

    return {uuid, true};
  }

  void Assets::addSubAssetTo(BaseAssetInfo* parent_asset, BaseAssetInfo* child_asset)
  {
    parent_asset->addSubAsset(child_asset);
  }

  void Assets::writeMetaInfo(JsonSerializerWriter& json_writer, BaseAssetInfo* info)
  {
    info->serialize(m_Engine, json_writer);

    String type_info_name = {info->m_TypeInfo->name().data(), info->m_TypeInfo->name().size()};
    String path_as_str    = info->filePathRel();

    json_writer.serialize("Path", path_as_str);
    json_writer.serialize("UUID", const_cast<BifrostUUID&>(info->uuid()));
    json_writer.serialize("Type", type_info_name);

    std::size_t num_sub_assets;

    if (json_writer.pushArray("m_SubAssets", num_sub_assets))
    {
      for (BaseAssetInfo& sub_asset : info->m_SubAssets)
      {
        if (json_writer.pushObject(nullptr))
        {
          writeMetaInfo(json_writer, &sub_asset);

          json_writer.popObject();
        }
      }

      json_writer.popArray();
    }
  }

  BaseAssetInfo* Assets::readMetaInfo(JsonSerializerReader& reader, bool is_sub_asset)
  {
    String rel_path;
    String uuid_str;
    String type;

    reader.serialize("Path", rel_path);
    reader.serialize("UUID", uuid_str);
    reader.serialize("Type", type);

    meta::BaseClassMetaInfo* type_info = meta::TypeInfoFromName(std::string_view{type.begin(), type.length()});

    if (!type_info || uuid_str.isEmpty())
    {
      std::printf("[Assets::loadMeta]: WARNING could not find asset datatype: %s\n", type.c_str());
      return nullptr;
    }

    BifrostUUID uuid = bfUUID_fromString(uuid_str.c_str());

    if (bfUUID_isEmpty(&uuid))
    {
      std::printf("[Assets::loadMeta]: WARNING failed to load UUID.");
      return nullptr;
    }

    const String            abs_path         = is_sub_asset ? rel_path : relPathToAbsPath(rel_path);
    const std::size_t       root_path_length = is_sub_asset ? 0u : String_length(m_RootPath);
    const meta::MetaVariant asset_handle     = type_info->instantiate(m_Memory, abs_path, root_path_length, uuid);
    BaseAssetInfo*          asset_handle_p   = meta::variantToCompatibleT<BaseAssetInfo*>(asset_handle);

    if (asset_handle_p)
    {
      asset_handle_p->m_TypeInfo = type_info;

      asset_handle_p->serialize(m_Engine, reader);

      m_AssetMap.emplace(uuid, asset_handle_p);
      m_NameToGUID.emplace(rel_path, uuid);

      std::size_t num_sub_assets = 0u;

      if (reader.pushArray("m_SubAssets", num_sub_assets))
      {
        for (std::size_t i = 0; i < num_sub_assets; ++i)
        {
          if (reader.pushObject(nullptr))
          {
            BaseAssetInfo* const sub_asset = readMetaInfo(reader, true);

            if (sub_asset)
            {
              asset_handle_p->addSubAsset(sub_asset);
            }

            reader.popObject();
          }
        }

        reader.popArray();
      }
    }

    return asset_handle_p;
  }
}  // namespace bf
