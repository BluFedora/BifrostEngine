#include "bifrost/asset_io/bifrost_json_parser.hpp"

#define JParserCurrentChar(p) ((p)->mFile[(p)->mFilePtr])
#define JParserCurrentStr(p) &JParserCurrentChar((p))
#define JParserIncrement(p)                                                                   \
  {                                                                                           \
    (++(p)->mFilePtr);                                                                        \
    if ((p)->mFilePtr < (p)->mFileLen)                                                        \
      if (JParserCurrentChar((p)) == '\n' || JParserCurrentChar((p)) == '\r') ++(p)->mLineNo; \
  }
#define JParserIs(p, t) ((p)->mCurrentToken.mType == (t))
#define JParserSkipSpace(p) \
  while (isspace(JParserCurrentChar((p)))) JParserIncrement((p))
#define JParserIsKeyword(p) (JParserCurrentChar((p)) == '(' || JParserCurrentChar((p)) == ')' || JParserCurrentChar((p)) == '_' || isalpha(JParserCurrentChar((p))))
#define JParserIsDigit(p) (JParserCurrentChar((p)) == '+' || \
                           JParserCurrentChar((p)) == '-' || \
                           JParserCurrentChar((p)) == 'E' || \
                           JParserCurrentChar((p)) == 'e' || \
                           isdigit(JParserCurrentChar((p))))
#define JParserSkipDigits(p) \
  while (JParserIsDigit((p))) JParserIncrement((p))
#define JParserSkipIf(p, cond) \
  if ((cond)) JParserIncrement((p))
#define JParserSkipKeyword(p) \
  while (JParserIsKeyword((p))) JParserIncrement((p))
#define JParserSkipString(p)                                                 \
  {                                                                          \
    while (JParserCurrentChar((p)) && JParserCurrentChar((p)) != JSON_QUOTE) \
    {                                                                        \
      const char prev = JParserCurrentChar((p));                             \
      JParserIncrement((p));                                                 \
      if (prev == '\\')                                                      \
      {                                                                      \
        JParserIncrement((p));                                               \
      }                                                                      \
      else if (JParserCurrentChar((p)) == JSON_QUOTE)                        \
        break;                                                               \
    }                                                                        \
  }
#define JParserSetToken(p, t, v)                  \
  (p)->mCurrentToken.mType  = (JsonTokenType)(t); \
  (p)->mCurrentToken.mValue = (v)

namespace bifrost
{
  namespace
  {
    enum JsonTokenType : char
    {
      JSON_L_CURLY   = '{',
      JSON_R_CURLY   = '}',
      JSON_L_SQR_BOI = '[',
      JSON_R_SQR_BOI = ']',
      JSON_COMMA     = ',',
      JSON_QUOTE     = '"',
      JSON_COLON     = ':',
      JSON_TRUE      = 't',
      JSON_FALSE     = 'f',
      JSON_NULL      = 'n',
      JSON_NUMBER    = '#',
      JSON_EOF       = '!'
    };

    struct JsonToken
    {
      char*         mValue;
      JsonTokenType mType;
    };

    struct JsonParserImpl
    {
      size_t    mFileLen;
      char*     mFile;
      JsonToken mCurrentToken;
      size_t    mFilePtr;
      size_t    mLineNo;
    };

    static unsigned char escape_convert(const unsigned char c)
    {
      /* Not implemented on MinGW:
        "sorry, unimplemented: non-trivial designated initializers not supported"
  static const char escape[256] =
  {
    ['a']   = '\a',
    ['b']   = '\b',
    ['f']   = '\f',
    ['n']   = '\n',
    ['r']   = '\r',
    ['t']   = '\t',
    ['v']   = '\v',
    ['\\']  = '\\',
    ['\'']  = '\'',
    ['"']   = '\"',
    ['?']   = '\?'

    // ... all the rest are 0 ...
  };
  */
      switch (c)
      {
        case 'a': return '\a';
        case 'b': return '\b';
        case 'f': return '\f';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case 'v': return '\v';
        case '\\': return '\\';
        case '\'': return '\'';
        case '\"': return '\"';
        case '?': return '\?';
        default: return '\0';
      }
    }

    size_t CString_unescape(char* str)
    {
      const char* oldStr = str;
      char*       newStr = str;

      while (*oldStr)
      {
        unsigned char c = *(unsigned char*)(oldStr++);

        if (c == '\\')
        {
          c = (unsigned char)(oldStr[0]);
          ++oldStr;
          if (c == '\0') break;
          if (escape_convert(c)) c = escape_convert(c);
        }

        *(newStr++) = char(c);
      }

      (*newStr) = '\0';

      return (newStr - str);
    }

    void JParserGetNextToken(JsonParserImpl* parser)
    {
      if (parser->mFilePtr < parser->mFileLen)
        JParserSkipSpace(parser);

      if (parser->mFilePtr < parser->mFileLen)
      {
        if (JParserIsKeyword(parser))
        {
          JParserSetToken(parser, JParserCurrentChar(parser), nullptr);
          JParserSkipKeyword(parser);
        }
        else if (JParserIsDigit(parser))
        {
          JParserSetToken(parser, JSON_NUMBER, JParserCurrentStr(parser));
          JParserSkipDigits(parser);
          JParserSkipIf(parser, JParserCurrentChar(parser) == '.');
          JParserSkipDigits(parser);
          JParserSkipIf(parser, JParserCurrentChar(parser) == 'f');
        }
        else if (JParserCurrentChar(parser) == JSON_QUOTE)
        {
          JParserIncrement(parser);  // '"'
          JParserSetToken(parser, JSON_QUOTE, JParserCurrentStr(parser));
          JParserSkipString(parser);
          JParserCurrentChar(parser) = '\0';
          CString_unescape(parser->mCurrentToken.mValue);
          JParserIncrement(parser);  // '"' (really '\0')
        }
        else
        {
          JParserSetToken(parser, JParserCurrentChar(parser), nullptr);
          JParserIncrement(parser);
        }
      }
      else
      {
        JParserSetToken(parser, JSON_EOF, nullptr);
      }
    }

    bool JParserEat(JsonParserImpl* parser, JsonTokenType type, int optional)
    {
      if (JParserIs(parser, type))
      {
        JParserGetNextToken(parser);

        return true;
      }

      if (!optional)
      {
        /*
        Logger::print(DebugLevel::DBG_ERROR,
                      "JSON_PARSE_ERROR Line: %i, Expected a '%c', got a '%c' at character#[%i]\n",
                      static_cast<int>(parser->mLineNo),
                      type,
                      parser->mCurrentToken.mType,
                      static_cast<int>(parser->mFilePtr));
        */
        ++parser->mFilePtr;

        return false;
      }

      return optional;
    }

    JsonValue interpret(JsonParserImpl* parser)
    {
      JsonValue return_value;

      switch (parser->mCurrentToken.mType)
      {
        case JSON_L_CURLY:
        {
          if (!JParserEat(parser, JSON_L_CURLY, 0)) return return_value;

          return_value.set<object_t>();

          while (!JParserIs(parser, JSON_R_CURLY))
          {
            const char* key = parser->mCurrentToken.mValue;

            if (!JParserEat(parser, JSON_QUOTE, 0)) return return_value;
            if (!JParserEat(parser, JSON_COLON, 0)) return return_value;

            if (key)
              return_value[key] = interpret(parser);

            if (!JParserEat(parser, JSON_COMMA, 1)) return return_value;
          }

          if (!JParserEat(parser, JSON_R_CURLY, 0)) return return_value;
          break;
        }
        case JSON_L_SQR_BOI:
        {
          if (!JParserEat(parser, JSON_L_SQR_BOI, 0)) return return_value;

          return_value.set<array_t>();

          while (!JParserIs(parser, JSON_R_SQR_BOI))
          {
            return_value.push_back(interpret(parser));
            if (!JParserEat(parser, JSON_COMMA, 1)) return return_value;
          }

          if (!JParserEat(parser, JSON_R_SQR_BOI, 0)) return return_value;
          break;
        }
        case JSON_QUOTE:
        {
          if (parser->mCurrentToken.mValue)
          {
            return_value = string_t(parser->mCurrentToken.mValue);
          }

          if (!JParserEat(parser, JSON_QUOTE, 0))
          {
            return return_value;
          }
          break;
        }
        case JSON_NUMBER:
        {
          if (parser->mCurrentToken.mValue)
            return_value = std::atof(parser->mCurrentToken.mValue);
          if (!JParserEat(parser, JSON_NUMBER, 0)) return return_value;
          break;
        }
        case JSON_TRUE:
        {
          return_value = true;
          if (!JParserEat(parser, JSON_TRUE, 0)) return return_value;
          break;
        }
        case JSON_FALSE:
        {
          return_value = false;
          if (!JParserEat(parser, JSON_FALSE, 0)) return return_value;
          break;
        }
        case JSON_R_CURLY:
        case JSON_R_SQR_BOI:
        case JSON_COMMA:
        case JSON_COLON:
        case JSON_NULL:
        case JSON_EOF:
          if (!JParserEat(parser, parser->mCurrentToken.mType, 0)) return return_value;
      }

      return return_value;
    }
  }  // namespace

  JsonValue JsonParser::parse(const std::string& source_string)
  {
    char* buffer = new char[source_string.size() + 1];

    memcpy(buffer, source_string.c_str(), source_string.size());

    buffer[source_string.size()] = '\0';

    JsonParserImpl parser = {
     source_string.size(),
     buffer,
     {nullptr, JSON_EOF},
     0,
     1};

    JParserEat(&parser, JSON_EOF, 0);

    JsonValue val = interpret(&parser);
    delete[] buffer;
    return val;
  }

  std::string JsonParser::toString(const JsonValue& value, const bool pretty_print)
  {
    std::string return_value;
    value.toString(return_value, pretty_print, 2);
    return return_value;
  }
}  // namespace tide

#undef JParserCurrentChar
#undef JParserCurrentStr
#undef JParserIncrement
#undef JParserIs
#undef JParserSkipSpace
#undef JParserIsKeyword
#undef JParserIsDigit
#undef JParserSkipDigits
#undef JParserSkipIf
#undef JParserSkipKeyword
#undef JParserSkipString
#undef JParserSetToken
