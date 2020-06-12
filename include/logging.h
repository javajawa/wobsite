// SPDX-FileCopyrightText: 2020 Benedict Harcourt <ben.harcourt@harcourtprogramming.co.uk>
//
// SPDX-License-Identifier: BSD-2-Clause

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

#define NONE 0
#define CRIT 1
#define ALRT 2
#define WARN 3
#define INFO 4
#define VERB 5

#define errfs( log, level, format, ... ) if ( level <= log ) { fprintf( stderr, "%s [" __FILE__ ":" STR(__LINE__) "] %s " format "\n", get_timestamp(), get_current_thread_name(), __VA_ARGS__ ); }
// _Static_asset( level > NONE, "Log Level must be lower than NONE" );

#define err(  log, level, mess )        errfs( log, level, "%s: %s", mess, strerror( errno ) )
#define errs( log, level, mess )        errfs( log, level, "%s",     mess )
#define errf( log, level, format, ... ) errfs( log, level, format ": %s", __VA_ARGS__, strerror( errno ) )

#endif
