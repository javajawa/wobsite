#include "responder.h"
#include "globals.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#define MAX_HEADER_LENGTH 512

#define HEADER_TOO_LONG_CONTENT "Request headers too large (max " STR(MAX_HEADER_LENGTH) " bytes)\n"

#define HEADER_TOO_LONG_RESPONSE \
	"HTTP/1.1 400 Bad Request (Request Header too long)\r\n" \
	"Connection: close\r\n" \
	"Content-Type: text/plain\r\n" \
	"Content-Length: " STR( sizeof( HEADER_TOO_LONG_CONTENT ) ) "\r\n" \
	"\r\n" \
	HEADER_TOO_LONG_CONTENT

#define DEFAULT_RESPONSE \
	"HTTP/1.1 200 OK\r\n" \
	"Connection: close\r\n" \
	"Content-Type: text/plain\r\n" \
	"Content-Length: 4\r\n" \
	"\r\n" \
	"Hi.\n"

#define DEFAULT_RESPONSE_LEGNTH sizeof( DEFAULT_RESPONSE ) - 1

void* accept_loop( void * args )
{
	char header_buffer[MAX_HEADER_LENGTH];

	int sock = *((int*)args);
	int connection;

	ssize_t result;

	struct timeval timeout;
	fd_set descriptors;

	errs( "Accepting requests" );
	while ( state == 0 )
	{
		FD_ZERO( &descriptors );
		FD_SET( sock, &descriptors );
		timeout.tv_sec = 3;

		result = select( sock + 1, &descriptors, NULL, NULL, &timeout );

		if ( result == -1 )
		{
			err( "Failed in select() on listening socket" );
			continue;
		}

		if ( result == 0 )
		{
			continue;
		}

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

		result = write( connection, DEFAULT_RESPONSE, DEFAULT_RESPONSE_LEGNTH );

		if ( result < 0 )
		{
			err( "Error writing response to client" );
		}
		else if ( result != sizeof( DEFAULT_RESPONSE ) - 1 )
		{
			errfs( "Error writing response to client: only wrote %ld of %ld bytes.", result, DEFAULT_RESPONSE_LEGNTH );
		}

		result = close( connection );

		if ( result == 0 )
		{
			errs( "End of connection (Success)" );
		}
		else
		{
			err( "End of connection" );
		}
	}

	return NULL;
}
