#ifdef VRMOD_DEV_BUILD

#include "vrmod_log.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <mutex>

static FILE* g_logFile = nullptr;
static std::mutex g_logMutex;

void vrmod_log_init(const char* filepath) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_logFile) fclose(g_logFile);
    g_logFile = fopen(filepath, "a");
    if (g_logFile) {
        fprintf(g_logFile, "--- VRMOD log session started ---\n");
        fflush(g_logFile);
    }
}

void vrmod_log_close() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_logFile) {
        fprintf(g_logFile, "--- VRMOD log session ended ---\n");
        fclose(g_logFile);
        g_logFile = nullptr;
    }
}

void vrmod_log_write(const char* level, const char* fmt, ...) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (!g_logFile) return;

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    fprintf(g_logFile, "[%ld.%03ld] [%s] ", (long)ts.tv_sec, ts.tv_nsec / 1000000, level);

    va_list args;
    va_start(args, fmt);
    vfprintf(g_logFile, fmt, args);
    va_end(args);

    fprintf(g_logFile, "\n");
    fflush(g_logFile);
}

#endif
