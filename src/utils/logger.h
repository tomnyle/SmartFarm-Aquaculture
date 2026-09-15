#ifndef SMARTFARM_LOGGER_H
#define SMARTFARM_LOGGER_H

#include <Arduino.h>

class Logger {
public:
    static void info(const char* message);
    static void warn(const char* message);
    static void error(const char* message);
    static void infof(const char* prefix, const String& value);
};

#endif
