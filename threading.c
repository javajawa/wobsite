#define _GNU_SOURCE

#include "threading.h"

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "logging.h"

struct thread_state
{
	pthread_t thread;
	char name[16];
	char type[12];
};

static size_t poolsize = 0;
static struct thread_state * pool = NULL;
static char const * default_thread_name = "unknown_thread";

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
	}

	return 0;
}

void thread_pool_init()
{
	if ( thread_pool_expand() )
	{
		err( "Failed to allocate thread pool" );
		exit( 1 );
	}

	pool[0].thread = pthread_self();
	pthread_getname_np( pool[0].thread, pool[0].name, 16 );
}

char const * get_thread_name()
{
	pthread_t self = pthread_self();

	for ( size_t i = 0; i < poolsize; ++i )
	{
		if ( pthread_equal( self, pool[i].thread ) )
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

		if ( pthread_create( &pool[index].thread, NULL, func, arg ) )
		{
			errf( "Error making responder thread %lu", i );
		}
		else
		{
			++created;
		}

		// TODO: Error Handling
		pthread_setname_np( pool[index].thread, pool[index].name );
	}

	return created;
}

size_t signal_threads( char const * type, int signal )
{
	size_t count;

	for ( size_t i = 0; i < poolsize; ++i )
	{
		if ( *pool[i].type == 0 )
		{
			continue;
		}

		if ( *type == 0 || strcmp( pool[i].type, type ) == 0 )
		{
			errfs( "Interrupting thread %s", pool[i].name );
			pthread_kill( pool[i].thread, signal );
			++count;
		}
	}

	return count;
}

int thread_join( char * type, void ** retval )
{
	size_t i, valid;
	int err;

	while ( 1 )
	{
		valid = 0;
		for ( i = 0; i < poolsize; ++i )
		{
			if ( *pool[i].type == 0 )
			{
				continue;
			}

			if ( *type == 0 || strcmp( type, pool[i].type ) == 0 )
			{
				++valid;

				err = pthread_tryjoin_np( pool[i].thread, retval );

				if ( err == 0 )
				{
					strcpy( type, pool[i].type );
					*pool[i].type = 0;

					return 0;
				}

				if ( err != EBUSY )
				{
					errfs( "Error trying to join thread %s: %s", pool[i].name, strerror( err ) );
					errno = 0;
				}
			}
		}

		if ( valid == 0 )
		{
			errno = ESRCH;
			return 1;
		}

		errfs( "Before Sleep, value threads = %li", valid );
		//                                999999999.
		nanosleep( &((struct timespec){ 1, 10000000LU }), NULL );
		errs( "After Sleep" );
	}

	return 1;
}
