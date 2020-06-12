// SPDX-FileCopyrightText: 2020 Benedict Harcourt <ben.harcourt@harcourtprogramming.co.uk>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef __WOBSITE_HTML_TEXTNODE_H
#define __WOBSITE_HTML_TEXTNODE_H

#include "node.h"

struct html_node_text
{
	enum html_node_type type;
	union token value;
};

#endif
