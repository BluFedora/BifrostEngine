/******************************************************************************/
/*!
  @file   NetworkContext.hpp
  @author Shareef Abdoul-Raheem
  @par    email: shareef.a\@digipen.edu
  @par    DigiPen login: shareef.a
  @par    Course: CS260
  @par    Assignment #2
  @date   11/04/2019
  @brief
    This is the main hub for using this network API.
    Any platform specific setup and destruction will be handled by this object.
*/
/******************************************************************************/

#ifndef NETWORK_CONTEXT_HPP
#define NETWORK_CONTEXT_HPP

#include "Socket.hpp" /* Address, unique_ptr, SocketHandle */

class NetworkContext;
using NetworkContextHandle = std::unique_ptr<NetworkContext>;

/*!
 * @brief 
 *  This is the main hub for using this network API.
 */
class NetworkContext final
{
 public:
  /*!
  * @brief 
  *   This is the function that should be used to create a NetworkContextHandle.
  * 
  * @param version_major 
  *   For WSA, should be 2 unless you want an older version.
  * 
  * @param version_minor 
  *   For WSA, should be 2 unless you want an older version.
  * 
  * @return NetworkContextHandle 
  *   The newly created handle.
  * 
  */
  static NetworkContextHandle create(unsigned char version_major = 2, unsigned char version_minor = 2);

 private:
  bool                       m_IsCreated;  //!< Whether or not this is an active context.
  detail::NetworkContextImpl m_WSAData;    //!< Internal platform specific handle.

 public:
  // This class should not be copied or moved.
  NetworkContext(const NetworkContext&) = delete;
  NetworkContext(NetworkContext&&)      = delete;
  NetworkContext& operator=(const NetworkContext&) = delete;
  NetworkContext& operator=(NetworkContext&&) = delete;

  /*!
   * @brief 
   *   Creates a Socket object
   * 
   * @param family 
   *  The type of connection to make.
   * 
   * @param type 
   *  The type of socket to create.
   * 
   * @param protocol 
   *  The protocol to be used if the socket type supports multiple protocols.
   * 
   * @return SocketHandle 
   *   The Socket object.
   */
  SocketHandle createSocket(NetworkFamily family, SocketType type, int protocol = 0) const;

  /*!
   * @brief Makes an Address object.
   * 
   * @param family 
   *  The type of connection to make.
   * 
   * @param address 
   *  The address in dotted-decimal notation.
   * 
   * @param port 
   *  The four digit port number to use.
   * 
   * @return Address 
   *  The Address object
   */
  Address makeAddress(NetworkFamily family, const char* address, unsigned short port) const;

  /*!
   * @brief 
   *  Closes the network context.
   *  Does not need to be explicitly classed since the destructor calls close.
   */
  void close();

  /*!
   * @brief 
   *  Destroy the Network Context object.
   *  calls "NetworkContext::close".
   */
  ~NetworkContext();  // NOLINT

 private:
  /*!
   * @brief 
   *   Construct a new Network Context object.
   */
  NetworkContext();
};

#include <string>

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
      std::printf("getaddrinfo(%s:%s): %s\n", host.c_str(), port, error_str);
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

#endif /* NETWORK_CONTEXT_HPP */
