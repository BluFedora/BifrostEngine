/******************************************************************************/
/*!
  @file   main.cpp
  @author Shareef Abdoul-Raheem
  @par    email: shareef.a\@digipen.edu
  @par    DigiPen login: shareef.a
  @par    Course: CS260
  @par    Assignment #3
  @date   11/17/2019
  @brief
    Cross platform networking API for implementing Assignment 3 for CS260 (TCP).
*/
/******************************************************************************/

#include "NetworkError.hpp"  // TODO: Remove

#include "NetworkContext.hpp"  // All the network classes.
#include <algorithm>           // min
#include <cassert>             // assert
#include <cstdio>              // printf
#include <cstring>             // strlen, memcpy
#include <utility>
#include <vector>  // vector<T>

static constexpr int            MESSAGE_BUFFER_SIZE = 256;
static constexpr unsigned short k_SRSMServerPort    = 4512;

struct BufferPage final
{
  char                   buffer[MESSAGE_BUFFER_SIZE] = {'\0'};
  detail::BytesCountImpl buffer_bytes_left           = MESSAGE_BUFFER_SIZE;
  BufferPage*            next                        = nullptr;

  detail::BytesCountImpl bufferSize() const { return MESSAGE_BUFFER_SIZE - buffer_bytes_left; }
};

class MessageBuffer final
{
 private:
  BufferPage* m_PageHead;
  BufferPage* m_PageCurrent;

 public:
  explicit MessageBuffer() :
    m_PageHead(new BufferPage()),
    m_PageCurrent{m_PageHead}
  {
  }

  MessageBuffer(const MessageBuffer&) = delete;
  MessageBuffer(MessageBuffer&&)      = delete;
  MessageBuffer& operator=(const MessageBuffer&) = delete;
  MessageBuffer& operator=(MessageBuffer&&) = delete;

  template<typename F>
  void forEachPage(F&& fn) const
  {
    BufferPage* page = m_PageHead;

    while (page)
    {
      BufferPage* const next = page->next;
      fn(page);
      page = next;
    }
  }

  void writeBytes(const char* buffer, detail::BytesCountImpl buffer_size)
  {
    detail::BytesCountImpl buffer_offset = 0;

    while (buffer_size > 0)
    {
      const detail::BytesCountImpl num_bytes_to_take = std::min(m_PageCurrent->buffer_bytes_left, buffer_size);
      const detail::BytesCountImpl page_offset       = m_PageCurrent->bufferSize();

      m_PageCurrent->buffer_bytes_left -= num_bytes_to_take;

      std::memcpy(m_PageCurrent->buffer + page_offset, buffer + buffer_offset, num_bytes_to_take);

      if (m_PageCurrent->buffer_bytes_left == 0)
      {
        const auto new_page = new BufferPage();
        m_PageCurrent->next = new_page;
        m_PageCurrent       = new_page;
      }

      buffer_size -= num_bytes_to_take;
      buffer_offset += num_bytes_to_take;
    }
  }

  ~MessageBuffer()
  {
    forEachPage([](BufferPage* page) { delete page; });
  }
};

// TODO: Move to core lib.
namespace cs260
{
  enum class HttpRequestMethod
  {
    POST,
    GET,
  };

  class HttpRequest final
  {
   private:
    std::string m_Request;

   public:
    HttpRequest(HttpRequestMethod method, const char* file) :
      m_Request{}
    {
      switch (method)
      {
        case HttpRequestMethod::POST:
          m_Request += "POST";
          break;
        case HttpRequestMethod::GET:
          m_Request += "GET";
          break;
      }

      m_Request += std::string(" ") + file + " HTTP/1.1";
      newLine();
    }

    void from(const char* email)
    {
      addKeyValue("From", email);
    }

    void host(const char* url)
    {
      addKeyValue("Host", url);
    }

    void accept(const char* mime_type)
    {
      addKeyValue("Accept", mime_type);
    }

    void userAgent(const char* agent)
    {
      addKeyValue("User-Agent", agent);
    }

    void contentType(const char* mime_type)
    {
      addKeyValue("Content-Type", mime_type);
    }

    void contentLength(const char* length)
    {
      addKeyValue("Content-Length", length);
    }

    void connection(const char* action)
    {
      addKeyValue("Connection", action);
    }

    void content(const char* data)
    {
      newLine();
      m_Request += data;
    }

    void end()
    {
      newLine();
    }

    /*[[nodiscard]] */ const std::string& request() const
    {
      return m_Request;
    }

   private:
    void addKeyValue(const char* key, const char* value)
    {
      m_Request += key;
      m_Request += ": ";
      m_Request += value;
      newLine();
    }

    void newLine()
    {
      m_Request += "\r\n";
    }
  };

#if 0
  std::vector<std::string> split(std::string source, const std::string& delimiters = "\r\n")
  {
    std::vector<std::string> results = {};
    const char*              first   = source.data();
    const char*              second  = first;
    const char* const        end     = first + source.length();

    while (second != end && first != end)
    {
      second = std::find_first_of(first, end, delimiters.cbegin(), delimiters.cend());

      if (first != second)
      {
        results.emplace_back(first, second - first);
      }

      first = second + 1;
    }

    return results;
  }
#endif
}  // namespace cs260

int main(int argc, const char* argv[])
{
  using namespace cs260;

  try
  {
    const char* const          url            = "localhost";
    const NetworkContextHandle network_ctx    = NetworkContext::create();
    const RequestURL           request_url    = RequestURL::create(url, k_SRSMServerPort);
    const NetworkFamily        network_family = NetworkFamily::IPv4;
    const SocketHandle         socket         = network_ctx->createSocket(network_family, SocketType::TCP);
    const Address              address        = network_ctx->makeAddress(network_family, request_url.ip_address, k_SRSMServerPort);

    if (!socket)
    {
      printf("Failed to create socket\n");
      return -1;
    }

    while (!socket->connectTo(address))
    {
      std::printf("Waiting on server...\n");
    }

#if 0
  retry:
    try
    {
      socket->connectTo(address);
    }
    catch (const NetworkError& e)
    {
      (void)e;
      std::printf("Waiting on server...\n");
      goto retry;
    }
#endif

    char large_buffer[1500 * 3];

    for (auto& b : large_buffer)
    {
      b = '$';
    }

    memcpy(large_buffer, "Hello", 5);

    large_buffer[sizeof(large_buffer) - 1] = '\0';

    const int num_bytes_sent = socket->sendDataTo(address, large_buffer, sizeof(large_buffer), SendToFlags::NONE);

    //assert(num_bytes_sent == 5);

    //socket->shutdown(SocketShutdownAction::SEND);


    socket->makeNonBlocking();

    char read_buffer[MESSAGE_BUFFER_SIZE];

    while (true)
    {
      const auto received_data = socket->receiveDataFrom(read_buffer, MESSAGE_BUFFER_SIZE);

      if (received_data.received_bytes_size == -2)
      {
        break;
      }

      if (received_data.received_bytes_size == -1)
      {
        std::printf("Waiting on message ;)\n");
        continue;
      }

      if (received_data.received_bytes_size == 0)
      {
        break;
      }

      if (received_data.received_bytes_size > 0)
      {
        std::printf("Got '%.*s' from the server\n", received_data.received_bytes_size, read_buffer);
      }
    }
  }
  catch (const NetworkError& e)
  {
    std::printf("Network API Error %s\n", e.what());
    return 2;
  }
  catch (const std::exception& e)
  {
    std::printf("Error %s\n", e.what());
    return 3;
  }

  return 0;
}
