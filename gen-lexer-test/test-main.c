#include "headers.h"

#include <stdio.h>

int main()
{
	struct lexed_headers out;
	char dat[] = "GET / HTTP/1.1\nHost: 127.0.0.2:8888\nCoNTent-Type: text/hello\r\nUser-Agent: curl/7.52.1\nAccept: */*";

	lex_headers( dat, &out );

	printf( "Accept   '%s'\n", out.accept );
	printf( "C-Type   '%s'\n", out.content_type );

	return 0;
}
