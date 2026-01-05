#ifndef __LOG_HPP__
#define __LOG_HPP__

#include <Arduino.h>

/* ================== 日志总开关 ================== */
#define LOG_ENABLE   1     // 0 = 关闭所有日志，1 = 打开日志

/* ================== 日志等级 ================== */
typedef enum
{
    LOG_LEVEL_INFO = 0,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_DEBUG
} LogLevel;

/* ================== 接口函数 ================== */
#if LOG_ENABLE

void Log_Init(uint32_t baudrate);

void Log_Print(LogLevel level,
               const char* file,
               uint16_t line,
               const char* fmt, ...);

/* ✅ 纯 printf：不带标签，不自动换行 */
void Log_Printf(const char* fmt, ...);

/* ✅ 十六进制 dump：16 bytes/行 + ASCII，prefix 可为 NULL */
void Log_DumpHex(const void* data, size_t len, const char* prefix);

/* ✅ 打印 uint8_t 数组：一行输出，prefix 可为 NULL */
void Log_PrintArrayU8(const uint8_t* arr, size_t len, const char* prefix);

/* ---------- 宏封装（自动带文件名和行号） ---------- */
#define LOG_I(fmt, ...)  Log_Print(LOG_LEVEL_INFO,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_W(fmt, ...)  Log_Print(LOG_LEVEL_WARN,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_E(fmt, ...)  Log_Print(LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_D(fmt, ...)  Log_Print(LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

/* ✅ 不带标签的 printf 宏 */
#define LOG_P(fmt, ...)  Log_Printf(fmt, ##__VA_ARGS__)

/* ✅ dump hex 宏 */
#define LOG_HEX(buf, len)             Log_DumpHex((buf), (len), NULL)
#define LOG_HEX_P(prefix, buf, len)   Log_DumpHex((buf), (len), (prefix))

/* ✅ array 宏 */
#define LOG_ARR_U8(arr, len)          Log_PrintArrayU8((arr), (len), NULL)
#define LOG_ARR_U8_P(prefix, arr, len) Log_PrintArrayU8((arr), (len), (prefix))

#else

/* 关闭日志时，全部编译期消失 */
#define Log_Init(...)
#define LOG_I(...)
#define LOG_W(...)
#define LOG_E(...)
#define LOG_D(...)
#define LOG_P(...)
#define LOG_HEX(...)
#define LOG_HEX_P(...)
#define LOG_ARR_U8(...)
#define LOG_ARR_U8_P(...)

#endif /* LOG_ENABLE */

#endif /* __LOG_HPP__ */
