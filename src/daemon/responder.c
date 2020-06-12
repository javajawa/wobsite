#include "daemon/responder.h"

#include "config.h"
#include "daemon/threading.h"

#include "string/strsep.h"
#include "http/request.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include <arpa/inet.h>

#define NO_ACTIVE_CONNECTION -1

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
		switch ( errno )
		{
			case EINTR:
				return 1;

			case EBADF:
				// TODO: Better handle case where sock is an invalid descriptor
			default:
				err( LOG_NET, WARN, "Failed in select() on listening socket" );
				return 1;
		}
	}

	if ( result == 0 )
	{
		return 1;
	}

	addr_len = sizeof( addr );
	connection->fd = accept( sock, &addr.raw, &addr_len );

	if ( connection->fd == -1 )
	{
		err( LOG_NET, WARN, "Error accepting connection" );
		// TODO: Better handle errors here.
		return 1;
	}

	if ( addr_len > sizeof( addr ) )
	{
		// VERBOSE only because this should not be possible,
		// and we can allow the compiler to optimise this check out.
		errfs( LOG_NET, VERB, "Could not fit remote address into buffer (got %u of %lu bytes)", addr_len, sizeof( addr ) );
	}

	switch ( addr.raw.sa_family )
	{
		case AF_INET:
			connection->port = addr.v4.sin_port;
			memset( &connection->host, 0, sizeof( struct in6_addr ) );
			memset( (uint8_t*)(&connection->host) + sizeof( struct in6_addr ) - sizeof( struct in_addr ) - 2, ~0, 2 );
			memcpy( (uint8_t*)(&connection->host) + sizeof( struct in6_addr ) - sizeof( struct in_addr ), &addr.v4.sin_addr,  sizeof( struct in_addr ) );
			break;

		case AF_INET6:
			connection->port = addr.v6.sin6_port;
			memcpy( &connection->host, &addr.v6.sin6_addr, sizeof( struct in6_addr ) );
			break;

		default:
			errfs( LOG_NET, ALRT, "Unknown socket family with enumeration value %d", addr.raw.sa_family );
			memset( &connection->host, 0, sizeof( struct in6_addr ) );
			connection->port = 0;
			break;
	}

	// Convert the IP information to a human readable string
	// Start at offset 1 because the first character is always '['
	if ( ! inet_ntop( AF_INET6, &connection->host, connection->remote+1, 52 ) )
	{
		strcpy( connection->remote + 1, "unknown" );
	}

	// Find the end of the pasted string
	for ( result = 1; connection->remote[result]; ++result );

	// Append the closing ']' and the port number
	snprintf( connection->remote + result, 52 - result, "]:%u", connection->port );

	errfs( LOG_NET, VERB, "Accepted connection from %s", connection->remote );

	return 0;
}

int read_headers( char* buffer, struct connection const * connection )
{
	ssize_t result;
	size_t  offset = 0;
	char   *reader = buffer;
	uint8_t state  = 0;
	struct timeval timeout;
	fd_set descriptors;
	struct timespec start, end;

	FD_ZERO( &descriptors );
	FD_SET( connection->fd, &descriptors );

	while ( 1 )
	{
		// Start the timed section.
		clock_gettime( CLOCK_MONOTONIC, &start );
		// Check for input.
		timeout.tv_sec  = connection->time.tv_sec;
		timeout.tv_usec = connection->time.tv_usec;
		result = select( connection->fd + 1, &descriptors, NULL, NULL, &timeout );

		if ( result == -1 )
		{
			return -1;
		}
		if ( result == 0 )
		{
			errno = ETIMEDOUT;
			return -1;
		}

		result = read( connection->fd, reader, MAX_HEADER_LENGTH - offset );

		if ( result <= 0 )
		{
			return result;
		}

		offset += result;

		for ( ; result != 0 && state != 4; ++reader, --result )
		{
			switch ( *reader )
			{
				case '\n': state = ( state + 2 ) & ~1 ; break;
				case '\r': state |= 1; break;
				default:   state = 0;
			}
		}
		--reader;

		clock_gettime( CLOCK_MONOTONIC, &end );

		// TODO: decrement timer in connection

		if ( state == 4 )
		{
			return offset;
		}

		if ( offset == MAX_HEADER_LENGTH )
		{
			// TODO: Error handling here
			ioctl( connection->fd, FIONREAD, &result );
			errfs( LOG_NET, VERB, "Request headers from %s were too long (approximately %ld bytes)", connection->remote, MAX_HEADER_LENGTH + result );

			errno = EMSGSIZE;
			return -1;
		}
	}
}

int chomp( const char ** data, const char * restrict token )
{
	while ( *token && **data )
	{
		if ( *token != **data )
		{
			return 1;
		}
		++token;
		++*data;
	}

	return 0;
}

int parse_request( struct request * const request, struct connection const * connection, char * const header )
{
	char * token = header;

	request->_end   = (char*)~1LLU;
	request->method = strsep_custom( &token, ' ', '\n' );

	if ( request->method == NULL )
	{
		errno = HTTP_BAD_REQUEST;
		return -1;
	}

	request->request = strsep_custom( &token, ' ', '\n' );

	if ( request->request == NULL || token - request->request >= MAX_REQUEST_LENGTH )
	{
		errno = HTTP_BAD_REQUEST;
		return -1;
	}

	if ( chomp( (const char **)&token, "HTTP/" ) )
	{
		errno = HTTP_BAD_REQUEST;
		return -1;
	}

	// Get the HTTP Version
	sscanf( strsep_custom( &token, '\n', '\0' ), "%hhd.%hhd", (signed char*)&request->protocol, (signed char*)&request->protocol + 1 );

	printf( "%s HTTP/%04hx", connection->remote, request->protocol );
	for ( char** p = &request->method; *p != (char*)~1LLU; ++p )
	{
		printf( " '%s'", *p );
	}
	printf( "\n" );

	return 0;
}

void* accept_loop( void * args )
{
	char header_buffer[MAX_HEADER_LENGTH] = "";
	struct request request;
	struct connection connection = { NO_ACTIVE_CONNECTION, 0, IN6ADDR_ANY_INIT, {0,0}, "[" };
	enum state state;

	int sock = *((int*)args);

	ssize_t result;

	errs( LOG_NET, INFO, "Accepting requests" );
	while ( 1 )
	{
		get_current_thread_details( NULL, &state );

		if ( connection.fd == NO_ACTIVE_CONNECTION )
		{
			if ( ( state & ACCEPT ) == 0 )
			{
				errs( LOG_NET, INFO, "Not in ACCEPT state, exiting" )
				break;
			}

			if ( accept_connection( &connection, sock ) )
			{
				continue;
			}
		}

		connection.time.tv_sec  = READ_TIMEOUT_SEC;
		connection.time.tv_usec = 0;

		result = read_headers( header_buffer, &connection );

		if ( result == -1 && errno != EBADF && errno != ECONNRESET )
		{
			errf( LOG_NET, WARN, "Error reading request from client %s", connection.remote );
			result = write( connection.fd, HEADER_TOO_LONG_RESPONSE, sizeof( HEADER_TOO_LONG_RESPONSE ) - 1 );

			// TODO: handle result == -1 properly
			if ( result != sizeof( HEADER_TOO_LONG_RESPONSE ) - 1 )
			{
				errfs( LOG_NET, WARN, "Unexpected write result for error response to %s: wrote %lu or %lu bytes", connection.remote, result, sizeof( HEADER_TOO_LONG_RESPONSE ) - 1 );
			}
		}
		else if( result == 0 || errno == EBADF || errno == ECONNRESET )
		{
			errfs( LOG_NET, INFO, "Connection closed by client %s", connection.remote );
		}

		if ( result <= 0 )
		{
			errno = 0;
			close( connection.fd );
			errf( LOG_NET, VERB, "Closing socket to client %s", connection.remote );
			connection.fd = NO_ACTIVE_CONNECTION;

			continue;
		}

		errfs( LOG_NET, VERB, "Read %ld bytes from client", result );

		if ( parse_request( &request, &connection, header_buffer ) == -1 )
		{
			if ( errno < 400 )
			{
				errf( LOG_NET, INFO, "System error parsing request from client %s", connection.remote );
			}
			else
			{
				errfs( LOG_NET, INFO, "Logic error parsing request from client %s (%d)", connection.remote, errno );
			}

			errno = 0;
			result = write( connection.fd, HEADER_TOO_LONG_RESPONSE, sizeof( HEADER_TOO_LONG_RESPONSE ) - 1 );

			// TODO: handle result == -1 properly compared to wrong length
			if ( result != sizeof( HEADER_TOO_LONG_RESPONSE ) - 1 )
			{
				errfs( LOG_NET, WARN, "Unexpected write result for error response to %s: wrote %lu or %lu bytes", connection.remote, result, sizeof( HEADER_TOO_LONG_RESPONSE ) - 1 );
			}

			result = close( connection.fd );

			if ( result != -1 )
			{
				errf( LOG_NET, VERB, "Closing connection from client %s", connection.remote );
			}

			connection.fd = NO_ACTIVE_CONNECTION;
			continue;
		}

		// TODO: Routing
		// TODO: Handling
		// TODO: Response Rendering
		errfs( LOG_NET, VERB, "Writing response to %s", connection.remote );
		result = write( connection.fd, DEFAULT_RESPONSE, DEFAULT_RESPONSE_LEGNTH );

		if ( result < 0 )
		{
			errf( LOG_NET, WARN, "Error writing response to client %s", connection.remote );
			close( connection.fd );
			connection.fd = NO_ACTIVE_CONNECTION;
		}
		else if ( result != DEFAULT_RESPONSE_LEGNTH )
		{
			errfs( LOG_NET, WARN, "Error writing response to client %s: only wrote %ld of %ld bytes.", connection.remote, result, DEFAULT_RESPONSE_LEGNTH );
			close( connection.fd );
			connection.fd = NO_ACTIVE_CONNECTION;
		}
		else
		{
			errfs( LOG_NET, VERB, "Wrote %lu bytes to %s", result, connection.remote );
		}

		if ( connection.fd != NO_ACTIVE_CONNECTION && ( state & KEEPALIVE ) == 0 )
		{
			close( connection.fd );
			connection.fd = NO_ACTIVE_CONNECTION;
		}
	}

	thread_done();

	return NULL;
}
