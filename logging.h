#ifndef _WOBSITE_LOGGING_H
#define _WOBSITE_LOGGING_H

#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "threading.h"

#define STRINGIFY(x) #x
#define STR(x) STRINGIFY(x)

#define errfs(format,...) fprintf( stderr, "[" __FILE__ ":" STR(__LINE__) "] %s " format "\n", get_thread_name(), __VA_ARGS__ )
#define err(mess)        errfs( "%s: %s", mess, strerror( errno ) )
#define errs(mess)       errfs( "%s",     mess )
#define errf(format,...) errfs( format ": %s", __VA_ARGS__, strerror( errno ) )

#endif
