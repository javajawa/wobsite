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
#include <arpa/inet.h>

#define NO_ACTIVE_CONNECTION -1
#define READ_TIMEOUT_SEC 3
#define CONN_TIMEOUT_SEC 1
#define MAX_HEADER_LENGTH 512

#define HEADER_TOO_LONG_CONTENT "Request headers too large (max " STR(MAX_HEADER_LENGTH) " bytes)\n"


#define HEADER_TOO_LONG_RESPONSE \
	"HTTP/1.1 400 Bad Request (Request Header too long)\r\n" \
	"Content-Type: text/plain\r\n" \
	"Connection: close\r\n" \
	"Content-Length: " "43" "\r\n" \
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

struct connection
{
	int             fd;
	in_port_t       port;
	struct in6_addr host;
	struct timeval  time;
	char            remote[54];
};

union sockaddr_inet
{
	struct sockaddr     raw;
	struct sockaddr_in  v4;
	struct sockaddr_in6 v6;
};

int parse_request( struct request * const request, struct connection const * connection, char * const header )
{
	memset( request, 0, sizeof( struct request ) );

	char *token_start, *token_end;

	token_end = header;

	token_start = strsep( &token_end, " " );

	if ( token_end == NULL )
	{
		return HTTP_BAD_REQUEST;
	}

	if ( token_end - token_start > 8 )
	{
		return HTTP_BAD_METHOD;
	}

	memcpy( request->method, token_start, token_end - token_start );

	// TODO: Write the rest of the request parser
	printf( "%s %s %s\n", connection->remote, request->method, "/" );

	return 0;
}

int accept_connection( struct connection * connection, int sock )
{
	ssize_t result;
	struct timeval timeout = { CONN_TIMEOUT_SEC, 0 };
	union sockaddr_inet addr;
	socklen_t addr_len;
	fd_set descriptors;

	FD_ZERO( &descriptors );
	FD_SET( sock, &descriptors );

	connection->fd = NO_ACTIVE_CONNECTION;
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

	addr_len = sizeof( addr );
	connection->fd = accept( sock, &addr.raw, &addr_len );

	if ( connection->fd == -1 )
	{
		err( "Error accepting connection" );
		// TODO: Better handle errors here.
		return 1;
	}

	if ( addr_len > sizeof( addr ) )
	{
		errfs( "Could not fit remote address into buffer (got %u of %lu bytes)", addr_len, sizeof( addr ) );
	}

	switch ( addr.raw.sa_family )
	{
		case AF_INET:
			connection->port = addr.v4.sin_port;
			memset( &connection->host, 0, sizeof( struct in6_addr ) );
			memset( (uint8_t*)(&connection->host) + 10, ~0, 2 );
			memcpy( (uint8_t*)(&connection->host) + 12, &addr.v4.sin_addr,  sizeof( struct in_addr ) );
			break;

		case AF_INET6:
			connection->port = addr.v6.sin6_port;
			memcpy( &connection->host, &addr.v6.sin6_addr, sizeof( struct in6_addr ) );
			break;

		default:
			errfs( "Unknown socket family with enumeration value %d", addr.raw.sa_family );
			memset( &connection->host, 0, sizeof( struct in6_addr ) );
			connection->port = 0;
			break;
	}

	if ( ! inet_ntop( AF_INET6, &connection->host, connection->remote+1, 52 ) )
	{
		strcpy( connection->remote + 1, "unknown" );
	}

	for ( result = 1; connection->remote[result]; ++result );
	snprintf( connection->remote + result, 52 - result, "]:%u", connection->port );

	errfs( "Accepted connection from %s", connection->remote );

	return 0;
}

int read_headers( char* buffer, struct connection const * connection )
{
	ssize_t result;
	struct timeval timeout = { READ_TIMEOUT_SEC, 0 };
	fd_set descriptors;

	FD_ZERO( &descriptors );
	FD_SET( connection->fd, &descriptors );

	result = select( connection->fd + 1, &descriptors, NULL, NULL, &timeout );

	if ( result == -1 )
	{
		errf( "Failed in select() on active connection from %s", connection->remote );
		return 1;
	}

	if ( result == 0 )
	{
		errfs( "Timed out waiting for request from %s", connection->remote );
		return 1;
	}

	// TODO: Check for double line break (with strsep?) for actual header length
	// TODO: re-call select for long headers.
	result = read( connection->fd, buffer, MAX_HEADER_LENGTH );

	if ( result == 0 )
	{
		return 1;
	}

	if ( result == -1 )
	{
		errf( "Error reading headers from %s", connection->remote );
		return 1;
	}

	if ( result == MAX_HEADER_LENGTH )
	{
		// TODO: Error handling here
		ioctl( connection->fd, FIONREAD, &result );

		errfs( "Request headers from %s were too long (approximately %ld bytes)", connection->remote, MAX_HEADER_LENGTH + result );
		// TODO: Error handling here
		// TODO: return this to the main accept_loop.
		write( connection->fd, HEADER_TOO_LONG_RESPONSE, sizeof( HEADER_TOO_LONG_RESPONSE ) - 1 );

		return 1;
	}

	errfs( "Read %ld bytes from client", result );
	return 0;
}

void* accept_loop( void * args )
{
	char header_buffer[MAX_HEADER_LENGTH];
	struct request request;
	struct connection connection = { NO_ACTIVE_CONNECTION, 0, IN6ADDR_ANY_INIT, {0,0}, "[" };

	int sock = *((int*)args);

	ssize_t result;

	errs( "Accepting requests" );
	while ( state == 0 )
	{
		if ( connection.fd == NO_ACTIVE_CONNECTION )
		{
			if ( accept_connection( &connection, sock ) )
			{
				continue;
			}
		}

		if ( read_headers( header_buffer, &connection ) )
		{
			close( connection.fd );
			connection.fd = NO_ACTIVE_CONNECTION;
			continue;
		}

		if ( parse_request( &request, &connection, header_buffer ) )
		{
			close( connection.fd );
			connection.fd = NO_ACTIVE_CONNECTION;
			continue;
		}

		// TODO: Routing
		// TODO: Handling
		// TODO: Response Rendering
		result = write( connection.fd, DEFAULT_RESPONSE, DEFAULT_RESPONSE_LEGNTH );

		if ( result < 0 )
		{
			errf( "Error writing response to client %s", connection.remote );
			close( connection.fd );
			connection.fd = NO_ACTIVE_CONNECTION;
		}
		else if ( result != sizeof( DEFAULT_RESPONSE ) - 1 )
		{
			errfs( "Error writing response to client %s: only wrote %ld of %ld bytes.", connection.remote, result, DEFAULT_RESPONSE_LEGNTH );
			close( connection.fd );
			connection.fd = NO_ACTIVE_CONNECTION;
		}
	}

	return NULL;
}
