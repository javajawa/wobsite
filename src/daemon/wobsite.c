#define _GNU_SOURCE

#include "logging.h"

#include "daemon/globals.h"
#include "daemon/responder.h"
#include "daemon/threading.h"

#include <semaphore.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define RESPONDER_THREADS 3

static sem_t sem;
static volatile char closing = 0;

int main( void )
{
	int socket_fd;
	struct sockaddr_in bind_addr = {
		AF_INET,
		0xB822,
		{ 0x0200007FL },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	};

	if ( thread_pool_init() )
	{
		err( "Unable to initialise thread pool" );
		return 1;
	}

	if ( sem_init( &sem, 0, 0 ) == -1 )
	{
		err( "Error initialising semaphore" );
	}

	socket_fd = socket( AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0 );

	if ( socket_fd == -1 )
	{
		err( "Failed to initialise socket" );
		return 1;
	}

	if ( bind( socket_fd, &bind_addr, sizeof(bind_addr) ) == -1 )
	{
		err( "Failed to open socket 127.0.0.2:8888" );
		return 1;
	}

	if ( listen( socket_fd, -1 ) )
	{
		err( "Error listening on socket 127.0.0.2:8888" );
	}

	errs( "Socket 127.0.0.2:8888 initialized" );

	errs( "Creating " STR(RESPONDER_THREADS) " responders" );
	create_threads( "responder", RESPONDER_THREADS, accept_loop, &socket_fd );

	while ( state == 0 )
	{
		if ( sem_wait( &sem ) == -1 )
		{
			errfs( "Semaphore wait complete - %d", errno );
			if ( errno == EINTR )
			{
				break;
			}

			err( "Error waiting on semaphore" );
		}
	}

	errs( "Shutting down socket" );
	close( socket_fd );

	char thread_group[12] = "responder";

	errs( "Signalling all threads for shutdown" );
	signal_threads( "responder", SIGHUP );

	while ( 1 )
	{
		strcpy( thread_group, "responder" );
		thread_join( thread_group, NULL );

		errfs( "Joined thread of type %s", thread_group );
	}

	return 0;
}
