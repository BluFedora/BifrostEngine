/******************************************************************************/
/*!
  @file   NetworkPlatform.hpp
  @author Shareef Abdoul-Raheem
  @par    email: shareef.a\@digipen.edu
  @par    DigiPen login: shareef.a
  @par    Course: CS260
  @par    Assignment #2
  @date   11/04/2019
  @brief 
    This is where the cross-platform magic happens.
    All methods defined here work in a cross-platform mannor while providing
    a consistent API.
*/
/******************************************************************************/

#ifndef NETWORK_PLATFORM_HPP
#define NETWORK_PLATFORM_HPP

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

enum class SocketType;
enum class NetworkFamily;
enum class SocketShutdownAction;

namespace detail
{
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
  };

#if defined(_WIN32)
  using NetworkContextImpl = WSADATA;
  using SocketImpl         = SOCKET;
  using NetworkFamilyImpl  = int;
  using SocketLengthImpl   = int;
  using BytesCountImpl     = int;
#else
  using NetworkContextImpl = int;
  using SocketImpl         = int;
  using NetworkFamilyImpl  = sa_family_t;
  using SocketLengthImpl   = socklen_t;
  using BytesCountImpl     = ssize_t;

#define INVALID_SOCKET (int)-1
#define SOCKET_ERROR (int)-1
#endif

  bool              createContext(NetworkContextImpl& out, unsigned char version_major, unsigned char version_minor);
  void              destroyContext(const NetworkContextImpl& ctx);
  void              makeNonBlocking(SocketImpl socket);
  void              closeSocket(SocketImpl socket);
  NetworkFamilyImpl toNative(NetworkFamily family);
  int               toNative(SocketType socket_type);
  int               toNative(SocketShutdownAction socket_shutdown_action);
  bool              isWaiting(int error_code);
  bool              isConnectionClosed(int error_code);
  bool              isAlreadyConnected(int error_code);
  int               getLastError();
  const char*       errorToString(int error_code, APIFunction function);
}  // namespace detail

#endif /* NETWORK_PLATFORM_HPP */
