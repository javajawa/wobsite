#ifndef _WOBSITE_THREADING_H
#define _WOBSITE_THREADING_H

#include <stddef.h>
#include <pthread.h>
#include <signal.h>

#include "daemon/state.h"

enum thread_type
{
	THREAD_ANY       = 0,
	THREAD_FREE      = 0,
	THREAD_MAIN      = 1,
	THREAD_RESPONDER = 2
};

struct thread_state
{
	pthread_t           thread;
	enum thread_type    type;
	enum state          state;
	char                name[16];
};

int thread_pool_init();
int thread_pool_destroy();

size_t create_threads( enum thread_type type, size_t count, void *(*func) (void *), void * arg );
size_t signal_threads( enum thread_type type, int signal );

int get_current_thread_details( enum thread_type *, enum state * );
char const * get_current_thread_name();
char const * get_thread_type_name( enum thread_type type );

size_t set_thread_group_state( enum thread_type type, enum state state );

int thread_done();
int thread_join_group( enum thread_type type, void ** retval );
int thread_join( struct thread_state * joined, void ** retval );

#endif
