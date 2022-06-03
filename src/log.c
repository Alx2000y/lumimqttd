#include "log.h"

void _syslog(int priority, const char *format, ...)
{
    va_list args;
    if (config.verbosity > 0 && priority > config.verbosity)
        return;
    va_start(args, format);
    vsyslog(priority, format, args);
    va_end(args);
}
