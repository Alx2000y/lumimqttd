#ifndef _SYSLLOG_H_
#define _SYSLLOG_H_
/* For syslog logging facility. */
#include <syslog.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cfg.h"

//extern char *logfile = "";
void _syslog(int priority, const char *format, ...);

#endif /* _SYSLLOG_H_ */