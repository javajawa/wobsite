#include "logging.h"

#include "daemon/globals.h"
#include "daemon/responder.h"
#include "daemon/threading.h"

#include <semaphore.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>

#define RESPONDER_THREADS 3
#define STDIN_BUFFER 16

static sem_t sem;

void signal_handler( int signal )
{
	(void)signal;
}

int main( void )
{
	int socket_fd;
	int result;
	struct sockaddr_in bind_addr = {
		AF_INET,
		0xB822,
		{ 0x0200007FL },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	};
	struct sigaction signal_control = { NULL };
	struct timespec poll_wait = { 0, 399999999 };
	char buffer[STDIN_BUFFER];

	// Force detecting the timezone here to guarantee its consistent
	// across all the threads.
	tzset();

	if ( thread_pool_init() )
	{
		err( "Unable to initialise thread pool" );
		return 1;
	}

	if ( sem_init( &sem, 0, 0 ) == -1 )
	{
		err( "Error initialising semaphore" );
		thread_pool_destroy();
		return 1;
	}

	socket_fd = socket( AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0 );

	if ( socket_fd == -1 )
	{
		err( "Failed to initialise socket" );
		thread_pool_destroy();
		return 1;
	}

	if ( bind( socket_fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr) ) == -1 )
	{
		err( "Failed to open socket 127.0.0.2:8888" );
		thread_pool_destroy();
		return 1;
	}

	if ( listen( socket_fd, -1 ) )
	{
		err( "Error listening on socket 127.0.0.2:8888" );
		thread_pool_destroy();
		return 1;
	}

	errs( "Socket 127.0.0.2:8888 initialized" );

	signal_control.sa_handler = signal_handler;
	signal_control.sa_flags   = 0;
	sigemptyset( &signal_control.sa_mask );

	result = sigaction( SIGUSR1, &signal_control, NULL );
	result = sigaction( SIGHUP, &signal_control, NULL );
	result = sigaction( SIGTERM, &signal_control, NULL );
	result = sigaction( SIGINT, &signal_control, NULL );

	if ( result == -1 )
	{
		err( "Failed to set up signal handler" );
		result = close( socket_fd );

		if ( result == -1 )
		{
			err( "Failed to close socket too" );
		}

		result = thread_pool_destroy();

		if ( result == -1 )
		{
			err( "Failed to close thread pool too" );
		}

		return 1;
	}

	errs( "Creating " STR(RESPONDER_THREADS) " responders" );
	create_threads( "responder", RESPONDER_THREADS, accept_loop, &socket_fd );

	while ( state == 0 )
	{
		while ( 1 )
		{
			result = read( STDIN_FILENO, buffer, STDIN_BUFFER );

			if ( result == 0 )
			{
				state = 1;
				break;
			}

			if ( result == -1 )
			{
				switch ( errno )
				{
					case 0:
						state = 1;
						break;

					case EBADF:
						break;

					case EAGAIN:
						break;

					case EINTR:
						state = 1;
						break;

					default:
						err( "Error reading from standard input" );
						break;
				}
			}

			if ( result < 4 )
			{
				break;
			}
		}

		if ( sem_timedwait( &sem, &poll_wait ) == -1 )
		{
			if ( errno == EINTR )
			{
				break;
			}

			if ( errno == ETIMEDOUT )
			{
				continue;
			}

			err( "Error waiting on semaphore" );
		}
	}

	errs( "Switching state to shutdown" );
	state = 1;

	errs( "Shutting down socket" );
	result = close( socket_fd );

	if ( result == -1 )
	{
		errs( "Error closing socket" );
	}

	char thread_group[12] = "responder";

	while ( 1 )
	{
		strcpy( thread_group, "responder" );
		result = thread_join( thread_group, NULL );

		if ( result == 0 )
		{
			errfs( "Joined thread of type %s", thread_group );
		}
		else
		{
			if ( errno == ESRCH )
			{
				break;
			}
			errf( "Failed to join thread of type %s", thread_group );
		}
	}

	// TODO: Proper error handling
	result = thread_pool_destroy();

	if ( result == -1 )
	{
		err( "Could not empty thread pool" );
	}

	return 0;
}
