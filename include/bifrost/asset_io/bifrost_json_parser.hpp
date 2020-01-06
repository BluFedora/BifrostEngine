#ifndef MJOLNIR_JSON_PARSER_HPP
#define MJOLNIR_JSON_PARSER_HPP

#include "bifrost_json_value.hpp"

namespace bifrost
{
  class JsonParser
  {
   public:
    static JsonValue   parse(const std::string& source_string);
    static std::string toString(const JsonValue& value, bool pretty_print = true);
  };
}  // namespace bifrost

#endif  // MJOLNIR_JSON_PARSER_HPP
