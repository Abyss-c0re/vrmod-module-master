#include "logging.h"

#ifdef VRMOD_DEV
#include <cstdio>
#include <ctime>
#include <cstring>
#include <mutex>

static FILE* g_logFile = nullptr;
static std::mutex g_logMutex;

void vrmod_log_init(const char* filepath) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_logFile) fclose(g_logFile);
    g_logFile = fopen(filepath, "a");
    if (g_logFile) {
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);
        char timebuf[32];
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", t);
        fprintf(g_logFile, "\n=== VRMOD Dev Log Started [%s] ===\n", timebuf);
        fflush(g_logFile);
    }
}

void vrmod_log_shutdown() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_logFile) {
        fprintf(g_logFile, "=== VRMOD Dev Log Ended ===\n");
        fclose(g_logFile);
        g_logFile = nullptr;
    }
}

void vrmod_log_write(const char* level, const char* file, int line, const char* fmt, ...) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (!g_logFile) return;

    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%H:%M:%S", t);

    const char* fname = strrchr(file, '/');
    fname = fname ? fname + 1 : file;

    fprintf(g_logFile, "[%s][%s] %s:%d: ", timebuf, level, fname, line);

    va_list args;
    va_start(args, fmt);
    vfprintf(g_logFile, fmt, args);
    va_end(args);

    fprintf(g_logFile, "\n");
    fflush(g_logFile);
}
#endif
