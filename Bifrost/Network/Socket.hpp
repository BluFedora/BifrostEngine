/******************************************************************************/
/*!
  @file   Socket.hpp
  @author Shareef Abdoul-Raheem
  @par    email: shareef.a\@digipen.edu
  @par    DigiPen login: shareef.a
  @par    Course: CS260
  @par    Assignment #2
  @date   11/04/2019
  @brief 
    This is the thin OOP abstaction over the Socket API.
*/
/******************************************************************************/

#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "Address.hpp"         /* Address    */
#include "NetworkPlatform.hpp" /* SocketImpl */

#include <memory> /* unique_ptr */

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

/*!
 * @brief 
 *  The type of network connection to make.
 */
enum class NetworkFamily
{
  LOCAL,
  IPv4,
  IPv6,
  BLUETOOTH,
};

/*!
 * @brief 
 *  The type of socket to make.
 */
enum class SocketType
{
  UDP,
  TCP,
};

/*!
 * @brief 
 *  For the "Socket::shutdown" function.
 */
enum class SocketShutdownAction
{
  RECEIVE,
  SEND,
  RECEIVE_SEND
};

/*!
 * @brief 
 *  The result from a 'Socket::receiveDataFrom' call.
 *  contains a bunch of bundled up data from 'recvfrom' / 'recv'.
 */
struct ReceiveResult final
{
  Address                  source_address      = {};                                                       //!< Where the message came from.
  detail::SocketLengthImpl source_address_size = static_cast<detail::SocketLengthImpl>(sizeof(sockaddr));  //!< The size of the 'ReceiveResult::source_address' field.
  char*                    received_bytes      = nullptr;                                                  //!< The bytes array that was written to.
  detail::BytesCountImpl   received_bytes_size = 0;                                                        //!< The number of bytes written to 'ReceiveResult::received_bytes' or '-1' if the non blocking call is still waiting, -2 if the connection was disconencted.
};

/*!
 * @brief 
 *  OOP wrapper around the Socket API.
 */
class Socket final
{
  friend class NetworkContext;

 private:
  SocketType         m_Type;    //!< Type of socket this is .
  detail::SocketImpl m_Socket;  //!< The handle for the underlying network system.
  bool               m_IsOpen;  //!< Whether or not this socket is currently open.

 public:
  Socket();

  // This class should not be copied or moved.
  Socket(const Socket&) = delete;
  Socket(Socket&&)      = delete;
  Socket& operator=(const Socket&) = delete;
  Socket& operator=(Socket&&) = delete;

  void                   bindTo(const Address& address) const;
  bool                   connectTo(const Address& address) const;
  void                   makeNonBlocking() const;
  detail::BytesCountImpl sendDataTo(const Address& address, const char* data, int data_size, SendToFlags::type flags = SendToFlags::NONE) const;
  ReceiveResult          receiveDataFrom(char* data, int data_size, ReceiveFromFlags::type flags = ReceiveFromFlags::NONE) const;
  void                   shutdown(SocketShutdownAction action) const;
  void                   close();
  ~Socket();
};

using SocketHandle = std::unique_ptr<Socket>;

#endif /* SOCKET_HPP */
