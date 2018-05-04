/******************************************************************************
 * Copyright (c) 2015-2018 TP-Link Technologies CO.,LTD.
 *
 * 文件名称:		panorama_log.c
 * 版           本:	1.0
 * 摘           要:	log功能
 * 作           者:	wupimin<wupimin@tp-link.com.cn>
 * 创建时间:		2018-04-28
 ******************************************************************************/

#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include "panorama_log.h"

extern int gLogMask;

static const char *logtype_str(int type)
{
	switch (type) {
		case LOG_FATAL:  return "Fatal";
		case LOG_ERROR: return "Error";
		case LOG_WARN:  return "Warnning";
		case LOG_INFO:    return "Info";
		case LOG_DEBUG:    return "Debug";
	}
	return "Undefined";
}

void panorama_log(int type, const char *fn, int line, const char *fmt, ...)
{
	if (gLogMask & type)
	{
		char tbuf[20];
		time_t tp = time(NULL);
		va_list ap;
		va_start(ap, fmt);
		strftime(tbuf, 20, "%F %T", localtime(&tp));
		printf("[%s %s]-[%s, %d]: ", tbuf, logtype_str(type), fn, line);
		vprintf(fmt, ap);

		va_end(ap);
	}
}


