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

#endif /* NETWORK_CONTEXT_HPP */
