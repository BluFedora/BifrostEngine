/******************************************************************************/
/*!
 * @file   bf_dbg_logger.c
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   Basic logging interface for the engine, and for extra fun allows for
 *   changing the color of the console output (assuming the terminal supports it).
 * 
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019-2021
 */
/******************************************************************************/
#include "bf/bf_dbg_logger.h"

#include <assert.h> /* assert */
#include <string.h> /* memset */

#if _WIN32
#include <Windows.h> /* GetStdHandle, GetConsoleScreenBufferInfo, SetConsoleTextAttribute */
#else
#include <stdio.h> /* sprintf, printf */
#endif

static bfIDbgLogger*   s_ILoggerList = NULL;
static unsigned int    s_IndentLevel = 0u;
static bfLoggerLevel   s_LoggerLevel = BF_LOGGER_LVL_VERBOSE;
static bfLogColorState s_ColorState  = {BF_LOGGER_COLOR_WHITE, BF_LOGGER_COLOR_BLACK, 0x0};

static void callCallback(bfLoggerLevel level, const char* file, const char* func, int line, const char* format, va_list args);

void bfLogAdd(bfIDbgLogger* logger)
{
  logger->next  = s_ILoggerList;
  logger->prev  = NULL;
  s_ILoggerList = logger;
}

void bfLogRemove(bfIDbgLogger* logger)
{
  if (logger->prev)
  {
    logger->prev->next = logger->next;
  }
  else
  {
    s_ILoggerList = logger->next;
  }

  if (logger->next)
  {
    logger->next->prev = logger->prev;
  }
  else
  {
    // This is where a tail pointer would be handled.
  }
}

void bfLogPush_(const char* file, const char* func, int line, bfFormatString(const char*) format, ...)
{
  assert(s_HasInitialized && "The logger subsystem was never initialized.");

  va_list args;
  va_start(args, format);
  callCallback(BF_LOGGER_LVL_PUSH, file, func, line, format, args);
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
  callCallback(BF_LOGGER_LVL_POP, file, func, line, "", args);

  s_IndentLevel -= amount;
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
                     FOREGROUND_INTENSITY * ((flags & BF_LOGGER_COLOR_FG_BOLD) != 0) |
                     BACKGROUND_INTENSITY * ((flags & BF_LOGGER_COLOR_BG_BOLD) != 0) |
                     COMMON_LVB_REVERSE_VIDEO * ((flags & BF_LOGGER_COLOR_INVERT) != 0) |
                     COMMON_LVB_UNDERSCORE * ((flags & BF_LOGGER_COLOR_UNDERLINE) != 0);

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

  const int bold_impl      = flags & BF_LOGGER_COLOR_FG_BOLD ? 1 : 21;
  const int underline_impl = flags & BF_LOGGER_COLOR_UNDERLINE ? 4 : 24;
  const int invert_impl    = flags & BF_LOGGER_COLOR_INVERT ? 7 : 27;
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
  bfDbgLogInfo log_info;
  log_info.level        = level;
  log_info.file         = file;
  log_info.func         = func;
  log_info.line         = line;
  log_info.indent_level = s_IndentLevel;
  log_info.format       = format;

  bfIDbgLogger* logger = s_ILoggerList;

  while (logger)
  {
    if (logger->callback)
    {
      logger->callback(logger, &log_info, args);
    }

    logger = logger->next;
  }
}

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2019-2021 Shareef Abdoul-Raheem

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
/******************************************************************************/