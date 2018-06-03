#ifndef _WOBSITE_THREADING_H
#define _WOBSITE_THREADING_H

#include <stddef.h>
#include <pthread.h>

struct thread_state
{
	pthread_t thread;
	char name[16];
	char type[12];
};

int thread_pool_init();
int thread_pool_destroy();

size_t create_threads( char const * type, size_t count, void *(*func) (void *), void * arg );
size_t signal_threads( char const * type, int signal );
int thread_join_group( char * type, void ** retval );
int thread_join( struct thread_state * joined, void ** retval );

char const * get_thread_name();

#endif
