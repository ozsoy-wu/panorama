#ifndef __PANORAMA_LOG_H__
#define __PANORAMA_LOG_H__

#include <stdarg.h>

typedef enum LOG_TYPE_E
{
	LOG_DEBUG = 0x1,
	LOG_INFO = 0x1<<1,
	LOG_WARN = 0x1<<2,
	LOG_ERROR = 0x1<<3,
	LOG_FATAL = 0x1<<4,
} LOG_TYPE;

void panorama_log(int type, const char *fn, int line, const char *fmt, ...);

#define Log(type, fmt, ...) panorama_log(type, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define PANORAMA_LOG(mask, level, fmt...) do {\
		if ((mask) & (level) break;\
		printf("%s, %d", __func__, __LINE__);\
		printf(fmt, __VA_ARGS__);\
	} while (0);

#endif // __PANORAMA_LOG_H__
