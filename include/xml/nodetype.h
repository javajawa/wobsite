// SPDX-FileCopyrightText: 2020 Benedict Harcourt <ben.harcourt@harcourtprogramming.co.uk>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef __WOBSITE_HTML_NODETYPE_H
#define __WOBSITE_HTML_NODETYPE_H

enum html_node_type
{
	NONE      = 0,
	ELEMENT   = 1,
	TEXT      = 3,
	CDATA     = 4,
	COMMENT   = 8,
	DOCUMENT  = 9,
	FRAGMENT  = 11,
	LAYOUT    = 64
};

#endif
