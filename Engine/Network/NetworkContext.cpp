/******************************************************************************/
/*!
  @file   NetworkContext.cpp
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

#include "NetworkContext.hpp"

#include "NetworkError.hpp"

#include <cstring>  /* memset */
#include <iostream> /* cerr */

NetworkContextHandle NetworkContext::create(unsigned char version_major, unsigned char version_minor)
{
  NetworkContextHandle context = NetworkContextHandle(new NetworkContext());
  context->m_IsCreated         = detail::createContext(context->m_WSAData, version_major, version_minor);
  return context;
}

NetworkContext::NetworkContext() :
  m_IsCreated{false},
  m_WSAData{}
{
}

SocketHandle NetworkContext::createSocket(NetworkFamily family, SocketType type, int protocol) const
{
  SocketHandle socket_handle = std::make_unique<Socket>();

  socket_handle->m_Type   = type;
  socket_handle->m_Socket = socket(detail::toNative(family), detail::toNative(type), protocol);

  if (socket_handle->m_Socket == INVALID_SOCKET)
  {
    throw NetworkError(detail::APIFunction::FN_SOCKET);
  }

  socket_handle->m_IsOpen = true;
  return socket_handle;
}

Address NetworkContext::makeAddress(NetworkFamily family, const char* address, unsigned short port) const
{
  const detail::NetworkFamilyImpl native_family = detail::toNative(family);
  Address                         address_obj   = {};

  std::memset(&address_obj.m_Handle, 0x0, sizeof(sockaddr_in));

  auto& addr_in = address_obj.handleIn();

  address_obj.handle().sa_family = native_family;
  addr_in.sin_port               = htons(port);

  if (address == nullptr)
  {
#if defined(_WIN32)
    addr_in.sin_addr.S_un.S_addr = INADDR_ANY;
#else
    addr_in.sin_addr.s_addr = INADDR_ANY;
#endif
  }
  else
  {
    const int error = inet_pton(native_family, address, &addr_in.sin_addr);

    if (error != 1)
    {
      std::printf("NetworkContext::makeAddress: %s\n", address);
      throw NetworkError(detail::APIFunction::FN_INET_PTON);
    }
  }

  return address_obj;
}

void NetworkContext::close()
{
  if (m_IsCreated)
  {
    detail::destroyContext(m_WSAData);
    m_IsCreated = false;
  }
}

NetworkContext::~NetworkContext()  // NOLINT
{
  try
  {
    close();
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << "\n";
  }
  catch (...)
  {
    std::cerr << "Unknown error while closing context.\n";
  }
}
