#include "bifrost/debug/bifrost_dbg_logger.h"

#include <assert.h> /* assert */

static IBifrostDbgLogger  s_ILogger;
static unsigned           s_IndentLevel;
static BifrostLoggerLevel s_LoggerLevel;

static void callCallback(BifrostLoggerLevel level, const char* file, const char* func, int line, const char* format, va_list args);

void bfLogger_init(const IBifrostDbgLogger* logger)
{
  assert(logger && "A valid logger must be passed into 'bfLogger_init'");
  assert(logger->callback && "A valid logger must be passed into 'bfLogger_init'");

  s_ILogger     = *logger;
  s_IndentLevel = 0u;
  s_LoggerLevel = BIFROST_LOGGER_LVL_VERBOSE;
}

void bfLogPush_(const char* file, const char* func, int line, const char* format, ...)
{
  va_list args;
  va_start(args, format);
  callCallback(BIFROST_LOGGER_LVL_PUSH, file, func, line, format, args);
  va_end(args);
  ++s_IndentLevel;
}

void bfLogPrint_(BifrostLoggerLevel level, const char* file, const char* func, int line, const char* format, ...)
{
  va_list args;
  va_start(args, format);
  callCallback(level, file, func, line, format, args);
  va_end(args);
}

void bfLogPop_(const char* file, const char* func, int line, unsigned amount)
{
  assert(amount <= s_IndentLevel && "There were more pops than pushes performed.");

  va_list args;
  va_start(args, amount);
  callCallback(BIFROST_LOGGER_LVL_POP, file, func, line, "", args);
  va_end(args);

  s_IndentLevel -= amount;
}

void bfLogger_deinit(void)
{
}

static void callCallback(BifrostLoggerLevel level, const char* file, const char* func, int line, const char* format, va_list args)
{
  BifrostDbgLogInfo log_info;
  log_info.level        = level;
  log_info.file         = file;
  log_info.func         = func;
  log_info.line         = line;
  log_info.indent_level = s_IndentLevel;
  log_info.format       = format;
  log_info.args         = args;

  s_ILogger.callback(s_ILogger.data, &log_info);
}
