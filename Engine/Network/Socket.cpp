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

#include "Socket.hpp"

#include "Address.hpp"      /* Address      */
#include "NetworkError.hpp" /* NetworkError */

Socket::Socket() :
  m_Type{},
  m_Socket{},
  m_IsOpen{false}
{
}

void Socket::bindTo(const Address& address) const
{
  const int err = bind(m_Socket, &address.handle(), static_cast<detail::SocketLengthImpl>(sizeof(sockaddr)));

  if (err != 0)
  {
    throw NetworkError(detail::APIFunction::FN_BIND);
  }
}

bool Socket::connectTo(const Address& address) const
{
  const int err = connect(m_Socket, &address.handle(), static_cast<detail::SocketLengthImpl>(sizeof(sockaddr)));

  if (err == SOCKET_ERROR)
  {
    const int error_code = detail::getLastError();

    //throw NetworkError(detail::APIFunction::FN_CONNECT);
    return detail::isAlreadyConnected(error_code);
  }

  return true;
}

void Socket::makeNonBlocking() const
{
  detail::makeNonBlocking(m_Socket);
}

detail::BytesCountImpl Socket::sendDataTo(const Address& address, const char* data, int data_size, SendToFlags::type flags) const
{
  const detail::BytesCountImpl num_bytes_sent = sendto(
   m_Socket,
   data,
   data_size,
   flags,
   &address.handle(),
   static_cast<detail::SocketLengthImpl>(sizeof(sockaddr)));

  if (num_bytes_sent < 0)
  {
    throw NetworkError(detail::APIFunction::FN_SEND_TO);
  }

  return num_bytes_sent;
}

ReceiveResult Socket::receiveDataFrom(char* data, int data_size, ReceiveFromFlags::type flags) const
{
  ReceiveResult result;

  result.received_bytes = data;

  if (m_Type == SocketType::UDP)
  {
    result.received_bytes_size = recvfrom(
     m_Socket,
     result.received_bytes,
     data_size,
     flags,
     &result.source_address.handle(),
     &result.source_address_size);
  }
  else
  {
    result.received_bytes_size = recv(m_Socket, data, data_size, 0);
  }

  if (result.received_bytes_size < 0)
  {
    const int error_code = detail::getLastError();

    if (!detail::isWaiting(error_code))
    {
      if (detail::isConnectionClosed(error_code))
      {
        result.received_bytes_size = -2;
      }
      else
      {
        throw NetworkError(detail::APIFunction::FN_RECVFROM);
      }
    }
  }

  return result;
}

void Socket::shutdown(SocketShutdownAction action) const
{
  ::shutdown(m_Socket, detail::toNative(action));
}

void Socket::close()
{
  if (m_IsOpen)
  {
    detail::closeSocket(m_Socket);
    m_IsOpen = false;
  }
}

Socket::~Socket()
{
  close();
}
