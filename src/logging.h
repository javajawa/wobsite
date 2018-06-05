#ifndef _WOBSITE_LOGGING_H
#define _WOBSITE_LOGGING_H

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

char const * get_thread_name();
char const * get_timestamp();
void strdump( unsigned char const * restrict, ssize_t );

#define STRINGIFY(x) #x
#define STR(x) STRINGIFY(x)

#define errfs(format,...) fprintf( stderr, "%s [" __FILE__ ":" STR(__LINE__) "] %s " format "\n", get_timestamp(), get_thread_name(), __VA_ARGS__ )
#define err(mess)        errfs( "%s: %s", mess, strerror( errno ) )
#define errs(mess)       errfs( "%s",     mess )
#define errf(format,...) errfs( format ": %s", __VA_ARGS__, strerror( errno ) )

#endif
