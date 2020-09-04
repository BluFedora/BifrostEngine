/******************************************************************************/
/*!
  @file   Address.hpp
  @author Shareef Abdoul-Raheem
  @par    email: shareef.a\@digipen.edu
  @par    DigiPen login: shareef.a
  @par    Course: CS260
  @par    Assignment #2
  @date   11/04/2019
  @brief
    Represents an address that can send and receive data from sockets.
*/
/******************************************************************************/

#ifndef ADDRESS_HPP
#define ADDRESS_HPP

#include "NetworkPlatform.hpp" /* sockaddr, sockaddr_in */

/*!
 * @brief 
 *  This class represents a an address that can send and receive packets.
 */
class Address final
{
  friend class NetworkContext;

private:
  sockaddr m_Handle = {}; //!< The internal handle this class holds to the underlying network system.

public:
  // This class is safely copy, move, and default construct/destruct-able.
  Address()                          = default;
  Address(const Address&)            = default;
  Address(Address&&)                 = default;
  Address& operator=(const Address&) = default;
  Address& operator=(Address&&)      = default;
  ~Address()                         = default;

  // A 'safe' cast to the 'sockaddr_in'.
  sockaddr_in&    handleIn() { return *reinterpret_cast<sockaddr_in*>(&m_Handle); }
  const sockaddr& handle() const { return m_Handle; }
  sockaddr&       handle() { return m_Handle; }
};

#endif /* ADDRESS_HPP */
