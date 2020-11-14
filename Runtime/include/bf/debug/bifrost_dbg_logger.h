/******************************************************************************/
/*!
 * @file   bifrost_dbg_logger.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#ifndef BIFROST_DBG_LOGGER_H
#define BIFROST_DBG_LOGGER_H

#include <stdarg.h> /* va_list, va_start, va_end */

#if __cplusplus
extern "C" {
#endif

#if _MSC_VER
/*
  Macro for having the compiler check for correct printf formatting.
 */
#define bfFormatString(t) __format_string t
#else
#define bfFormatString(t) t
#endif

typedef enum
{
  BIFROST_LOGGER_LVL_VERBOSE, /*!< Normal logging                                                                                           */
  BIFROST_LOGGER_LVL_WARNING, /*!< When the user does an action that is undesirable but not necessarily bad.                                */
  BIFROST_LOGGER_LVL_ERROR,   /*!< A recoverable error.                                                                                     */
  BIFROST_LOGGER_LVL_FATAL,   /*!< An unrecoverable error and the program must be shut down.                                                */
  BIFROST_LOGGER_LVL_PUSH,    /*!< Meta Data (ex: Editor graphical handling)                                                                */
  BIFROST_LOGGER_LVL_POP,     /*!< Meta Data (ex: Editor graphical handling) DO NOT USE the callback 'va_list' as it will be uninitialized. */

} bfLoggerLevel;

typedef enum
{
  /* Available Colors */
  BIFROST_LOGGER_COLOR_BLACK,
  BIFROST_LOGGER_COLOR_WHITE,
  BIFROST_LOGGER_COLOR_YELLOW,
  BIFROST_LOGGER_COLOR_MAGENTA,
  BIFROST_LOGGER_COLOR_CYAN,
  BIFROST_LOGGER_COLOR_RED,
  BIFROST_LOGGER_COLOR_GREEN,
  BIFROST_LOGGER_COLOR_BLUE,

  /* Color Flags */
  BIFROST_LOGGER_COLOR_FG_BOLD   = (1 << 0),
  BIFROST_LOGGER_COLOR_BG_BOLD   = (1 << 1),
  BIFROST_LOGGER_COLOR_UNDERLINE = (1 << 2),
  BIFROST_LOGGER_COLOR_INVERT    = (1 << 3),

} bfLoggerColor;

typedef struct
{
  bfLoggerLevel level;
  const char*   file;
  const char*   func;
  int           line;
  unsigned      indent_level;
  const char*   format;

} BifrostDbgLogInfo;

typedef struct
{
  void* data;
  void (*callback)(void* data, BifrostDbgLogInfo* info, va_list args);

} IBifrostDbgLogger;

void bfLogger_init(const IBifrostDbgLogger* logger);
void bfLogPush_(const char* file, const char* func, int line, bfFormatString(const char*) format, ...);
void bfLogPrint_(bfLoggerLevel level, const char* file, const char* func, int line, bfFormatString(const char*) format, ...);
#if __cplusplus
void bfLogPop_(const char* file, const char* func, int line, unsigned amount = 1);
#else
void bfLogPop_(const char* file, const char* func, int line, unsigned amount);
#endif
void bfLogger_deinit(void);

/* clang-format off */
#ifndef bfLogPush
  #define bfLogPush(format, ...)  bfLogPush_(__FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
  #define bfLogPrint(format, ...) bfLogPrint_(BIFROST_LOGGER_LVL_VERBOSE, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
  #define bfLogWarn(format, ...)  bfLogPrint_(BIFROST_LOGGER_LVL_WARNING, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
  #define bfLogError(format, ...) bfLogPrint_(BIFROST_LOGGER_LVL_ERROR, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
  #define bfLogFatal(format, ...) bfLogPrint_(BIFROST_LOGGER_LVL_FATAL, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
  #define bfLogPop(...)           bfLogPop_(__FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif /* bfLogPush */
/* clang-format on */

typedef struct
{
  bfLoggerColor fg_color;
  bfLoggerColor bg_color;
  unsigned int  flags;

} bfLogColorState;

/*! Returns the previous color state. */
bfLogColorState bfLogSetColor(bfLoggerColor fg_color, bfLoggerColor bg_color, unsigned int flags);

#if __cplusplus
}
#endif

#endif /* BIFROST_DBG_LOGGER_H */
