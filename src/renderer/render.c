#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "render.h"

#define c_node state->node->node

struct state_node
{
	union html_node const * node;
	struct state_node * prev;
};

int render(
	union html_node const * main_node,
	char const * const * replacements,
	struct render_state * state
)
{
	struct attribute const * att;
	struct state_node * tmp;

	if ( ! main_node )
	{
		return -1;
	}

	if ( state->node == NULL )
	{
		state->node = calloc( 1, sizeof( struct state_node ) );
		c_node = main_node;
	}

	while ( 1 )
	{
		//fprintf( stderr, "Node %p type %d.", (void *)c_node, c_node->b.type );
		switch ( c_node->b.type )
		{
			case DOCUMENT:
			case CDATA:
			case COMMENT:
			case FRAGMENT:
				return -1;

			case LAYOUT:
				//fprintf( stderr, " -- yielding with state info %d\n", c_node->l.id );

				return (c_node++)->l.id;

			case NONE:
				tmp         = state->node->prev;
				free( state->node );
				state->node = tmp;

				if ( state->node == NULL )
				{
					//fprintf( stderr, " -- exiting from render\n" );
					return 0;
				}

				switch ( c_node->b.type )
				{
					case ELEMENT:
						//fprintf( stderr, " (closing elem <%s>)", c_node->e.tag_name );
						putchar( '<' );
						putchar( '/' );
						printf( "%s", c_node->e.tag_name );
						putchar( '>' );

						break;

					default:
						break;
				}

				break;

			case TEXT:
				//fprintf( stderr, " text type %d", c_node->t.value.t.type );
				switch ( c_node->t.value.t.type )
				{
					case TOKEN_STRING:
						printf( "%s", c_node->t.value.s.str );
						break;

					case TOKEN_PARAM:
						printf( "%s", replacements[c_node->t.value.p.idx] );
						break;

					case TOKEN_NULL:
					default:
						// TODO: Error?
						break;

				}
				break;

			case ELEMENT:
				//fprintf( stderr, " element <%s>", c_node->e.tag_name );
				putchar( '<' );
				printf( "%s", c_node->e.tag_name );

				for ( att = c_node->e.attrs; att && att->value.t.type != TOKEN_NULL; ++att )
				{
					putchar( ' ' );
					printf( "%s", att->key );
					putchar( '=' );
					putchar( '"' );

					switch ( att->value.t.type )
					{
						case TOKEN_STRING:
							printf( "%s",  att->value.s.str );
							break;
						case TOKEN_PARAM:
							printf( "%s", replacements[ att->value.p.idx ] );
							break;

						default:
							printf( "ERR" );
					}

					putchar( '"' );
				}

				if ( c_node->e.children )
				{
					tmp         = calloc( 1, sizeof( struct state_node ) );
					tmp->prev   = state->node;
					tmp->node   = c_node->e.children - 1;
					state->node = tmp;
				}
				else
				{
					putchar( '/' );
				}

				putchar( '>' );

				break;
		}

		++c_node;
		//fprintf( stderr, "\n" );
	}
}

