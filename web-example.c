#include <stdio.h>

#include "render.h"

#include "layouts/example.h"
#include "layouts/container.h"


int main( void )
{
	struct render_state outer_state = { NULL };
	struct render_state inner_state = { NULL };
	int state;

	do
	{
		state = render( &container, NULL, &outer_state );

		switch ( state )
		{
			case 1:
				render( &example, (char const*[]){ "foo-class", "Ben" }, &inner_state );
				render( &example, (char const*[]){ "foo-class", "Eth" }, &inner_state );
				break;
		}
	}
	while ( state != 0 );

	putchar( '\n' );

	return 0;
}
