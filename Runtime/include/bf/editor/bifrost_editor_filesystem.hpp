/******************************************************************************/
/*!
* @file   bifrost_editor_filesystem.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*   Filesystem management for the editor.
*
* @version 0.0.1
* @date    2020-03-09
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#ifndef BIFROST_EDITOR_FILESYSTEM_HPP
#define BIFROST_EDITOR_FILESYSTEM_HPP

#include "bf/ListView.hpp"                        // ListView<T>, Node<T>
#include "bf/data_structures/bifrost_string.hpp"  // String

namespace bf::editor
{
  class EditorOverlay;

  struct FileEntry
  {
    String              name;
    String              full_path;
    StringRange         file_extension;  //!< Backed by FileEntry::full_path.
    ListView<FileEntry> children;
    Node<FileEntry>     next;
    IBaseAsset*         asset_info;
    bool                is_file;

   public:
    FileEntry(String&& name, const String& full_path, bool is_file);

    bool isFile() const { return is_file; }
  };

  class FileSystem final : NonCopyMoveable<FileSystem>
  {
   private:
    IMemoryManager&   m_Memory;
    Array<FileEntry*> m_AllNodes;
    FileEntry*        m_Root;
    FileEntry*        m_RenamedNode;
    bool              m_HasBeenModified;

   public:
    explicit FileSystem(IMemoryManager& memory);

    FileEntry& root() const { return *m_Root; }

    void       clear(String&& name, const String& path);
    FileEntry& makeNode(String&& name, const String& path, bool is_file);
    void       rename(EditorOverlay& editor, FileEntry& entry, const StringRange& new_name) const;
    void       remove(FileEntry& entry);
    void       uiShow(EditorOverlay& editor);

    ~FileSystem();

   private:
    void uiShowImpl(EditorOverlay& editor, FileEntry& entry);
    void clearImpl();
  };
}  // namespace bf::editor

#endif /* BIFROST_EDITOR_FILESYSTEM_HPP */
