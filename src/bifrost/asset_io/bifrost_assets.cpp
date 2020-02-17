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

#include "bifrost/memory/bifrost_imemory_manager.hpp"
#include "bifrost/platform/bifrost_platform.h"  // HANDLE

#include <filesystem> /*  */

#if BIFROST_PLATFORM_WINDOWS
#define NOMINMAX
#include <Windows.h>
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
      char              null_terminated_path[MAX_PATH] = {'\0'};
      const std::size_t null_terminated_path_length    = std::min(path.length(), bfCArraySize(null_terminated_path));

      std::strncpy(null_terminated_path, path.bgn, null_terminated_path_length);
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

  Assets::Assets() :
    m_NameToGUID{},
    m_AssetMap{},
    m_RootPath{nullptr}
  {
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

    m_RootPath = String_newLen(nullptr, 0);

    const fs_string_type fs_path_str = fs_path.native();

    String_resize(&m_RootPath, fs_path_str.length());

    int i = 0;

    for (const auto path_char : fs_path_str)
    {
      m_RootPath[i++] = static_cast<char>(path_char);
    }

    return AssetError::NONE;
  }

  void Assets::setRootPath(std::nullptr_t)
  {
    m_NameToGUID.clear();
    m_AssetMap.clear();

    if (m_RootPath)
    {
      String_delete(m_RootPath);
      m_RootPath = nullptr;
    }
  }

  Assets::~Assets()
  {
    setRootPath(nullptr);
  }
}  // namespace bifrost
