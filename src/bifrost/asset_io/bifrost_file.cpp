#include "bifrost/asset_io/bifrost_file.hpp"

#if _WIN32 /* Windows */
#include <Windows.h>
#include <cassert>
#include <io.h>
#define access _access_s
#define F_OK 0
#else /* macOS, Linux, Android, iOS */
#include <sys/stat.h>
#include <unistd.h> /* access, F_OK */
#endif

#include <cstring> /* strlen      */
#include <memory>  /* unique_ptr  */

namespace bifrost
{
  namespace file
  {
    bool pathEndsIn(const char* path, const char* ending, int ending_len, int path_len)
    {
      if (path_len <= 0) { path_len = (int)std::strlen(path); }
      if (ending_len <= 0) { ending_len = (int)std::strlen(ending); }

      return std::strncmp(path + path_len - ending_len, ending, ending_len) == 0;
    }

    template<typename F>
    static std::size_t canonicalizePath_(char* path_bgn, F&& condition)
    {
      const char* const path_bgn_cpy = path_bgn;

      // Replaces all '\' => '/' for cross-platform compatibility.
      while (condition(path_bgn))
      {
        if (*path_bgn == '\\')
        {
          *path_bgn = '/';
        }

        ++path_bgn;
      }

      std::size_t length = path_bgn - path_bgn_cpy;

      // Removes the trailing slash.
      if (length && path_bgn[-1] == '/')
      {
        path_bgn[-1] = '\0';
        --length;
      }

      return length;
    }

    std::size_t canonicalizePath(char* path_bgn, const char* path_end)
    {
      return canonicalizePath_(path_bgn, [path_end](const char* path_bgn) { return path_bgn != path_end; });
    }

    std::size_t canonicalizePath(char* path_bgn)
    {
      return canonicalizePath_(path_bgn, [](const char* path_bgn) { return *path_bgn; });
    }

    StringRange directoryOfFile(const StringRange& path)
    {
      const char* end = path.end;

      while (end != path.bgn)
      {
        if (*end == '/')
        {
          break;
        }

        --end;
      }

      return {path.bgn, end};
    }
    /*
    String pathFileName(String filename)
    {
      const std::size_t last_slash_idx = filename.find_last_of("\\/");

      if (last_slash_idx != std::string::npos)
      {
        filename.erase(0, last_slash_idx + 1);
      }

      std::size_t period_idx;

      do
      {
        period_idx = filename.rfind('.');

        if (period_idx == std::string::npos)
          break;
        filename.erase(period_idx);
      } while (period_idx != std::string::npos);

      return filename;
    }

    void replaceAll(String& path, char replace, char with)
    {
      std::transform(path.begin(), path.end(), path.begin(), [replace, with](char c) -> char
      {
        if (c == replace) 
          return with;
        return c;
      });
    }
    */
  }  // namespace file

  bool File::exists(const char* path)
  {
    return access(path, F_OK) == 0;
  }

  File::File(const char* filename, const unsigned int mode)
  {
    open(filename, mode);
  }

  File::File(const String& filename, const unsigned int mode)
  {
    open(filename.cstr(), mode);
  }

  file::FileError File::open(const char* filename, const unsigned int mode)
  {
    m_FileName = filename;
    m_FileStream.open(filename, std::ios_base::openmode(mode));

    if (!isOpen())
    {
      return file::FileError::FILE_DID_NOT_OPEN;
    }

    return file::FileError::NONE;
  }

  file::FileError File::open(const String& filename, const unsigned int mode)
  {
    return open(filename.cstr(), mode);
  }

  bool File::isOpen() const
  {
    return m_FileStream.is_open();
  }

  void File::seek(const int movement, const file::FileSeek mode)
  {
    m_FileStream.seekg(movement, static_cast<std::ios_base::seekdir>(mode));
  }

  std::size_t File::size() const
  {
#if _WIN32
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    const BOOL                fOk = GetFileAttributesExA(m_FileName.cstr(), GetFileExInfoStandard, &fileInfo);

    if (!fOk)
      return -1;

    assert(0 == fileInfo.nFileSizeHigh);
    return std::size_t(fileInfo.nFileSizeLow);
#else
    struct stat st;
    stat(m_FileName.cstr(), &st);
    return (std::size_t)st.st_size;
#endif
  }

  void File::close()
  {
    m_FileStream.close();
  }

  File& File::writeBytes(const char* bytes, std::streamsize size)
  {
    m_FileStream.write(bytes, size);
    return *this;
  }

  File& File::writeInt8(const int8_t value)
  {
    return write<int8_t>(value);
  }

  File& File::writeInt16(const int16_t value)
  {
    int8_t bytes[2];

    bytes[0] = (value & 0x00FF) >> 0;
    bytes[1] = (value & 0xFF00) >> 8;

    return writeBytes((char*)bytes, sizeof(bytes));
  }

  File& File::writeInt32(const int32_t value)
  {
    int8_t bytes[4];

    bytes[0] = (value & 0x000000FF) >> 0;
    bytes[1] = (value & 0x0000FF00) >> 8;
    bytes[2] = (value & 0x00FF0000) >> 16;
    bytes[3] = (value & 0xFF000000) >> 24;

    return writeBytes((char*)bytes, sizeof(bytes));
  }

  File& File::writeInt64(const int64_t value)
  {
    int8_t bytes[8];

    bytes[0] = static_cast<int8_t>((value & 0x00000000000000FF) >> 0);
    bytes[1] = static_cast<int8_t>((value & 0x000000000000FF00) >> 8);
    bytes[2] = static_cast<int8_t>((value & 0x0000000000FF0000) >> 16);
    bytes[3] = static_cast<int8_t>((value & 0x00000000FF000000) >> 24);
    bytes[4] = static_cast<int8_t>((value & 0x000000FF00000000) >> 32);
    bytes[5] = static_cast<int8_t>((value & 0x0000FF0000000000) >> 40);
    bytes[6] = static_cast<int8_t>((value & 0x00FF000000000000) >> 48);
    bytes[7] = static_cast<int8_t>((value & 0xFF00000000000000) >> 56);

    return writeBytes((char*)bytes, sizeof(bytes));
  }

  File& File::writeUint8(const uint8_t value)
  {
    return write<uint8_t>(value);
  }

  File& File::writeUint16(const uint16_t value)
  {
    const uint8_t l_bits = (value & 0x00FF) >> 0;
    const uint8_t h_bits = (value & 0xFF00) >> 8;

    writeUint8(l_bits);
    writeUint8(h_bits);

    return *this;
  }

  File& File::writeUint32(const uint32_t value)
  {
    const uint8_t bits0 = (value & 0x000000FF) >> 0;
    const uint8_t bits1 = (value & 0x0000FF00) >> 8;
    const uint8_t bits2 = (value & 0x00FF0000) >> 16;
    const uint8_t bits3 = (value & 0xFF000000) >> 24;

    writeUint8(bits0);
    writeUint8(bits1);
    writeUint8(bits2);
    writeUint8(bits3);

    return *this;
  }

  File& File::writeUint64(const uint64_t value)
  {
    const auto bits0 = static_cast<uint8_t>((value & 0x00000000000000FFull) >> 0);
    const auto bits1 = static_cast<uint8_t>((value & 0x000000000000FF00ull) >> 8);
    const auto bits2 = static_cast<uint8_t>((value & 0x0000000000FF0000ull) >> 16);
    const auto bits3 = static_cast<uint8_t>((value & 0x00000000FF000000ull) >> 24);
    const auto bits4 = static_cast<uint8_t>((value & 0x000000FF00000000ull) >> 32);
    const auto bits5 = static_cast<uint8_t>((value & 0x0000FF0000000000ull) >> 40);
    const auto bits6 = static_cast<uint8_t>((value & 0x00FF000000000000ull) >> 48);
    const auto bits7 = static_cast<uint8_t>((value & 0xFF00000000000000ull) >> 56);

    writeUint8(bits0);
    writeUint8(bits1);
    writeUint8(bits2);
    writeUint8(bits3);
    writeUint8(bits4);
    writeUint8(bits5);
    writeUint8(bits6);
    writeUint8(bits7);

    return *this;
  }

  File& File::readBytes(char* bytes, std::streamsize size)
  {
    m_FileStream.read(bytes, size);
    return *this;
  }

  File& File::readInt8(int8_t& value)
  {
    return read<int8_t>(value);
  }

  File& File::readInt16(int16_t& value)
  {
    int8_t bits[2];
    read<decltype(bits)>(bits);

    value = ((int16_t)bits[0] << 0) | ((int16_t)bits[1] << 8);

    return *this;
  }

  File& File::readInt32(int32_t& value)
  {
    int8_t bits[4];
    read<decltype(bits)>(bits);

    value = ((int32_t)bits[0] << 0) | ((int32_t)bits[1] << 8) | ((int32_t)bits[2] << 16) | ((int32_t)bits[3] << 24);

    return *this;
  }

  File& File::readInt64(int64_t& value)
  {
    int8_t bits[8];
    read<decltype(bits)>(bits);

    value = ((int64_t)bits[0] << 0) | ((int64_t)bits[1] << 8) | ((int64_t)bits[2] << 16) | ((int64_t)bits[3] << 24) |
            ((int64_t)bits[4] << 32) | ((int64_t)bits[5] << 40) | ((int64_t)bits[6] << 48) | ((int64_t)bits[7] << 56);

    return *this;
  }

  File& File::readUint8(uint8_t& value)
  {
    return read<uint8_t>(value);
  }

  File& File::readUint16(uint16_t& value)
  {
    uint8_t bits[2];
    read<decltype(bits)>(bits);

    value = ((uint16_t)bits[0] >> 0) | ((uint16_t)bits[1] >> 8);

    return *this;
  }

  File& File::readUint32(uint32_t& value)
  {
    uint8_t bits[4];
    read<decltype(bits)>(bits);

    value = ((uint32_t)bits[0] >> 0) | ((uint32_t)bits[1] >> 8) | ((uint32_t)bits[2] >> 16) | ((uint32_t)bits[3] >> 24);

    return *this;
  }

  File& File::readUint64(uint64_t& value)
  {
    uint8_t bits[8];
    read<decltype(bits)>(bits);

    value = ((uint64_t)bits[0] << 0) | ((uint64_t)bits[1] << 8) | ((uint64_t)bits[2] << 16) | ((uint64_t)bits[3] << 24) |
            ((uint64_t)bits[4] << 32) | ((uint64_t)bits[5] << 40) | ((uint64_t)bits[6] << 48) | ((uint64_t)bits[7] << 56);

    return *this;
  }

  void File::readAll(String& out)
  {
    out.reserve(size());

    std::istreambuf_iterator<char>       file_stream_bgn = m_FileStream;
    const std::istreambuf_iterator<char> file_stream_end = std::istreambuf_iterator<char>();

    while (file_stream_bgn != file_stream_end)
    {
      out.append(*file_stream_bgn);
      ++file_stream_bgn;
    }
  }
}  // namespace bifrost