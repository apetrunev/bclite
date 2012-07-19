#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include "macros.h"

void
message(const char *fmt, ...)
{
	char fmtbuf[255];
	char buf[1024];
	va_list ap;
	int n;

	return_if_fail(fmt != NULL);
	
	va_start(ap, fmt);
	
	snprintf(fmtbuf, sizeof(fmtbuf), "%s" CRLF, fmt);
	
	n = vsnprintf(buf, sizeof(buf), fmtbuf, ap);
	
	if (n > 0)
		write(STDERR_FILENO, buf, n);		
	else {
		SHOULDNT_REACH();
	}
}
