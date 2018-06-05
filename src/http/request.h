#include <stdint.h>

struct request
{
	char    method[MAX_METHOD_LENGTH];
	uint8_t protocol;
	char    request[MAX_REQUEST_LENGTH];
};

enum http_status_code
{
	HTTP_OK = 200,
	HTTP_BAD_REQUEST = 400,
	HTTP_BAD_METHOD  = 405
};

