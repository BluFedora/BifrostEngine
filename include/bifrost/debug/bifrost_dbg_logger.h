/*!
 * @file   bifrost_dbg_logger.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019
 */
#ifndef BIFROST_DBG_LOGGER_H
#define BIFROST_DBG_LOGGER_H

#include <stdarg.h> /* va_list, va_start, va_end */

#if __cplusplus
extern "C" {
#endif
typedef enum BifrostLoggerLevel_t
{
  BIFROST_LOGGER_LVL_VERBOSE,  // Normal logging
  BIFROST_LOGGER_LVL_WARNING,  // When the user does an action that is undesirable but not necessarily bad.
  BIFROST_LOGGER_LVL_ERROR,    // A recoverable error.
  BIFROST_LOGGER_LVL_FATAL,    // An unrecoverable error and the program must be shut down.
  BIFROST_LOGGER_LVL_PUSH,     // Meta Data Needed For The Callback (Editor graphical handling)
  BIFROST_LOGGER_LVL_POP,      // Meta Data Needed For The Callback (Editor graphical handling) WARNING: DO NOT USE 'info->args' as it will be uninitialized.

} BifrostLoggerLevel;

typedef enum BifrostLoggerColor_t
{
  BIFROST_LOGGER_COLOR_BLACK,
  BIFROST_LOGGER_COLOR_WHITE,
  BIFROST_LOGGER_COLOR_YELLOW,
  BIFROST_LOGGER_COLOR_MAGENTA,
  BIFROST_LOGGER_COLOR_CYAN,
  BIFROST_LOGGER_COLOR_RED,
  BIFROST_LOGGER_COLOR_GREEN,
  BIFROST_LOGGER_COLOR_BLUE,

  BIFROST_LOGGER_COLOR_FG_BOLD   = (1 << 0),
  BIFROST_LOGGER_COLOR_BG_BOLD   = (1 << 1),
  BIFROST_LOGGER_COLOR_UNDERLINE = (1 << 2),
  BIFROST_LOGGER_COLOR_INVERT    = (1 << 3),

} BifrostLoggerColor;

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

// Main API
void bfLogger_init(const IBifrostDbgLogger* logger);
void bfLogPush_(const char* file, const char* func, int line, const char* format, ...);
void bfLogPrint_(BifrostLoggerLevel level, const char* file, const char* func, int line, /*__format_string*/ const char* format, ...);
#if __cplusplus
void bfLogPop_(const char* file, const char* func, int line, unsigned amount = 1);
#else
void bfLogPop_(const char* file, const char* func, int line, unsigned amount);
#endif
void bfLogger_deinit(void);

// clang-format off
#ifndef bfLogPush
#define bfLogPush(format, ...)  bfLogPush_(__FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
#define bfLogPrint(format, ...) bfLogPrint_(BIFROST_LOGGER_LVL_VERBOSE, __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
#define bfLogWarn(format, ...)  bfLogPrint_(BIFROST_LOGGER_LVL_WARNING, __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
#define bfLogError(format, ...) bfLogPrint_(BIFROST_LOGGER_LVL_ERROR,   __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
#define bfLogFatal(format, ...) bfLogPrint_(BIFROST_LOGGER_LVL_FATAL,   __FILE__, __FUNCTION__, __LINE__, format, __VA_ARGS__)
#define bfLogPop(...)           bfLogPop_(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#endif /* bfLogPush */
// clang-format on

// Helpers for Logging callbacks
typedef struct
{
  BifrostLoggerColor fg_color;
  BifrostLoggerColor bg_color;
  unsigned int       flags;

} bfLogColorState;

// Returns the previous color state.
bfLogColorState bfLogSetColor(BifrostLoggerColor fg_color, BifrostLoggerColor bg_color, unsigned int flags);

#if __cplusplus
}
#endif

#endif /* BIFROST_DBG_LOGGER_H */
