#include "responder.h"
#include "globals.h"
#include "http/request.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#define NO_ACTIVE_CONNECTION -1
#define READ_TIMEOUT_SEC 3
#define CONN_TIMEOUT_SEC 1
#define MAX_HEADER_LENGTH 512

#define HEADER_TOO_LONG_CONTENT "Request headers too large (max " STR(MAX_HEADER_LENGTH) " bytes)\n"


#define HEADER_TOO_LONG_RESPONSE \
	"HTTP/1.1 400 Bad Request (Request Header too long)\r\n" \
	"Content-Type: text/plain\r\n" \
	"Connection: close\r\n" \
	"Content-Length: " STR( sizeof( HEADER_TOO_LONG_CONTENT ) ) "\r\n" \
	"\r\n" \
	HEADER_TOO_LONG_CONTENT

#define DEFAULT_RESPONSE \
	"HTTP/1.1 200 OK\r\n" \
	"Connection: keep-alive\r\n" \
	"Keep-Alive: timeout=" STR( READ_TIMEOUT_SEC ) ", max=64\r\n" \
	"Content-Type: text/plain\r\n" \
	"Content-Length: 4\r\n" \
	"\r\n" \
	"Hi.\n"

#define DEFAULT_RESPONSE_LEGNTH sizeof( DEFAULT_RESPONSE ) - 1

int parse_request( struct request * const request, char * const header )
{
	memset( request, 0, sizeof( struct request ) );

	char *token_start, *token_end;

	token_end = header;

	token_start = strsep( &token_end, " " );

	if ( token_end - token_start > 8 )
	{
		return HTTP_BAD_METHOD;
	}

	memcpy( request->method, token_start, token_end - token_start );
	errfs( "%lx %s", (uint64_t)*(request->method), request->method );

	return 0;
}

int accept_connection( int* connection, int sock )
{
	ssize_t result;
	struct timeval timeout = { CONN_TIMEOUT_SEC, 0 };
	fd_set descriptors;

	FD_ZERO( &descriptors );
	FD_SET( sock, &descriptors );

	*connection = NO_ACTIVE_CONNECTION;
	result = select( sock + 1, &descriptors, NULL, NULL, &timeout );

	if ( result == -1 )
	{
		err( "Failed in select() on listening socket" );
		// TODO: Better handle case where sock is an invalid descriptor.
		return 1;
	}

	if ( result == 0 )
	{
		return 1;
	}

	*connection = accept( sock, NULL, NULL );

	if ( *connection == -1 )
	{
		err( "Error accepting connection" );
		// TODO: Better handle errors here.
		return 1;
	}

	errs( "Accepted connection" );

	return 0;
}

int read_headers( char* buffer, int connection )
{
	ssize_t result;
	struct timeval timeout = { READ_TIMEOUT_SEC, 0 };
	fd_set descriptors;

	FD_ZERO( &descriptors );
	FD_SET( connection, &descriptors );

	result = select( connection + 1, &descriptors, NULL, NULL, &timeout );

	if ( result == -1 )
	{
		err( "Failed in select() on active connection" );
		return 1;
	}

	if ( result == 0 )
	{
		errs( "Timed out waiting for request" );
		return 1;
	}

	// TODO: Check for double line break (with strsep?) for actual header length
	// TODO: re-call select for long headers.
	result = read( connection, buffer, MAX_HEADER_LENGTH );

	if ( result == 0 )
	{
		close( connection );
		return 1;
	}

	if ( result == -1 )
	{
		err( "Error reading headers" );
		return 1;
	}

	if ( result == MAX_HEADER_LENGTH )
	{
		// TODO: Error handling here
		ioctl( connection, FIONREAD, &result );

		errfs( "Request headers were too long (approximately %ld bytes)", MAX_HEADER_LENGTH + result );
		// TODO: Error handling here
		write( connection, HEADER_TOO_LONG_RESPONSE, sizeof( HEADER_TOO_LONG_RESPONSE ) - 1 );

		return 1;
	}

	errfs( "Read %ld bytes from client", result );
	return 0;
}

void* accept_loop( void * args )
{
	char header_buffer[MAX_HEADER_LENGTH];
	struct request request;

	int sock = *((int*)args);
	int connection = -1;

	ssize_t result;

	errs( "Accepting requests" );
	while ( state == 0 )
	{
		if ( connection == NO_ACTIVE_CONNECTION )
		{
			if ( accept_connection( &connection, sock ) )
			{
				continue;
			}
		}

		if ( read_headers( header_buffer, connection ) )
		{
			close( connection );
			connection = NO_ACTIVE_CONNECTION;
			continue;
		}

		if ( parse_request( &request, header_buffer ) )
		{
			close( connection );
			connection = NO_ACTIVE_CONNECTION;
			continue;
		}

		// TODO: Routing
		// TODO: Handling
		// TODO: Response Rendering
		result = write( connection, DEFAULT_RESPONSE, DEFAULT_RESPONSE_LEGNTH );

		if ( result < 0 )
		{
			err( "Error writing response to client" );
			close( connection );
			connection = NO_ACTIVE_CONNECTION;
		}
		else if ( result != sizeof( DEFAULT_RESPONSE ) - 1 )
		{
			errfs( "Error writing response to client: only wrote %ld of %ld bytes.", result, DEFAULT_RESPONSE_LEGNTH );
			close( connection );
			connection = NO_ACTIVE_CONNECTION;
		}
	}

	return NULL;
}
