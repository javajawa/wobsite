#include "responder.h"

#include <fcgiapp.h>

void* accept_loop( void * args )
{
	int sock = *((int*)args);
	FCGX_Request request;

	if ( FCGX_InitRequest( &request, sock, 0 ) )
	{
		err( "Failed to initialise reques buffer" );
		return NULL;
	}

	errs( "Waiting for request" );
	while ( 1 )
	{
		if ( FCGX_Accept_r( &request ) == -1 )
		{
			errs( "Error accepting request" );
			break;
		}
	}

	return NULL;
}
