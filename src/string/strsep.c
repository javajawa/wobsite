// SPDX-FileCopyrightText: 2020 Benedict Harcourt <ben.harcourt@harcourtprogramming.co.uk>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "string/strsep.h"

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

			if ( delim == '\n' && *(*str-2) == '\r' )
			{
				*(*str-2) = '\0';
			}

			return start;
		}
	}
}
