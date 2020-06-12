// SPDX-FileCopyrightText: 2020 Benedict Harcourt <ben.harcourt@harcourtprogramming.co.uk>
//
// SPDX-License-Identifier: BSD-2-Clause

#include <time.h>
#include <logging.h>

static char timestr[9];
static char* escapes =
	"0"     // Null
	"\0"    // Start of heading
	"\0"    // Start of text
	"\0"    // End of text
	"\0"    // End of transmission
	"\0"    // Enquiry
	"\0"    // Ack
	"a"     // Bell
	"b"     // Backspace
	"t"     // Tab
	"n"     // New line
	"v"     // Vertical table
	"\0"	// ???
	"r"     // Carriage return
	"\0"    // Shift out
	"\0"    // Shift in
	"\0"    // Data link escape
	"\0"    // Device control 1
	"\0"    // Device control 2
	"\0"    // Device control 3
	"\0"    // Device control 4
	"\0"    // NACK
	"\0"    // SYN
	"\0"    // End of transmission block
	"\0"    // Cancel
	"\0"    // End of medium
	"\0"    // Subsititue
	"["     // Escape
	"\0"    // File sep
	"\0"    // group sep
	"\0"    // record sep
	"\0"    // unit sep
;

char const * get_timestamp()
{
	time_t timestamp;
	struct tm date;

	time( &timestamp );
	localtime_r( &timestamp, &date );

	strftime( timestr, 9, "%H:%M:%S", &date );

	return timestr;
}

void strdump( unsigned char const * restrict str, ssize_t lim )
{

	if ( lim == -1 )
	{
		lim = strlen( (char const *)str );
	}

	do
	{
		if ( *str < 32 )
		{
			if ( escapes[*str] )
			{
				fprintf( stderr, "\033[35m\\%c\033[0m", escapes[*str] );
			}
			else
			{
				fprintf( stderr, "\033[35m\\%02x\033[0m", *str );
			}

			if ( *str == '\n' )
			{
				fprintf( stderr, "\n" );
			}
		}
		else
		{
			fprintf( stderr, "%c", *str );
		}

		++str;
	}
	while ( --lim );

	fprintf( stderr, "\n" );
}
