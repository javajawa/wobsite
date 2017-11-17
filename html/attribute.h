#ifndef __WOBSITE_HTML_ATTRIBUTE_H
#define __WOBSITE_HTML_ATTRIBUTE_H

#include <stddef.h>

#include "token.h"

#ifndef WOBSITE_XML_MAX_ATTR_LENGTH
#define WOBSITE_XML_MAX_ATTR_LENGTH 16
#endif

struct attribute
{
	char const key[WOBSITE_XML_MAX_ATTR_LENGTH];
	union token value;
};

#endif
