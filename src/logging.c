#include <time.h>

char timestr[9];

char const * get_timestamp()
{
	time_t timestamp;
	struct tm date;

	time( &timestamp );
	localtime_r( &timestamp, &date );

	strftime( timestr, 9, "%H:%I:%S", &date );

	return timestr;
}

