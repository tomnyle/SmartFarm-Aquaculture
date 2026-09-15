#include "logger.h"

// Small bilingual logger / Logger song ngữ đơn giản cho Serial.
void Logger::info(const char* message) {
    Serial.print("[INFO] ");
    Serial.println(message);
}

void Logger::warn(const char* message) {
    Serial.print("[WARN] ");
    Serial.println(message);
}

void Logger::error(const char* message) {
    Serial.print("[ERROR] ");
    Serial.println(message);
}

void Logger::infof(const char* prefix, const String& value) {
    Serial.print(prefix);
    Serial.println(value);
}
