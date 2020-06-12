// SPDX-FileCopyrightText: 2020 Benedict Harcourt <ben.harcourt@harcourtprogramming.co.uk>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef __WOBSITE_HTML_ELEMENT_H
#define __WOBSITE_HTML_ELEMENT_H

#include "attribute.h"
#include "nodetype.h"

#ifndef WOBSITE_XML_MAX_TAG_LENGTH
#define WOBSITE_XML_MAX_TAG_LENGTH 16
#endif

union html_node;

struct html_node_element
{
	enum   html_node_type const type;
	struct attribute      const * const attrs;
	union  html_node      const * const children;
	char const tag_name[WOBSITE_XML_MAX_TAG_LENGTH];
};

#include "node.h"

#endif
