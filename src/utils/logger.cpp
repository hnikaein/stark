/**
 * @author Hassan Nikaein
 */

#include "logger.h"
#include <iostream>
#include <cstdarg>
#include <cstring>

#define LOGGER_FUNCTION(FUNC_NAME, LOG_LEVEL)       \
void Logger::FUNC_NAME(const char *format, ...) {\
    va_list args;\
    va_start(args, format);\
    if ((LOG_LEVEL) > this->log_level)\
        return;\
    log(formatString(format, args), LOG_LEVEL);\
    va_end(args);\
}\

Logger *logger;

Logger::Logger(int log_level) : log_level(static_cast<LogLevel>(log_level)) {}

LOGGER_FUNCTION(debugl4, DEBUGL4)

LOGGER_FUNCTION(debugl3, DEBUGL3)

LOGGER_FUNCTION(debugl2, DEBUGL2)

LOGGER_FUNCTION(debug, DEBUG)

LOGGER_FUNCTION(info, INFO)

LOGGER_FUNCTION(warn, WARN)

LOGGER_FUNCTION(error, ERROR)

LOGGER_FUNCTION(fatal, FATAL)

using namespace std;

void Logger::log(const string &s, LogLevel level) {
    if (level > this->log_level)
        return;
    time_t ctt = time(nullptr);
    char *time = asctime(localtime(&ctt));
    time[strlen(time) - 1] = '\0';
    mtx.lock();
    cout << time << ": " << s << "\n";
    mtx.unlock();
}

string Logger::formatString(const char *const format, va_list args) {
    char buffer[FORMAT_LENGTH];
    buffer[0] = 0;
    vsnprintf(buffer, FORMAT_LENGTH, format, args);
    return string(buffer);
}

string Logger::formatString(const char *const format, ...) {
    va_list args;
    va_start(args, format);
    string s = formatString(format, args);
    va_end(args);
    return s;
}

