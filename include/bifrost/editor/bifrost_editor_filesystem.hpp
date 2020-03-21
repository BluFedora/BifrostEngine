/******************************************************************************/
/*!
* @file   bifrost_editor_filesystem.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*   Filesystem managment for the editor.
*
* @version 0.0.1
* @date    2020-03-09
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#ifndef BIFROST_EDITOR_FILESYSTEM_HPP
#define BIFROST_EDITOR_FILESYSTEM_HPP

#include "bifrost/data_structures/bifrost_intrusive_list.hpp"  // Node
#include "bifrost/data_structures/bifrost_string.hpp"          // String

namespace bifrost::editor
{
  class EditorOverlay;

  struct FileEntry final
  {
    String                         name; // TODO: Make Name Just a StringRange into 'FileEntry::full_path'
    String                         full_path;
    bool                           is_file;
    BifrostUUID                    uuid;
    intrusive::ListView<FileEntry> children;
    intrusive::Node<FileEntry>     next;

   public:
    FileEntry(String&& name, const String& full_path, bool is_file) :
      name{name},
      full_path{full_path},
      is_file{is_file},
      uuid{bfUUID_makeEmpty()},
      children{&FileEntry::next},
      next{}
    {
    }
  };

  class FileSystem final : bfNonCopyMoveable<FileSystem>
  {
   private:
    IMemoryManager&   m_Memory;
    Array<FileEntry*> m_AllNodes;
    FileEntry*        m_Root;
    FileEntry*        m_RenamedNode;
    bool              m_HasBeenModified;

   public:
    explicit FileSystem(IMemoryManager& memory) :
      m_Memory{memory},
      m_AllNodes{memory},
      m_Root{nullptr},
      m_RenamedNode{nullptr},
      m_HasBeenModified{false}
    {
    }

    FileEntry& root() const { return *m_Root; }

    void        clear(String&& name, const String& path);
    FileEntry&  makeNode(String&& name, const String& path, bool is_file);
    void        uiShow(EditorOverlay& editor);
    StringRange relativePath(const FileEntry& entry) const;
    void        remove(FileEntry& entry);

    ~FileSystem();

   private:
    void uiShowImpl(EditorOverlay& editor, FileEntry& entry);
    void clearImpl();
  };
}  // namespace bifrost::editor

#endif /* BIFROST_EDITOR_FILESYSTEM_HPP */
