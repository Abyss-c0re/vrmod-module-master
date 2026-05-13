#pragma once

#ifdef VRMOD_DEV_BUILD

void vrmod_log_init(const char* filepath);
void vrmod_log_close();
void vrmod_log_write(const char* level, const char* fmt, ...);

#define VRMOD_LOG_INIT(path)    vrmod_log_init(path)
#define VRMOD_LOG_CLOSE()       vrmod_log_close()
#define VRMOD_LOG_INFO(...)     vrmod_log_write("INFO",  __VA_ARGS__)
#define VRMOD_LOG_WARN(...)     vrmod_log_write("WARN",  __VA_ARGS__)
#define VRMOD_LOG_ERROR(...)    vrmod_log_write("ERROR", __VA_ARGS__)
#define VRMOD_LOG_DEBUG(...)    vrmod_log_write("DEBUG", __VA_ARGS__)

#else

#define VRMOD_LOG_INIT(path)    ((void)0)
#define VRMOD_LOG_CLOSE()       ((void)0)
#define VRMOD_LOG_INFO(...)     ((void)0)
#define VRMOD_LOG_WARN(...)     ((void)0)
#define VRMOD_LOG_ERROR(...)    ((void)0)
#define VRMOD_LOG_DEBUG(...)    ((void)0)

#endif
