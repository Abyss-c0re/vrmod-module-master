#ifndef VRMOD_LOG_H
#define VRMOD_LOG_H

#ifdef VRMOD_DEV_BUILD

#include <cstdio>
#include <cstdarg>
#include <ctime>

namespace vrmod {

inline FILE* g_logFileHandle = nullptr;

inline void LogInit(const char* filepath) {
    g_logFileHandle = fopen(filepath, "a");
    if (g_logFileHandle) {
        time_t now = time(nullptr);
        fprintf(g_logFileHandle, "\n=== VRMOD session started: %s", ctime(&now));
        fflush(g_logFileHandle);
    }
}

inline void LogShutdown() {
    if (g_logFileHandle) {
        fprintf(g_logFileHandle, "=== VRMOD session ended ===\n");
        fclose(g_logFileHandle);
        g_logFileHandle = nullptr;
    }
}

inline void Log(const char* level, const char* fmt, ...) {
    if (!g_logFileHandle) return;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    fprintf(g_logFileHandle, "[%ld.%03ld][%s] ",
            ts.tv_sec, ts.tv_nsec / 1000000, level);
    va_list args;
    va_start(args, fmt);
    vfprintf(g_logFileHandle, fmt, args);
    va_end(args);
    fputc('\n', g_logFileHandle);
    fflush(g_logFileHandle);
}

} // namespace vrmod

#define VRMOD_LOG(fmt, ...)       vrmod::Log("INFO",  fmt, ##__VA_ARGS__)
#define VRMOD_LOG_WARN(fmt, ...)  vrmod::Log("WARN",  fmt, ##__VA_ARGS__)
#define VRMOD_LOG_ERROR(fmt, ...) vrmod::Log("ERROR", fmt, ##__VA_ARGS__)
#define VRMOD_LOG_INIT(path)      vrmod::LogInit(path)
#define VRMOD_LOG_SHUTDOWN()      vrmod::LogShutdown()

#else

#define VRMOD_LOG(...)       ((void)0)
#define VRMOD_LOG_WARN(...)  ((void)0)
#define VRMOD_LOG_ERROR(...) ((void)0)
#define VRMOD_LOG_INIT(path) ((void)0)
#define VRMOD_LOG_SHUTDOWN() ((void)0)

#endif
#endif
