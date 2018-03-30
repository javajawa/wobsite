#ifndef __WOBSITE_TOKEN_H
#define __WOBSITE_TOKEN_H

#include <stdint.h>

enum token_type
{
	TOKEN_NULL,
	TOKEN_STRING,
	TOKEN_PARAM
};

struct token_
{
	enum token_type const type;
};

struct token_string
{
	enum token_type const type;
	char const * const str;
};

struct token_param
{
	enum token_type const type;
	uint16_t const idx;
};

union token
{
	struct token_ t;
	struct token_string s;
	struct token_param  p;
};

#endif
