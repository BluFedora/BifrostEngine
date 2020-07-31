/******************************************************************************/
/*!
  @file   NetworkError.hpp
  @author Shareef Abdoul-Raheem
  @par    email: shareef.a\@digipen.edu
  @par    DigiPen login: shareef.a
  @par    Course: CS260
  @par    Assignment #2
  @date   11/04/2019
  @brief  
    The main (read: only) error exception class.
    This will be throw for any error that can happen.
*/
/******************************************************************************/

#ifndef NETWORK_ERROR_HPP
#define NETWORK_ERROR_HPP

#include "NetworkPlatform.hpp" /* APIFunction, errorToString, getLastError */
#include <stdexcept>           /* runtime_error                            */
#include <string>              /* to_string                                */

/*!
 * @brief 
 *  Custom API error codes.
 */
enum class NetworkErrorCode
{
  FAILED_TO_CREATE_CONTEXT,
  FAILED_TO_FIND_CORRECT_WSA_VERSION,
  FAILED_TO_CREATE_ADDRESS_FROM_URL,
  FAILED_TO_DESTROY_CONTEXT,
};

/*!
 * @brief 
 *  This will be throw for any error that can happen from this network library.
 */
class NetworkError final : public std::runtime_error
{
private:
  static const char* errorCodeToString(NetworkErrorCode error_code)
  {
    switch (error_code)
    {
      case NetworkErrorCode::FAILED_TO_CREATE_CONTEXT: return "Failed to create the underlying network context.";
      case NetworkErrorCode::FAILED_TO_FIND_CORRECT_WSA_VERSION: return "Failed to find the correct version of the WSA network context.";
      case NetworkErrorCode::FAILED_TO_DESTROY_CONTEXT: return "Failed to destroy the network context.";
      default: return "Unknown Error";
    }
  }

public:
  explicit NetworkError(NetworkErrorCode error_code) :
    runtime_error(errorCodeToString(error_code))
  {
  }

  explicit NetworkError(NetworkErrorCode /*error_code*/, const char* custom_message) :
    runtime_error(custom_message)
  {
  }

  explicit NetworkError(detail::APIFunction function) :
    runtime_error(errorToString(detail::getLastError(), function) + std::string("(") + std::to_string(detail::getLastError()) + ")")
  {
  }
};

#endif /* NETWORK_ERROR_HPP */
