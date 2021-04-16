/******************************************************************************/
/*!
 * @file   bf_net.hpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   Cross platform layer over the berkeley api on Win32 and Posix.
 *
 * @version 1.0.0
 * @date    2021-02-02
 *
 * @copyright Copyright (c) 2020-2021
 */
/******************************************************************************/
#ifndef BF_NET_HPP
#define BF_NET_HPP

#if defined(_WIN32)

// Windows.h has a list of defines for making it smaller in it at the top.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define VC_EXTRALEAN
#define WINDOWS_EXTRA_LEAN

#include <WS2tcpip.h> /* inet_pton,                                                                                */
#include <WinSock2.h> /* u_short, sockaddr, sockaddr_in, SOCKET, WSADATA, WSAStartup, socket, recvfrom, WSACleanup */

#else

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#endif

#include <cstdint>  // Sized in types
#include <string>   // string

namespace bfNet
{
#if defined(_WIN32)
  using NetworkContext    = WSADATA;
  using SocketHandle      = SOCKET;
  using NetworkFamilyImpl = int;
  using SocketLengthImpl  = int;
#else
  using NetworkContext    = int;
  using SocketHandle      = int;
  using NetworkFamilyImpl = sa_family_t;
  using SocketLengthImpl  = socklen_t;

#define INVALID_SOCKET (int)-1
#define SOCKET_ERROR (int)-1
#endif

  static constexpr SocketHandle k_InvalidSocketHandle = INVALID_SOCKET;

  using BytesCountType = std::int64_t;

  enum class NetworkFamily
  {
    LOCAL,
    IPv4,
    IPv6,
    BLUETOOTH,
  };

  enum class SocketType
  {
    UDP,
    TCP,
  };

  enum class SocketShutdownAction
  {
    RECEIVE,
    SEND,
    RECEIVE_SEND
  };

  enum class APIFunction
  {
    FN_WSA_STARTUP,
    FN_CLOSE_SOCKET,
    FN_RECVFROM,
    FN_RECV = FN_RECVFROM,
    FN_SOCKET,
    FN_INET_PTON,
    FN_SEND_TO,
    FN_BIND,
    FN_CONNECT,
    FN_IO_CTL_SOCKET,
    FN_IO_CTL = FN_IO_CTL_SOCKET,
    FN_SHUTDOWN,
    FN_WSA_IOCTL,
  };

  namespace SendToFlags
  {
    using type = int;

    constexpr type NONE = 0x00000000;
  }  // namespace SendToFlags

  namespace ReceiveFromFlags
  {
    using type = int;

    constexpr type NONE = 0x00000000;
  }  // namespace ReceiveFromFlags

  struct Error
  {
    int         code;  //!< Implementation defined error code, 0 if no error.
    APIFunction api;   //!< The function that caused the error, allows for more descriptive error messaged from 'bfNet::errorToString'.

    bool isSuccess() const { return code == 0; }
  };

  /*!
   * @brief 
   *  This class represents an address that can send and receive packets.
   */
  struct Address
  {
    sockaddr handle = {};  //!< The internal handle this class holds to the underlying network system.

    // A 'safe' cast to the 'sockaddr_in'.
    sockaddr_in& handleIn() { return *reinterpret_cast<sockaddr_in*>(&handle); }
  };

  enum ReceiveResultStatus
  {
    WAITING_ON_MESSAGE,
    CONTAINS_DATA,
    CONNECTION_CLOSED,
    CONTAINS_ERROR,
  };

  /*!
   * @brief 
   *  The result from a 'Socket::receiveDataFrom' call.
   *  contains a bunch of bundled up data from 'recvfrom' / 'recv'.
   */
  struct ReceiveResult
  {
    Address             source_address      = {};                                               //!< Where the message came from.
    SocketLengthImpl    source_address_size = static_cast<SocketLengthImpl>(sizeof(sockaddr));  //!< The size of the 'ReceiveResult::source_address' field.
    char*               received_bytes      = nullptr;                                          //!< The bytes array that was written to.
    BytesCountType      received_bytes_size = 0;                                                //!< The number of bytes written to 'ReceiveResult::received_bytes' or '-1' if the non blocking call is still waiting, -2 if the connection was disconencted.
    ReceiveResultStatus status              = ReceiveResultStatus::CONTAINS_DATA;               //!< `received_bytes` should only be read if `status` == CONTAINS_DATA.
    int                 error_code          = 0;                                                //!< Set to error code when result.received_bytes_size < 0, so may not be real error (aka `status` != CONTAINS_DATA).
  };

  struct Socket
  {
    SocketType   type   = SocketType::TCP;
    SocketHandle handle = k_InvalidSocketHandle;

    operator bool() const { return handle != k_InvalidSocketHandle; }

    Error          makeNonBlocking() const;
    Error          bindTo(const Address& address) const;
    Error          connectTo(const Address& address) const;  // Check Error with `isErrorAlreadyConnected`.
    BytesCountType sendDataTo(const Address& address, const char* data, int data_size, SendToFlags::type flags = SendToFlags::NONE) const;
    ReceiveResult  receiveDataFrom(char* data, int data_size, ReceiveFromFlags::type flags = ReceiveFromFlags::NONE) const;
    Error          shutdown(SocketShutdownAction action) const;
    void           close();

    // Special function for making IPC over a localhost TCP connection faster.
    Error win32EnableTCPLoopbackFastPath();
  };

  // Main API

  bool              Startup(NetworkContext* optional_output_ctx = nullptr);
  Socket            createSocket(NetworkFamily family, SocketType type, int protocol = 0);
  Address           makeAddress(NetworkFamily family, const char* address, unsigned short port, int* error_code = nullptr);
  NetworkFamilyImpl toNative(NetworkFamily family);
  int               toNative(SocketType socket_type);
  int               toNative(SocketShutdownAction socket_shutdown_action);
  int               getLastErrorCode();
  bool              isErrorWaiting(int error_code);
  bool              isErrorConnectionClosed(int error_code);
  bool              isErrorAlreadyConnected(int error_code);
  const char*       errorToString(Error err);
  bool              Shutdown();

  // Request Helper Classes

  struct RequestURL final
  {
    static constexpr std::size_t ADDRESS_BUFFER_SIZE = std::max(INET6_ADDRSTRLEN, INET_ADDRSTRLEN);

    static RequestURL create(std::string url, unsigned short port)
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
    RequestURL(std::string host_, std::string request_, unsigned short port_) :
      host{std::move(host_)},
      request{std::move(request_)},
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

      char port[5 + 1];
      std::snprintf(port, 5, "%u", port_);
      port[5] = '\0';

      addrinfo* result;
      const int err = getaddrinfo(host.c_str(), port, &hint, &result);

      if (err)
      {
        const char* error_str = gai_strerror(err);
        // std::printf("getaddrinfo(%s:%s): %s\n", host.c_str(), port, error_str);
        // TODO(SR): throw NetworkError(NetworkErrorCode::FAILED_TO_CREATE_ADDRESS_FROM_URL, error_str);
        return;
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

    const std::string& request() const
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
}  // namespace bfNet

#endif /* BF_NET_HPP */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2020-2021 Shareef Abdoul-Raheem

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
/******************************************************************************/
