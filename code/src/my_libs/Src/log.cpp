#include "log.hpp"
#include <stdarg.h>
#include <stdio.h>

#if LOG_ENABLE

#define LOG_BUF_SIZE 128

static const char* level_str[] = {
    "INFO", "WARN", "ERROR", "DEBUG"
};

void Log_Init(uint32_t baudrate)
{
    Serial.begin(baudrate);
    while (!Serial);
}

void Log_Print(LogLevel level,
               const char* file,
               uint16_t line,
               const char* fmt, ...)
{
    char buf[LOG_BUF_SIZE];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    Serial.print('[');
    Serial.print(level_str[level]);
    Serial.print("][");
    Serial.print(file);
    Serial.print(':');
    Serial.print(line);
    Serial.print("] ");

    Serial.println(buf);
}

/* ✅ 1) 纯 printf：不带标签 */
void Log_Printf(const char* fmt, ...)
{
    char buf[LOG_BUF_SIZE];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    Serial.print(buf);
}

/* ✅ 2) 十六进制 dump：16 bytes 一行，带偏移 */
void Log_DumpHex(const void* data, size_t len, const char* prefix)
{
    const uint8_t* p = (const uint8_t*)data;
    if (prefix && prefix[0]) {
        Serial.println(prefix);
    }

    for (size_t i = 0; i < len; i += 16) {
        // offset
        Serial.print("0x");
        Serial.print((unsigned)(i), HEX);
        Serial.print(": ");

        // hex bytes
        for (size_t j = 0; j < 16; j++) {
            if (i + j < len) {
                uint8_t b = p[i + j];
                if (b < 0x10) Serial.print('0');
                Serial.print(b, HEX);
                Serial.print(' ');
            } else {
                Serial.print("   ");
            }
        }

        // ascii
        Serial.print(" |");
        for (size_t j = 0; j < 16 && (i + j) < len; j++) {
            uint8_t c = p[i + j];
            Serial.print((c >= 32 && c <= 126) ? (char)c : '.');
        }
        Serial.println('|');
    }
}

/* ✅ 3) 打印 uint8_t 数组（一行输出，默认十六进制） */
void Log_PrintArrayU8(const uint8_t* arr, size_t len, const char* prefix)
{
    if (prefix && prefix[0]) {
        Serial.print(prefix);
        Serial.print(": ");
    }

    Serial.print('[');
    for (size_t i = 0; i < len; i++) {
        if (i) Serial.print(", ");
        Serial.print("0x");
        if (arr[i] < 0x10) Serial.print('0');
        Serial.print(arr[i], HEX);
    }
    Serial.println(']');
}

#endif /* LOG_ENABLE */
