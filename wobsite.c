#define _GNU_SOURCE

#include "logging.h"
#include "threading.h"
#include "responder.h"

#include <fcgiapp.h>
#include <semaphore.h>
#include <signal.h>

#define RESPONDER_THREADS 3

static sem_t sem;
static volatile char closing = 0;

static void term_handler() // int sig, siginfo_t data, void * unused )
{
	errs( "Exit signal received, commencing shutdown" );
	closing = 1;
	sem_post( &sem );
}

int set_handlers( void )
{
	struct sigaction sa;

	sigemptyset( &sa.sa_mask );
	sa.sa_sigaction = &term_handler;
	sa.sa_flags     = 0;

	if ( sigaction( SIGTERM, &sa, NULL ) == -1 )
	{
		err( "Error adding TERM signal handler" );
		return 1;
	}

	if ( sigaction( SIGHUP, &sa, NULL ) == -1 )
	{
		err( "Error adding HUP signal handler" );
		return 1;
	}

	return 0;
}

int main( void )
{
	int socket;
	char const * bind = ":8888";
	if ( sem_init( &sem, 0, 0 ) == -1 )
	{
		err( "Error initialising semaphor" );
	}

	if ( FCGX_Init() == -1 )
	{
		err( "Failed to initialise FCGI" );
		return 1;
	}

	socket = FCGX_OpenSocket( bind, 256 );

	if ( socket == -1 )
	{
		errf( "Failed to open socket %s", bind );
		return 1;
	}
	errfs( "FCGI Socket %s initialized", bind );

	thread_pool_init();

	errs( "Creating " STR(RESPONDER_THREADS) " responders" );
	create_threads( "responder", RESPONDER_THREADS, accept_loop, &socket );

	if ( set_handlers() )
	{
		return 1;
	}

	while ( closing == 0 )
	{
		if ( sem_wait( &sem ) == -1 )
		{
			errfs( "Semaphor wait complete - %d", errno );
			if ( errno == EINTR )
			{
				break;
			}

			err( "Error waiting on semaphor" );
		}
	}

	errs( "Shutting down FCGI socket" );
	FCGX_ShutdownPending();

	char thread_group[12] = "responder";

	errs( "Signalling all threads for shutdown" );
	signal_threads( "responder", SIGHUP );

	while ( 1 )
	{
		strcpy( thread_group, "responder" );
		thread_join( thread_group, NULL );

		errfs( "Joined thread of type %s", thread_group );
	}

	/*for ( size_t thread = 0; thread < RESPONDER_THREADS; ++thread )
	{
		errfs( "Signalling %s", responders[thread].name );
		pthread_kill( responders[thread].thread, SIGHUP );
		//pthread_join( responders[thread].thread, NULL );
	}*/

	return 0;
}
