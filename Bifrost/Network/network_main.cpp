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
// #include <string>         // string
#include <cctype>   // tolower
#include <cstdlib>  // strtol
#include <vector>   // vector<T>

#undef min
#undef max

static constexpr int               MESSAGE_BUFFER_SIZE = 256;
static constexpr unsigned short    PORT                = 80;
static constexpr const char* const PORT_STR            = "80";

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
      BufferPage* next = page->next;
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
  static constexpr std::size_t ADDRESS_BUFFER_SIZE = std::max(INET6_ADDRSTRLEN, INET_ADDRSTRLEN);

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

  struct RequestURL final
  {
    static RequestURL create(std::string url, const char* port)
    {
      const auto double_slash = std::find(url.begin(), url.end(), '/');

      if (double_slash < url.end() && double_slash + 1 < url.end())
      {
        if (double_slash[1] == '/')
        {
          url = url.substr(double_slash - url.begin() + 2);
        }
      }

      const auto last_slash = std::find(url.begin(), url.end(), '/');

      auto        host    = url;
      std::string request = "/";

      if (last_slash != url.end())
      {
        host    = host.substr(0u, last_slash - url.begin());
        request = std::string(&*last_slash, url.end() - last_slash);
      }

      return {host, request, port};
    }

    std::string host;
    std::string request;
    char        ip_address[ADDRESS_BUFFER_SIZE];

   private:
    RequestURL(std::string host_, std::string request_, const char* port) :
      host{host_},
      request{request_},
      ip_address{'\0'}
    {
      addrinfo hint;                // NOLINT(hicpp-member-init)
      hint.ai_flags     = 0x0;      // AI_PASSIVE, AI_CANONNAME, AI_NUMERICHOST
      hint.ai_family    = AF_INET;  // AF_UNSPEC; // Accepts both IPv4 and IPv6
      hint.ai_socktype  = 0;        // Accepts any socket type.
      hint.ai_protocol  = 0;        // Accepts any protocol.
      hint.ai_addrlen   = 0;        // Required to be 0 by standard.
      hint.ai_canonname = nullptr;  // Required to be nullptr by standard.
      hint.ai_addr      = nullptr;  // Required to be nullptr by standard.
      hint.ai_next      = nullptr;  // Required to be nullptr by standard.

      addrinfo* result;
      const int err = getaddrinfo(host.c_str(), port, &hint, &result);

      if (err)
      {
        const char* error_str = gai_strerror(err);
        std::printf("getaddrinfo(%s:%s): %s\n", host.c_str(), port, error_str);
        throw NetworkError(NetworkErrorCode::FAILED_TO_CREATE_ADDRESS_FROM_URL, error_str);
      }

      for (addrinfo* link = result; link; link = link->ai_next)
      {
        sockaddr_in*      remote = reinterpret_cast<struct sockaddr_in*>(link->ai_addr);
        const void* const addr   = &remote->sin_addr;

        inet_ntop(link->ai_family, addr, ip_address, ADDRESS_BUFFER_SIZE);
      }

      freeaddrinfo(result);
    }
  };

  std::vector<std::string> split(std::string source, const std::string delimiters = "\r\n")
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
}  // namespace cs260

static bool comp_casei(const char a, const char b)
{
  return std::tolower(a) == std::tolower(b);
}

int main(int argc, const char* argv[])
{
  using namespace cs260;

  try
  {
    const char* const          url            = argv[1];
    const NetworkContextHandle network_ctx    = NetworkContext::create();
    const RequestURL           request_url    = RequestURL::create(url, PORT_STR);
    const NetworkFamily        network_family = NetworkFamily::LOCAL;
    const SocketHandle         socket         = network_ctx->createSocket(network_family, SocketType::TCP);
    const Address              address        = network_ctx->makeAddress(network_family, request_url.ip_address, PORT);




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
