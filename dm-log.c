/*
   log.c : log functions
*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "dm-idev.h"

#define		LOGHEAD			"iMon"
#define		LOG_BUF_LEN		256
#define		LOG_FILE		"log.latest"

char _log_buf[LOG_BUF_LEN];
char *_log_end = _log_buf + LOG_BUF_LEN;

void dm_log(IDEV *idevp, char *format, ...)
{
	va_list ap;
	int count = LOG_BUF_LEN;
	char *head_str;
	FILE *_log;

	if (idevp == NULL)
		head_str = LOGHEAD;
	else
		head_str = idevp->name;

	{
		struct tm *p;
		time_t m_t;
		m_t = time(NULL);
		p = localtime(&m_t);

		count -= snprintf(_log_end - count, count, 
				"%4d-%02d-%02d %02d:%02d:%02d - ", 1900+p->tm_year, 
				p->tm_mon+1, p->tm_mday, p->tm_hour, 
				p->tm_min, p->tm_sec);
	}
	count -= snprintf(_log_end - count, count, "[%s] ", head_str);
		
	va_start(ap, format);
	count -= vsnprintf(_log_end - count, count, format, ap);
	va_end(ap);
	count -= snprintf(_log_end - count, count, "\n");

	printf("%s", _log_buf);
	_log = fopen(LOG_FILE, "a");
	if (_log == NULL) {
		printf("log file %s open failed.\n", LOG_FILE);
		return;
	}
	fprintf(_log, "%s", _log_buf);
	fclose(_log);
}
