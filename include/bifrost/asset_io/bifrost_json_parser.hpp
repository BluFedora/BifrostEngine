#ifndef MJOLNIR_JSON_PARSER_HPP
#define MJOLNIR_JSON_PARSER_HPP

#include "bifrost_json_value.hpp"

namespace bifrost
{
  class JsonParser
  {
   public:
    [[deprecated("New Json Utilities should be used instead")]]
    static JsonValue   parse(const String& source_string);
    [[deprecated("New Json Utilities should be used instead")]]
    static std::string toString(const JsonValue& value, bool pretty_print = true);
  };
}  // namespace bifrost

#endif  // MJOLNIR_JSON_PARSER_HPP
