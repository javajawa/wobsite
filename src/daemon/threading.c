#include "daemon/threading.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "logging.h"
#include "config.h"

static size_t poolsize = 0;
static struct thread_state * pool = NULL;
static char const * default_thread_name = "unknown_thread";

static pthread_mutex_t thread_state_mtx;

static const char* thread_type_names[] = {
	(char*)NULL,
	"main",
	"responder"
};

static size_t thead_type_counts[] = {
	0,
	0
};

char const * get_thread_type_name( enum thread_type type )
{
	return thread_type_names[ type ];
}

/**
 * Attempt to expand the allocated memory space which maintains
 * the list of active threads we have.
 * The current size is held in "poolsize" and is increased in
 * increments of 8.
 *
 * This function is not thread-safe
 */
int thread_pool_expand()
{
	poolsize += 8;
	pool = realloc( pool, sizeof( struct thread_state ) * poolsize );

	if ( pool == NULL )
	{
		return 1;
	}

	for ( size_t i = poolsize - 8; i < poolsize; ++i )
	{
		pool[i].type  = THREAD_ANY;
		pool[i].state = UNCREATED;
	}

	return 0;
}

/**
 * Creates the threadpool
 */
int thread_pool_init()
{
	pthread_mutex_lock(&thread_state_mtx);

	if ( poolsize )
	{
		errno = EBADE;

		pthread_mutex_unlock(&thread_state_mtx);

		return 1;
	}

	if ( thread_pool_expand() )
	{
		return 1;
	}

	pool[0].thread = pthread_self();
	pool[0].type   = THREAD_MAIN;

#ifdef _GNU_SOURCE
	pthread_getname_np( pool[0].thread, pool[0].name, 16 );
#else
	strcpy( pool[0].name, "wobsite" );
#endif

	pthread_mutex_unlock(&thread_state_mtx);

	return 0;
}

int thread_pool_destroy()
{
	pthread_mutex_lock(&thread_state_mtx);

	if ( !poolsize )
	{
		errno = EBADE;

		pthread_mutex_unlock(&thread_state_mtx);

		return -1;
	}

	// Only allow the main process to destroy the pool.
	if ( pthread_self() != pool[0].thread )
	{
		errno = EACCES;

		return -1;
	}

	// i = 0 will be the main thread, which is allowed to still
	// be running, so start in 1.
	for ( size_t i = 1; i < poolsize; ++i )
	{
		if ( pool[i].type != THREAD_ANY )
		{
			errno = EBUSY;

			pthread_mutex_unlock(&thread_state_mtx);

			return -1;
		}
	}

	free( pool );
	pool = NULL;
	poolsize = 0;

	pthread_mutex_unlock(&thread_state_mtx);

	return 0;
}

int get_current_thread_details( enum thread_type * type, enum state * state )
{
	pthread_t self = pthread_self();

	for ( size_t i = 0; i < poolsize; ++i )
	{
		if ( pool[i].type != 0 && pthread_equal( self, pool[i].thread ) )
		{
			if ( type )
			{
				*type  = pool[i].type;
			}
			if ( state )
			{
				*state = pool[i].state;
			}
			return 0;
		}
	}

	errno = ESRCH;

	return 1;
}

char const * get_current_thread_name()
{
	pthread_t self = pthread_self();

	for ( size_t i = 0; i < poolsize; ++i )
	{
		if ( pool[i].type != 0 && pthread_equal( self, pool[i].thread ) )
		{
			return pool[i].name;
		}
	}

	return default_thread_name;
}

size_t create_threads( enum thread_type type, size_t count, void *(*func) (void *), void * arg )
{
	size_t created = 0;
	size_t index = 0;

	if ( pool == NULL )
	{
		return 0;
	}

	// TODO: Lock the mutex here!

	for ( size_t i = 0; i < count; ++i )
	{
		while ( pool[index].type != THREAD_FREE )
		{
			if ( ++index == poolsize )
			{
				if ( thread_pool_expand() )
				{
					return created;
				}
			}
		}

		pool[index].type = type;
		// TODO: Error Handling
		snprintf( pool[index].name, 15, "%s-%lu", thread_type_names[ type ], ++thead_type_counts[ type ] );

		errfs( LOG_THREAD, INFO, "Creating %s thread %s", thread_type_names[ type ], pool[index].name );

		if ( pthread_create( &pool[index].thread, NULL, func, arg ) )
		{
			errf( LOG_THREAD, ALRT, "Error running thread %s",  pool[index].name );
		}
		else
		{
			++created;
		}

#ifdef _GNU_SOURCE
		// TODO: Error Handling
		pthread_setname_np( pool[index].thread, pool[index].name );
#endif
	}

	// TODO: Unlock mutex here

	return created;
}

size_t signal_threads( enum thread_type type, int signal )
{
	size_t count = 0;

	for ( size_t i = 0; i < poolsize; ++i )
	{
		if ( pool[i].type == THREAD_FREE )
		{
			continue;
		}

		if ( type == THREAD_ANY || pool[i].type == type )
		{
			errfs( LOG_THREAD, VERB, "Interrupting thread %s", pool[i].name );
			pthread_kill( pool[i].thread, signal );
			++count;
		}
	}

	return count;
}

int thread_join_group( enum thread_type type, void ** retval )
{
	size_t i, valid;
	int err;

	while ( 1 )
	{
		valid = 0;
		// Thread 0 can not be joined.
		for ( i = 1; i < poolsize; ++i )
		{
			if ( pool[i].type == THREAD_FREE )
			{
				continue;
			}

			if ( type == THREAD_ANY || pool[i].type == type )
			{
				++valid;

				if ( ( pool[i].state & ACCEPT ) == ACCEPT )
				{
					errfs( LOG_THREAD, INFO, "Interrupting thread %s", pool[i].name );
					pthread_kill( pool[i].thread, SIGINT );
					continue;
				}

				err = pthread_join( pool[i].thread, retval );

				if ( err == 0 )
				{
					errfs( LOG_THREAD, INFO, "Thread %s joined", pool[i].name );
					pool[i].type  = THREAD_FREE;
					pool[i].state = UNCREATED;

					return 0;
				}

				if ( err != EBUSY )
				{
					errfs( LOG_THREAD, ALRT, "Error trying to join thread %s: %s", pool[i].name, strerror( err ) );
					errno = 0;
				}
			}
		}

		if ( valid == 0 )
		{
			errno = ESRCH;
			return 1;
		}

		errfs( LOG_THREAD, INFO, "Waiting on %li threads", valid );
		nanosleep( &((struct timespec){ 0, 100000000LU }), NULL );
	}

	return 1;
}

int thread_join( struct thread_state * joined, void ** retval )
{
	int result;

	for ( size_t i = 1; i < poolsize; ++i )
	{
		if ( pool[i].type == THREAD_FREE )
		{
			continue;
		}

		if ( pool[i].state != JOINED )
		{
			continue;
		}

		result = pthread_join( pool[i].thread, retval );

		switch ( result )
		{
			case 0:
				if ( joined != NULL )
				{
					memcpy( joined, &pool[i], sizeof( struct thread_state ) );
				}

				pool[i].type = THREAD_FREE;

				return pool[i].thread;

			case EBUSY:
				continue;

			default:
				err( LOG_THREAD, ALRT, "Unable to (try)join a thread" );
				break;
		}
	}

	return 0;
}

int thread_done()
{
	pthread_t self = pthread_self();

	for ( size_t i = 1; i < poolsize; ++i )
	{
		if ( pool[i].type == THREAD_FREE )
		{
			continue;
		}

		if ( ! pthread_equal( self, pool[i].thread ) )
		{
			continue;
		}

		if ( pool[i].state == JOINED )
		{
			// TODO: Proper error code
			return 1;
		}

		pool[i].state = JOINED;

		return 0;
	}

	return ESRCH;
}
