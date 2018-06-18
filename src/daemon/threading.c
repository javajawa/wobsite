#include "threading.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "logging.h"
#include "config.h"

static size_t poolsize = 0;
static struct thread_state * pool = NULL;
static char const * default_thread_name = "unknown_thread";

static pthread_mutex_t thread_state_mtx;

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
		*pool[i].type = 0;
		pool[i].ended = 0;
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
	strcpy( pool[0].type, "main" );

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
		if ( *pool[i].type != 0 )
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

char const * get_thread_name()
{
	pthread_t self = pthread_self();

	for ( size_t i = 0; i < poolsize; ++i )
	{
		if ( *pool[i].type != 0 && pthread_equal( self, pool[i].thread ) )
		{
			return pool[i].name;
		}
	}

	return default_thread_name;
}

size_t create_threads( char const * name, size_t count, void *(*func) (void *), void * arg )
{
	size_t created = 0;
	size_t index = 0;

	if ( pool == NULL )
	{
		return 0;
	}

	for ( size_t i = 0; i < count; ++i )
	{
		while ( *pool[index].type != 0 )
		{
			if ( ++index == poolsize )
			{
				if ( thread_pool_expand() )
				{
					return created;
				}
			}
		}

		// TODO: Error Handling
		strncpy( pool[index].type, name, 11 );
		snprintf( pool[index].name, 15, "%s-%lu", name, i );

		errfs( LOG_THREAD, INFO, "Creating %s thread %s", pool[index].type, pool[index].name );

		if ( pthread_create( &pool[index].thread, NULL, func, arg ) )
		{
			errf( LOG_THREAD, ALRT, "Error making responder thread %lu", i );
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

	return created;
}

size_t signal_threads( char const * type, int signal )
{
	size_t count = 0;

	for ( size_t i = 0; i < poolsize; ++i )
	{
		if ( *pool[i].type == 0 )
		{
			continue;
		}

		if ( *type == 0 || strcmp( pool[i].type, type ) == 0 )
		{
			errfs( LOG_THREAD, VERB, "Interrupting thread %s", pool[i].name );
			pthread_kill( pool[i].thread, signal );
			++count;
		}
	}

	return count;
}

int thread_join_group( char * type, void ** retval )
{
	size_t i, valid;
	int err;

	while ( 1 )
	{
		valid = 0;
		// Thread 0 can not be joined.
		for ( i = 1; i < poolsize; ++i )
		{
			if ( *pool[i].type == 0 )
			{
				continue;
			}

			if ( *type == 0 || strcmp( type, pool[i].type ) == 0 )
			{
				++valid;

				if ( pool[i].ended == 0 )
				{
					pthread_kill( pool[i].thread, SIGINT );
					continue;
				}

				err = pthread_join( pool[i].thread, retval );

				if ( err == 0 )
				{
					errfs( LOG_THREAD, INFO, "Thread %s joined", pool[i].name );
					strcpy( type, pool[i].type );
					*pool[i].type = 0;

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
		if ( *pool[i].type == 0 )
		{
			continue;
		}

		if ( pool[i].ended == 0 )
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

				*pool[i].type = 0;

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
		if ( *pool[i].type == 0 )
		{
			continue;
		}

		if ( ! pthread_equal( self, pool[i].thread ) )
		{
			continue;
		}

		if ( pool[i].ended != 0 )
		{
			// TODO: Proper error code
			return 1;
		}

		pool[i].ended = 1;

		return 0;
	}

	return ESRCH;
}
