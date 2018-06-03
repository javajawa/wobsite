#include <stdint.h>

struct request
{
	char    method[8];
	uint8_t protocol;
	char    request[1024];
};

enum http_status_code
{
	HTTP_OK = 200,
	HTTP_BAD_REQUEST = 400,
	HTTP_BAD_METHOD  = 405
};

