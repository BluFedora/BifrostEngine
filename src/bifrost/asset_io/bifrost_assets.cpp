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

#include "bifrost/asset_io/bifrost_file.hpp"           // File
#include "bifrost/asset_io/bifrost_json_parser.hpp"    //
#include "bifrost/asset_io/bifrost_json_value.hpp"     // JsonValue
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

  Assets::Assets(Engine& engine, IMemoryManager& memory) :
    m_Engine{engine},
    m_Memory{memory},
    m_NameToGUID{},
    m_AssetMap{},
    m_RootPath{nullptr},
    m_MetaPath{}
  {
  }

  BifrostUUID Assets::indexAssetImpl(const StringRange path, StringRange meta_file_name, bool& create_new, meta::BaseClassMetaInfo* type_info)
  {
    const String path_key = path;
    const auto   it_name  = m_NameToGUID.find(path_key);

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

    BifrostUUID uuid = bfUUID_generate();

    char            path_buffer[path::MAX_LENGTH];
    LinearAllocator alloc_path{path_buffer, sizeof(path_buffer)};

    // TODO(Shareef): Make this a helper function.
    string_utils::alloc_fmt(alloc_path, nullptr, "%s/%s/%.*s", m_RootPath, META_PATH_NAME, meta_file_name.length(), meta_file_name.bgn);

    const JsonValue json = JsonValue{
     std::pair{std::string("Path"), std::string(path.bgn, path.end())},
     std::pair{std::string("UUID"), std::string(uuid.as_string)},
     std::pair{std::string("Type"), std::string(type_info->name())},
    };

    std::string json_str;
    json.toString(json_str, true, 4);

    File project_file{path_buffer, file::FILE_MODE_WRITE};

    if (project_file)
    {
      project_file.writeBytes(json_str.c_str(), json_str.length());
      project_file.close();
    }

    m_NameToGUID.emplace(path_key, uuid);

    create_new = true;
    return uuid;
  }

  void Assets::loadMeta(StringRange meta_file_name)
  {
    char            path_buffer[path::MAX_LENGTH];
    LinearAllocator alloc_path{path_buffer, sizeof(path_buffer)};

    // TODO(Shareef): Make this a helper function.
    string_utils::alloc_fmt(alloc_path, nullptr, "%s/%s/%.*s", m_RootPath, META_PATH_NAME, meta_file_name.length(), meta_file_name.bgn);

    File project_file{path_buffer, file::FILE_MODE_READ};

    if (project_file)
    {
      String output;
      project_file.readAll(output);

      JsonValue                loaded_data = JsonParser::parse(output);
      const std::string        path        = loaded_data["Path"];
      const std::string        uuid_str    = loaded_data["UUID"];
      const std::string        type        = loaded_data["Type"];
      BifrostUUID              uuid        = bfUUID_fromString(uuid_str.c_str());
      meta::BaseClassMetaInfo* type_info   = meta::TypeInfoFromName(type);

      if (!type_info)
      {
        std::printf("[Assets::loadMeta]: WARNING could not find asset datatype: %s\n", type.c_str());
        return;
      }

      Any asset_handle = type_info->instantiate(m_Memory, StringRange(path.c_str(), path.c_str() + path.length()), uuid);
      BaseAssetInfo* asset_handle_p = asset_handle.as<BaseAssetInfo*>();

      m_AssetMap.emplace(uuid, asset_handle_p);
      m_NameToGUID.emplace(path.c_str(), uuid);

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

  Assets::~Assets()
  {
    if (m_RootPath)
    {
      String_delete(m_RootPath);
    }
  }
}  // namespace bifrost
