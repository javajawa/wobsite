#include "logging.h"
#include "config.h"

#include "daemon/responder.h"
#include "daemon/threading.h"

#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>

#define STDIN_BUFFER 16

void signal_handler( int signal )
{
	errfs( LOG_THREAD, CRIT, "Received signal %d to %s", signal, get_current_thread_name() );
	(void)signal;
}

void main_loop();

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

	// Force detecting the timezone here to guarantee its consistent
	// across all the threads.
	tzset();

	if ( thread_pool_init() )
	{
		err( LOG_THREAD, CRIT, "Unable to initialise thread pool" );
		return 1;
	}

	socket_fd = socket( AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0 );

	if ( socket_fd == -1 )
	{
		err( LOG_NET, CRIT, "Failed to initialise socket" );
		thread_pool_destroy();
		return 1;
	}

	if ( bind( socket_fd, (struct sockaddr *)&bind_addr, sizeof(bind_addr) ) == -1 )
	{
		err( LOG_NET, CRIT, "Failed to open socket 127.0.0.2:8888" );
		thread_pool_destroy();
		return 1;
	}

	if ( listen( socket_fd, -1 ) )
	{
		err( LOG_NET, CRIT, "Error listening on socket 127.0.0.2:8888" );
		thread_pool_destroy();
		return 1;
	}

	errs( LOG_NET, INFO, "Socket 127.0.0.2:8888 initialized" );

	errs( LOG_THREAD, INFO, "Setting up signal handler" );

	signal_control.sa_handler = signal_handler;
	signal_control.sa_flags   = 0;
	sigemptyset( &signal_control.sa_mask );

	result = sigaction( SIGUSR1, &signal_control, NULL );
	result = sigaction( SIGHUP,  &signal_control, NULL );
	result = sigaction( SIGTERM, &signal_control, NULL );
	result = sigaction( SIGINT,  &signal_control, NULL );

	if ( result == -1 )
	{
		err( LOG_THREAD, CRIT, "Failed to set up signal handler" );
		result = close( socket_fd );

		if ( result == -1 )
		{
			err( LOG_NET, ALRT, "Failed to close socket too" );
		}

		result = thread_pool_destroy();

		if ( result == -1 )
		{
			err( LOG_THREAD, ALRT, "Failed to close thread pool too" );
		}

		return 1;
	}

	errs( LOG_THREAD, VERB, "Creating " STR(RESPONDER_THREADS) " responders" );
	// TODO: Error handling (if error, set state = STATE_ERROR?)
	create_threads( THREAD_RESPONDER, RESPONDER_THREADS, accept_loop, &socket_fd );

	errs( LOG_THREAD, WARN, "Beginning main loop" );
	main_loop();

	errs( LOG_THREAD, INFO, "Signalling responders to hangup" );
	signal_threads( THREAD_RESPONDER, SIGHUP );

	while ( 1 )
	{
		result = thread_join_group( THREAD_RESPONDER, NULL );

		if ( result == 0 )
		{
			errfs( LOG_THREAD, VERB, "Joined thread of type %s", get_thread_type_name( THREAD_RESPONDER ) );
		}
		else
		{
			if ( errno == ESRCH )
			{
				break;
			}
			errf( LOG_THREAD, ALRT, "Failed to join thread of type %s", get_thread_type_name( THREAD_RESPONDER ) );
		}
	}

	// TODO: Proper error handling
	result = thread_pool_destroy();

	if ( result == -1 )
	{
		err( LOG_THREAD, ALRT, "Could not empty thread pool" );
	}

	errs( LOG_NET, INFO, "Shutting down socket" );
	result = close( socket_fd );

	if ( result == -1 )
	{
		errs( LOG_NET, ALRT, "Error closing socket" );
	}

	return 0;
}


void main_loop( void )
{
	uint8_t stdin_valid = 1;
	ssize_t result;
	struct thread_state joined_thread;
	void * thread_result;
	char buffer[STDIN_BUFFER];

	while ( 1 )
	{
		while ( stdin_valid )
		{
			result = read( STDIN_FILENO, buffer, STDIN_BUFFER );

			if ( result == 0 )
			{
				return;
			}

			if ( result == -1 )
			{
				switch ( errno )
				{
					case 0:
					case EINTR:
						return;

					case EBADF:
						stdin_valid = 0;
						break;

					case EAGAIN:
						break;

					default:
						err( LOG_THREAD, WARN, "Error reading from standard input" );
						break;
				}
			}

			// Keep reading until all buffer is read
			// TODO: Actually read and throw away unused data.
			if ( result < STDIN_BUFFER )
			{
				break;
			}
		}

		if ( thread_join( &joined_thread, &thread_result ) )
		{
			errfs(
				LOG_THREAD, VERB,
				"Joined thread %s (%lu), result %p => %s",
				joined_thread.name, joined_thread.thread,
				thread_result, (char*)thread_result
			);
		}
	}
}
