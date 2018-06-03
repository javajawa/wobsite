#ifndef _WOBSITE_THREADING_H
#define _WOBSITE_THREADING_H

#include <stddef.h>

int thread_pool_init();
int thread_pool_destroy();

size_t create_threads( char const * type, size_t count, void *(*func) (void *), void * arg );
size_t signal_threads( char const * type, int signal );
int thread_join( char * type, void ** retval );

char const * get_thread_name();

#endif
