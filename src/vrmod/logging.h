#pragma once

#ifdef VRMOD_DEV
#include <cstdarg>

void vrmod_log_init(const char* filepath);
void vrmod_log_shutdown();
void vrmod_log_write(const char* level, const char* file, int line, const char* fmt, ...);

#define VRMOD_LOG_DEBUG(fmt, ...) vrmod_log_write("DEBUG", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define VRMOD_LOG_INFO(fmt, ...)  vrmod_log_write("INFO",  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define VRMOD_LOG_ERROR(fmt, ...) vrmod_log_write("ERROR", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define vrmod_log_init(path) ((void)0)
#define vrmod_log_shutdown() ((void)0)
#define VRMOD_LOG_DEBUG(fmt, ...) ((void)0)
#define VRMOD_LOG_INFO(fmt, ...)  ((void)0)
#define VRMOD_LOG_ERROR(fmt, ...) ((void)0)
#endif
