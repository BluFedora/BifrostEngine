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
    String                         path;
    String                         full_path;
    bool                           is_file;
    BifrostUUID                    uuid;
    intrusive::ListView<FileEntry> children;
    intrusive::Node<FileEntry>     next;

   public:
    FileEntry(String&& name, const String& full_path, bool is_file) :
      path{name},
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

   public:
    explicit FileSystem(IMemoryManager& memory) :
      m_Memory{memory},
      m_AllNodes{memory},
      m_Root{nullptr}
    {
    }

    FileEntry& root() const { return *m_Root; }

    void clear(String&& name, const String& path)
    {
      clearImpl();
      m_Root = &makeNode(std::move(name), path, false);
    }

    FileEntry& makeNode(String&& name, const String& path, bool is_file)
    {
      FileEntry* const entry = m_Memory.allocateT<FileEntry>(std::move(name), path, is_file);
      m_AllNodes.push(entry);

      return *entry;
    }

    void uiShow(EditorOverlay& editor) const;

    ~FileSystem()
    {
      clearImpl();
    }

   private:
    static void uiShowImpl(EditorOverlay& editor, FileEntry& entry);

    void clearImpl()
    {
      for (FileEntry* const entry : m_AllNodes)
      {
        m_Memory.deallocateT(entry);
      }

      m_AllNodes.clear();
    }
  };
}  // namespace bifrost::editor

#endif /* BIFROST_EDITOR_FILESYSTEM_HPP */
