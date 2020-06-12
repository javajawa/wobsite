// SPDX-FileCopyrightText: 2020 Benedict Harcourt <ben.harcourt@harcourtprogramming.co.uk>
//
// SPDX-License-Identifier: BSD-2-Clause

#include <stdint.h>

#define HTTP_VERSION_1_1 0x0101
#define HTTP_VERSION_1_0 0x0100

struct request
{
	uint16_t protocol;
	char*    method;
	char*    request;
	char*    _end;
};

enum http_status_code
{
	HTTP_OK = 200,
	HTTP_BAD_REQUEST = 400,
	HTTP_BAD_METHOD  = 405
};

