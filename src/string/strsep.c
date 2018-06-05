#include "strsep.h"

#include <stddef.h>

char * strsep_custom( char ** str, const char delim, const char canary )
{
	char * start = *str;
	char curr;

	while ( 1 )
	{
		curr = **str;
		++(*str);

		if ( curr == canary )
		{
			return NULL;
		}
		if ( curr == 0 )
		{
			return NULL;
		}
		if ( curr == delim )
		{
			*(*str-1) = 0;
			return start;
		}
	}
}
