/******************************************************************************/
/*!
 * @file   bifrost_dbg_logger.c
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#include "bifrost/debug/bifrost_dbg_logger.h"

#include <assert.h> /* assert */
#include <string.h> /* memset */

#if _WIN32
#include <Windows.h> /* GetStdHandle, GetConsoleScreenBufferInfo, SetConsoleTextAttribute */
#else
#include <stdio.h> /* sprintf, printf */
#endif

static IBifrostDbgLogger s_ILogger        = {NULL, NULL};
static unsigned          s_IndentLevel    = 0u;
static bfLoggerLevel     s_LoggerLevel    = BIFROST_LOGGER_LVL_VERBOSE;
static bfLogColorState   s_ColorState     = {BIFROST_LOGGER_COLOR_WHITE, BIFROST_LOGGER_COLOR_BLACK, 0x0};
static int               s_HasInitialized = 0;

static void callCallback(bfLoggerLevel level, const char* file, const char* func, int line, const char* format, va_list args);

void bfLogger_init(const IBifrostDbgLogger* logger)
{
  assert(!s_HasInitialized && "The logger subsystem was already initialized.");
  assert(logger && logger->callback && "A valid logger must be passed into 'bfLogger_init'");

  s_ILogger        = *logger;
  s_IndentLevel    = 0u;
  s_LoggerLevel    = BIFROST_LOGGER_LVL_VERBOSE;
  s_HasInitialized = 1;
}

void bfLogPush_(const char* file, const char* func, int line, bfFormatString(const char*) format, ...)
{
  assert(s_HasInitialized && "The logger subsystem was never initialized.");

  va_list args;
  va_start(args, format);
  callCallback(BIFROST_LOGGER_LVL_PUSH, file, func, line, format, args);
  va_end(args);
  ++s_IndentLevel;
}

void bfLogPrint_(bfLoggerLevel level, const char* file, const char* func, int line, bfFormatString(const char*) format, ...)
{
  assert(s_HasInitialized && "The logger subsystem was never initialized.");

  va_list args;
  va_start(args, format);
  callCallback(level, file, func, line, format, args);
  va_end(args);
}

void bfLogPop_(const char* file, const char* func, int line, unsigned amount)
{
  assert(s_HasInitialized && "The logger subsystem was never initialized.");
  assert(amount <= s_IndentLevel && "There were more pops than pushes performed.");

  va_list args;
  memset(&args, 0x0, sizeof(args));
  callCallback(BIFROST_LOGGER_LVL_POP, file, func, line, "", args);

  s_IndentLevel -= amount;
}

void bfLogger_deinit(void)
{
  assert(s_HasInitialized && "The logger subsystem was never initialized.");
  s_HasInitialized = 0;
}

bfLogColorState bfLogSetColor(bfLoggerColor fg_color, bfLoggerColor bg_color, unsigned int flags)
{
#if _WIN32
  static const WORD k_FgColorMap[] =
   {
    0x0,
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    FOREGROUND_RED | FOREGROUND_GREEN | 0x0,
    FOREGROUND_RED | 0x0 | FOREGROUND_BLUE,
    0x0 | FOREGROUND_GREEN | FOREGROUND_BLUE,
    FOREGROUND_RED,
    FOREGROUND_GREEN,
    FOREGROUND_BLUE,
   };

  static const WORD k_BgColorMap[] =
   {
    0x0,
    BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
    BACKGROUND_RED | BACKGROUND_GREEN | 0x0,
    BACKGROUND_RED | 0x0 | BACKGROUND_BLUE,
    0x0 | FOREGROUND_GREEN | BACKGROUND_BLUE,
    BACKGROUND_RED,
    BACKGROUND_GREEN,
    BACKGROUND_BLUE,
   };

  const WORD color = k_FgColorMap[fg_color] | k_BgColorMap[bg_color] |
                     FOREGROUND_INTENSITY * ((flags & BIFROST_LOGGER_COLOR_FG_BOLD) != 0) |
                     BACKGROUND_INTENSITY * ((flags & BIFROST_LOGGER_COLOR_BG_BOLD) != 0) |
                     COMMON_LVB_REVERSE_VIDEO * ((flags & BIFROST_LOGGER_COLOR_INVERT) != 0) |
                     COMMON_LVB_UNDERSCORE * ((flags & BIFROST_LOGGER_COLOR_UNDERLINE) != 0);

  const HANDLE h_console = GetStdHandle(STD_OUTPUT_HANDLE);

  /*
   To get the current attribs:
     CONSOLE_SCREEN_BUFFER_INFO console_info;
     GetConsoleScreenBufferInfo(h_console, &console_info);
     const WORD saved_attributes = console_info.wAttributes;
  */

  SetConsoleTextAttribute(h_console, color);
#else
  /* [http://www.lihaoyi.com/post/BuildyourownCommandLinewithANSIescapecodes.html#256-colors] */
  static const int s_ColorMap[] = {30, 37, 33, 35, 36, 31, 32, 34};

  const int bold_impl      = flags & BIFROST_LOGGER_COLOR_FG_BOLD ? 1 : 21;
  const int underline_impl = flags & BIFROST_LOGGER_COLOR_UNDERLINE ? 4 : 24;
  const int invert_impl    = flags & BIFROST_LOGGER_COLOR_INVERT ? 7 : 27;
  const int fg_color_impl  = s_ColorMap[fg_color];
  const int bg_color_impl  = s_ColorMap[bg_color] + 10;

  char      command[128];
  const int count = sprintf(command, "\033[%d;%d;%d;%d;%dm", bold_impl, underline_impl, invert_impl, fg_color_impl, bg_color_impl);
  command[count]  = '\0';

  printf("%s", command);
#endif

  const bfLogColorState old_state = s_ColorState;

  s_ColorState.fg_color = fg_color;
  s_ColorState.bg_color = bg_color;
  s_ColorState.flags    = flags;

  return old_state;
}

static void callCallback(bfLoggerLevel level, const char* file, const char* func, int line, const char* format, va_list args)
{
  if (s_ILogger.callback)
  {
    BifrostDbgLogInfo log_info;
    log_info.level        = level;
    log_info.file         = file;
    log_info.func         = func;
    log_info.line         = line;
    log_info.indent_level = s_IndentLevel;
    log_info.format       = format;

    s_ILogger.callback(s_ILogger.data, &log_info, args);
  }
}