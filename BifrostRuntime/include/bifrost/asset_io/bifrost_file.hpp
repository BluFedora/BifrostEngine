#ifndef BIFROST_FILE_HPP
#define BIFROST_FILE_HPP

// uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t

#include "bifrost/data_structures/bifrost_string.hpp" /* String, StringRange */
#include "bifrost/memory/bifrost_imemory_manager.hpp" /* TenmBuffer          */

#include <algorithm> /* reverse      */
#include <fstream>   /* fstream, ios */

namespace bf
{
  namespace file
  {
    enum FileMode
    {
      FILE_MODE_APPEND        = std::ios::app,
      FILE_MODE_START_AT_END  = std::ios::ate,
      FILE_MODE_BINARY        = std::ios::binary,
      FILE_MODE_READ          = std::ios::in,
      FILE_MODE_WRITE         = std::ios::out,
      FILE_MODE_OVERWRITE_ALL = std::ios::trunc
    };

    enum class FileSeek
    {
      FILE_SEEK_BEGIN    = std::ios::beg,
      FILE_SEEK_RELATIVE = std::ios::cur,
      FILE_SEEK_END      = std::ios::end
    };

    enum class FileError
    {
      NONE,
      FILE_DID_NOT_OPEN
    };

    // TODO(SR): These path helpers can probably be in a separate header file.

    // TODO(SR): This function should be _modernized_ to use 'StringRange' as that would make things a bit clearer in usage.
    //           Also this has nothing to do with 'Paths' this is just a string algorithm!?!
    bool pathEndsIn(const char* path, const char* ending, int ending_len = -1, int path_len = -1);
    bool isValidName(const StringRange& path);

    /// All slashes are forward slashes '/'.      \n
    /// A path for a folder has no following '/'. \n
    /// Returns the new length of the path.       \n
    std::size_t canonicalizePath(char* path_bgn, const char* path_end);
    std::size_t canonicalizePath(char* path_bgn);  // Gives you the length

    // These functions assumes a canonical path by the calling of 'canonicalizePath'.
    StringRange directoryOfFile(const StringRange& path);  // In canonical style.
    StringRange extensionOfFile(const StringRange& path);  // Includes the (dot) e.g '.ext', if no extension a null path is returned.
    StringRange fileNameOfPath(const StringRange& path);   // 'filename.ext'

  }  // namespace file

  class File final
  {
   public:
    static bool exists(const char* path);

    template<class T>
    static void endianSwap(T* obj)
    {
      uint8_t* memp = reinterpret_cast<uint8_t*>(obj);
      std::reverse(memp, memp + sizeof(T));
    }

   private:
    String       m_FileName;
    std::fstream m_FileStream;

   public:
    File() = default;
    File(const char* filename, const unsigned int mode);
    File(const String& filename, const unsigned int mode);

    File(const File& rhs) = delete;
    File(File&& rhs)      = delete;
    File& operator=(const File& rhs) = delete;
    File& operator=(File&& rhs) = delete;

    // Basic FileIO //
    operator bool() const { return isOpen(); }
    file::FileError open(const char* filename, const unsigned int mode);
    file::FileError open(const String& filename, const unsigned int mode);
    bool            isOpen() const;
    void            seek(const int movement, const file::FileSeek mode);
    std::size_t     size() const;
    void            close();

    // Binary API (Endian Independent as Long as You Use this class for both reading and writing) //

    File& writeBytes(const char* bytes, std::streamsize size);
    File& writeInt8(std::int8_t value);
    File& writeInt16(std::int16_t value);
    File& writeInt32(std::int32_t value);
    File& writeInt64(std::int64_t value);
    File& writeUint8(std::uint8_t value);
    File& writeUint16(std::uint16_t value);
    File& writeUint32(std::uint32_t value);
    File& writeUint64(std::uint64_t value);

    File& readBytes(char* bytes, std::streamsize size);
    File& readInt8(int8_t& value);
    File& readInt16(std::int16_t& value);
    File& readInt32(std::int32_t& value);
    File& readInt64(std::int64_t& value);
    File& readUint8(std::uint8_t& value);
    File& readUint16(std::uint16_t& value);
    File& readUint32(std::uint32_t& value);
    File& readUint64(std::uint64_t& value);

    // Templated (Binary) API //
    template<typename T>
    File& write(const T& data);
    File& write(const String& data);
    template<typename T>
    File& read(T& data);

    // Read All API //

    // 'out' will be appended to so remember to clear if beforehand if need-be
    void readAll(String& out);

    // the returned buffer will be nul terminated but out_size includes that in the size.
    // Remember to free the buffer.
    char* readAll(IMemoryManager& allocator, std::size_t& out_size);

    // Same as the above overload except destructors handle freeing.
    [[nodiscard]] TempBuffer readAll(IMemoryManager& allocator);

    template<typename T>
    File& operator<<(const T& data)
    {
      m_FileStream << data;
      return *this;
    }

    template<typename T>
    File& operator>>(T& data)
    {
      m_FileStream >> data;
      return *this;
    }

    ~File() = default;
  };

  template<typename T>
  File& File::write(const T& data)
  {
    return writeBytes(reinterpret_cast<const char*>(&data), sizeof(T));
  }

  template<typename T>
  File& File::read(T& data)
  {
    return readBytes(reinterpret_cast<char*>(&data), sizeof(T));
  }
}  // namespace bifrost

#endif /* BIFROST_FILE_HPP */
