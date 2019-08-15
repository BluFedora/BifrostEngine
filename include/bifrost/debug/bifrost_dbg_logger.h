#if 0  /* TODO(Shareef): Logging Colors. */
// clang-format off
#define RESET                   "\033[0m"
#define BLACK                   "\033[30m"        /* Black */
#define RED                     "\x1B[31m"        /* Red */
#define GREEN                   "\033[32m"        /* Green */
#define YELLOW                  "\033[33m"        /* Yellow */
#define BLUE                    "\033[34m"        /* Blue */
#define MAGENTA                 "\033[35m"        /* Magenta */
#define CYAN                    "\033[36m"        /* Cyan */
#define WHITE                   "\033[37m"        /* White */
#define BOLDBLACK               "\033[1m\033[30m" /* Bold Black */
#define BOLDRED                 "\033[1m\033[31m" /* Bold Red */
#define BOLDGREEN               "\033[1m\033[32m" /* Bold Green */
#define BOLDYELLOW              "\033[1m\033[33m" /* Bold Yellow */
#define BOLDBLUE                "\033[1m\033[34m" /* Bold Blue */
#define BOLDMAGENTA             "\033[1m\033[35m" /* Bold Magenta */
#define BOLDCYAN                "\033[1m\033[36m" /* Bold Cyan */
#define BOLDWHITE               "\033[1m\033[37m" /* Bold White */
// clang-format on
#endif /* 0 */

#ifndef BIFROST_DBG_LOGGER_H
#define BIFROST_DBG_LOGGER_H

#include <stdarg.h> /* va_list, va_start, va_end */

#if __cplusplus
extern "C" {
#endif
typedef enum BifrostLoggerLevel_t
{
  BIFROST_LOGGER_LVL_VERBOSE,  // Normal logging
  BIFROST_LOGGER_LVL_WARNING,  // When the user does an action that is undeserirable but not neccesarily bad.
  BIFROST_LOGGER_LVL_ERROR,    // A recoverable error.
  BIFROST_LOGGER_LVL_FATAL,    // An unrecoverable error and the program must be shut down.
  BIFROST_LOGGER_LVL_PUSH,     // Meta Data Needed For The Callback (Editor graphical handling)
  BIFROST_LOGGER_LVL_POP,      // Meta Data Needed For The Callback (Editor graphical handling)

} BifrostLoggerLevel;

typedef struct BifrostDbgLogInfo_t
{
  BifrostLoggerLevel level;
  const char*        file;
  const char*        func;
  int                line;
  unsigned           indent_level;
  const char*        format;
  va_list            args;

} BifrostDbgLogInfo;

typedef struct IBifrostDbgLogger_t
{
  void* data;
  void (*callback)(void* data, const BifrostDbgLogInfo* info);

} IBifrostDbgLogger;

void bfLogger_init(const IBifrostDbgLogger* logger);
void bfLogPush_(const char* file, const char* func, int line, const char* format, ...);
void bfLogPrint_(BifrostLoggerLevel level, const char* file, const char* func, int line, const char* format, ...);
#if __cplusplus
void bfLogPop_(const char* file, const char* func, int line, unsigned amount = 1);
#else
void bfLogPop_(const char* file, const char* func, int line, unsigned amount);
#endif
void bfLogger_deinit(void);

#ifndef bfLogPush
#define bfLogPush(format, ...) bfLogPush_(__FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
#define bfLogPrint(format, ...) bfLogPrint_(BIFROST_LOGGER_LVL_VERBOSE, __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
#define bfLogWarn(format, ...) bfLogPrint_(BIFROST_LOGGER_LVL_WARNING, __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
#define bfLogError(format, ...) bfLogPrint_(BIFROST_LOGGER_LVL_ERROR, __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
#define bfLogFatal(format, ...) bfLogPrint_(BIFROST_LOGGER_LVL_FATAL, __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
#define bfLogPop(...) bfLogPop_(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#endif /* bfLogPush */

#if __cplusplus
}
#endif

#endif /* BIFROST_DBG_LOGGER_H */