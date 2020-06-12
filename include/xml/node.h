// SPDX-FileCopyrightText: 2020 Benedict Harcourt <ben.harcourt@harcourtprogramming.co.uk>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef __WOBSITE_HTML_NODE_H
#define __WOBSITE_HTML_NODE_H

#include "nodetype.h"

struct html_node_
{
	enum html_node_type type;
};

union html_node;

#include "element.h"
#include "textnode.h"
#include "layout.h"

union html_node
{
	struct html_node_ b;
	struct html_node_element e;
	struct html_node_text t;
	struct html_node_layout l;
};

#endif
