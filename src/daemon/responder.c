#include "responder.h"
#include "globals.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#define MAX_HEADER_LENGTH 60

#define HEADER_TOO_LONG_RESPONSE \
	"HTTP/1.1 400 Bad Request (Request Header too long)\r\n" \
	"Connection: close" \
	"Content-Type: text/plain\r\n" \
	"Content-Length: 43\r\n" \
	"\r\n" \
	"Request headers too large (max 8192 bytes)\n"

#define DEFAULT_RESPONSE \
	"HTTP/1.1 200 OK\r\n" \
	"Connection: close" \
	"Content-Type: text/plain\r\n" \
	"Content-Length: 4\r\n" \
	"\r\n" \
	"Hi.\n"

void* accept_loop( void * args )
{
	char header_buffer[MAX_HEADER_LENGTH];

	int sock = *((int*)args);
	int connection;

	ssize_t result;

	errs( "Accepting requests" );
	while ( 1 )
	{
		errfs( "Waiting for request on fd#%d", sock );

		connection = accept( sock, NULL, NULL );

		if ( connection == -1 )
		{
			err( "Error accepting connection" );
			continue;
		}

		errs( "Accepted connection" );
		result = read( connection, header_buffer, MAX_HEADER_LENGTH );

		if ( result == -1 )
		{
			err( "Error reading headers" );
			continue;
		}

		if ( result == MAX_HEADER_LENGTH )
		{
			// TODO: Error handling here
			ioctl( connection, FIONREAD, &result );

			errfs( "Request headers were too long (approximately %ld bytes)", MAX_HEADER_LENGTH + result );
			// TODO: Error handling here
			write( connection, HEADER_TOO_LONG_RESPONSE, sizeof( HEADER_TOO_LONG_RESPONSE ) - 1 );
			// TODO: Error handling here
			close( connection );

			continue;
		}

		errfs( "Read %ld bytes from client", result );

		// TODO: Error handling here
		write( connection, DEFAULT_RESPONSE, sizeof( DEFAULT_RESPONSE ) - 1 );

		result = read( connection, header_buffer, MAX_HEADER_LENGTH );

		if ( result < 0 )
		{
			err( "End of connection" );
		}
		else
		{
			header_buffer[result] = '\0';
			errfs( "End of connection: '%s'", header_buffer );
		}
	}

	return NULL;
}
