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

#include "NetworkPlatform.hpp"

#include "NetworkError.hpp" /* NetworkError                        */
#include "Socket.hpp"       /* NetworkFamily, SocketShutdownAction */

#if defined(_WIN32)

#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
// Link against 'ws2_32' from the command line.
#elif defined(_MSC_VER)
#pragma comment(lib, "ws2_32.lib")  // Ws2_32.dll
#else
#error Unsupported compiler remove this error if you can link against 'ws2_32.lib'.
#endif

#else

#include <sys/ioctl.h>  // ioctl, FIONBIO
#include <unistd.h>     // close

#define MAGIC_VALID_CONTEXT 1

#endif

namespace detail
{
  bool createContext(NetworkContextImpl& out, unsigned char version_major, unsigned char version_minor)
  {
#if defined(_WIN32)
    const int wsa_startup_error = WSAStartup(MAKEWORD(version_major, version_minor), &out);

    if (wsa_startup_error != 0) { throw NetworkError(NetworkErrorCode::FAILED_TO_CREATE_CONTEXT); }

    if (LOBYTE(out.wVersion) != version_major || HIBYTE(out.wVersion) != version_minor)
    {
      const int wsa_cleanup_error = WSACleanup();

      if (wsa_cleanup_error == SOCKET_ERROR) { throw NetworkError(NetworkErrorCode::FAILED_TO_DESTROY_CONTEXT); }

      throw NetworkError(NetworkErrorCode::FAILED_TO_FIND_CORRECT_WSA_VERSION);
    }
#else
    out = MAGIC_VALID_CONTEXT;
    (void)version_major;
    (void)version_minor;
#endif

    return true;
  }

  void destroyContext(const NetworkContextImpl& ctx)
  {
#if defined(_WIN32)
    const int wsa_cleanup_error = WSACleanup();

    if (wsa_cleanup_error == SOCKET_ERROR)
    {
      throw NetworkError(NetworkErrorCode::FAILED_TO_DESTROY_CONTEXT);
    }
#else
    // A magic number set in 'createContext' just so that were a semi sure it was called.
    if (ctx != MAGIC_VALID_CONTEXT)
    {
      throw NetworkError(NetworkErrorCode::FAILED_TO_DESTROY_CONTEXT);
    }
#endif
  }

  void makeNonBlocking(SocketImpl socket)
  {
#if defined(_WIN32)
    u_long mode = 1;

    const int err = ioctlsocket(socket, FIONBIO, &mode);

    if (err != NO_ERROR)
    {
      throw NetworkError(APIFunction::FN_IO_CTL_SOCKET);
    }
#else
    char mode = 1;

    const int err = ioctl(socket, FIONBIO, &mode);

    if (err < 0)
    {
      throw NetworkError(APIFunction::FN_IO_CTL);
    }
#endif
  }

  void closeSocket(SocketImpl socket)
  {
#if defined(_WIN32)
    closesocket(socket);
#else
    close(socket);
#endif
  }

  NetworkFamilyImpl toNative(NetworkFamily family)
  {
    switch (family)
    {
      case NetworkFamily::LOCAL: return AF_UNIX; // NOT ACTUALLY SUPPORTED BY WINDOWS.
      case NetworkFamily::IPv4: return AF_INET;
      case NetworkFamily::IPv6: return AF_INET6;

#if defined(_WIN32)
      case NetworkFamily::BLUETOOTH: return AF_BTH;
#else
      case NetworkFamily::BLUETOOTH: return AF_BLUETOOTH;
#endif
      default: return -1;
    }
  }

  int toNative(SocketType socket_type)
  {
    switch (socket_type)
    {
      case SocketType::UDP: return SOCK_DGRAM;
      case SocketType::TCP: return SOCK_STREAM;
      default: return -1;
    }
  }

  int toNative(SocketShutdownAction socket_shutdown_action)
  {
    switch (socket_shutdown_action)
    {
#if defined(_WIN32)
      case SocketShutdownAction::RECEIVE: return SD_RECEIVE;
      case SocketShutdownAction::SEND: return SD_SEND;
      case SocketShutdownAction::RECEIVE_SEND: return SD_BOTH;
#else
      case SocketShutdownAction::RECEIVE: return SHUT_RD;
      case SocketShutdownAction::SEND: return SHUT_WR;
      case SocketShutdownAction::RECEIVE_SEND: return SHUT_RDWR;
#endif
      default: return -1;
    }
  }

  bool isWaiting(int error_code)
  {
#if defined(_WIN32)
    return error_code == WSAEWOULDBLOCK;
#else
    return (error_code == EAGAIN) || (error_code == EWOULDBLOCK);
#endif
  }

  bool isConnectionClosed(int error_code)
  {
#if defined(_WIN32)
    return error_code == WSAECONNRESET;
#else
    return error_code == ECONNREFUSED);
#endif
  }

  bool isAlreadyConnected(int error_code)
  {
#if defined(_WIN32)
    return error_code == WSAEISCONN;
#else
    return error_code == EISCONN;
#endif
  }

  int getLastError()
  {
#if defined(_WIN32)
    return WSAGetLastError();
#else
    return errno;
#endif
  }

  const char* errorToString(int error_code, APIFunction function)
  {
#if defined(_WIN32)
    switch (error_code)
    {
      case WSASYSNOTREADY:  // WSAStartup
        return "The underlying network subsystem is not ready for network communication.";
      case WSAVERNOTSUPPORTED:  // WSAStartup
        return "The version of Windows Sockets support requested is not provided by this particular Windows Sockets implementation.";
      case WSAEINPROGRESS:  // WSAStartup, closesocket
        return "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
      case WSAEPROCLIM:  // WSACleanup, recvfrom
        return "A limit on the number of tasks supported by the Windows Sockets implementation has been reached.";
      case WSAEFAULT:
        switch (function)
        {
          case APIFunction::FN_WSA_STARTUP: return "The lpWSAData parameter is not a valid pointer.";
          case APIFunction::FN_RECVFROM: return "The buffer pointed to by the buf or from parameters are not in the user address space, or the fromlen parameter is too small to accommodate the source address of the peer address.";
          case APIFunction::FN_INET_PTON: return "The pszAddrString or pAddrBuf parameters are NULL or are not part of the user address space.";
          default: return "WSAEFAULT";
        }
      case WSAEINTR:  // recvfrom, closesocket
        return "The (blocking) call was canceled through WSACancelBlockingCall.";
      case WSAEINVAL:
        switch (function)
        {
          case APIFunction::FN_RECVFROM: return "The socket has not been bound with bind, or an unknown flag was specified, or MSG_OOB was specified for a socket with SO_OOBINLINE enabled, or (for byte stream-style sockets only) len was zero or negative.";
          case APIFunction::FN_SOCKET: return "An invalid argument was supplied. This error is returned if the af parameter is set to AF_UNSPEC and the type and protocol parameter are unspecified.";
          default: return "WSAEINVAL";
        }
        // recvfrom
      case WSAEISCONN:
        return "The socket is connected. This function is not permitted with a connected socket, whether the socket is connection oriented or connectionless.";
        // recvfrom
      case WSAENETRESET:
        return "For a datagram socket, this error indicates that the time to live has expired.";
        // recvfrom
        // closesocket: "The descriptor is not a socket."
      case WSAENOTSOCK:
        return "The descriptor in the s parameter is not a socket.";
        // recvfrom
      case WSAEOPNOTSUPP:
        return "MSG_OOB was specified, but the socket is not stream-style such as type SOCK_STREAM, OOB data is not supported in the communication domain associated with this socket, or the socket is unidirectional and supports only send operations.";
        // recvfrom
      case WSAESHUTDOWN:
        return "The socket has been shut down; it is not possible to recvfrom on a socket after shutdown has been invoked with how set to SD_RECEIVE or SD_BOTH.";
        // recvfrom
        // closesocket: "The socket is marked as nonblocking, but the l_onoff member of the linger structure is set to nonzero and the l_linger member of the linger structure is set to a nonzero timeout value."
      case WSAEWOULDBLOCK:
        return "The socket is marked as nonblocking and the recvfrom operation would block.";
        // recvfrom
      case WSAEMSGSIZE:
        return "The message was too large to fit into the buffer pointed to by the buf parameter and was truncated.";
        // recvfrom
      case WSAETIMEDOUT:
        return "The connection has been dropped, because of a network failure or because the system on the other end went down without notice.";
        // recvfrom
      case WSAECONNRESET:
        return "The virtual circuit was reset by the remote side executing a hard or abortive close. The application should close the socket; it is no longer usable. On a UDP-datagram socket this error indicates a previous send operation resulted in an ICMP Port Unreachable message.";
        // WSACleanup, recvfrom, socket, closesocket
      case WSANOTINITIALISED:
        return "A successful WSAStartup call must occur before using this function.";
        // WSACleanup, recvfrom, socket, closesocket
      case WSAENETDOWN:
        return "The network subsystem has failed.";
        // socket
        // inet_pton: "The address family specified in the Family parameter is not supported. This error is returned if the Family parameter specified was not AF_INET or AF_INET6."
      case WSAEAFNOSUPPORT:
        return "The specified address family is not supported. For example, an application tried to create a socket for the AF_IRDA address family but an infrared adapter and device driver is not installed on the local computer.";
        // socket
      case WSAEMFILE:
        return "No more socket descriptors are available.";
        // socket
      case WSAEINVALIDPROVIDER:
        return "The service provider returned a version other than 2.2.";
        // socket
      case WSAEINVALIDPROCTABLE:
        return "The service provider returned an invalid or incomplete procedure table to the WSPStartup.";
        // socket
      case WSAENOBUFS:
        return "No buffer space is available. The socket cannot be created.";
        // socket
      case WSAEPROTONOSUPPORT:
        return "The specified protocol is not supported.";
        // socket
      case WSAEPROTOTYPE:
        return "The specified protocol is the wrong type for this socket.";
        // socket
      case WSAEPROVIDERFAILEDINIT:
        return "The service provider failed to initialize. This error is returned if a layered service provider (LSP) or namespace provider was improperly installed or the provider fails to operate correctly.";
        // socket
      case WSAESOCKTNOSUPPORT: return "The specified socket type is not supported in this address family.";

      default: return "Unknown error";
    }
#else
    // EWOULDBLOCK is the same as EAGAIN so use both at the same time.

    switch (error_code)
    {
      case ENOTCONN:
      {
        // TODO(Shareef): Make more descriptive.
        return "Socket Not Connected.";
      }
    }

    switch (function)
    {
      case APIFunction::FN_WSA_STARTUP: return "Error from startup";
      case APIFunction::FN_CLOSE_SOCKET: return "Error from closesocket";
      case APIFunction::FN_RECVFROM: return "Error from recv / recvfrom";
      case APIFunction::FN_SOCKET: return "Error from socket";
      case APIFunction::FN_INET_PTON: return "Error from inet_pton";
      case APIFunction::FN_SEND_TO: return "Error from sendto";
      case APIFunction::FN_BIND: return "Error from bind";
      case APIFunction::FN_CONNECT: return "Error from connect";
      case APIFunction::FN_IO_CTL_SOCKET: return "Error from io_ctl_socket";
    };

    return "Use Windows for a more descriptive error message";
#endif
  }
}  // namespace detail
