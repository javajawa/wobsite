// SPDX-FileCopyrightText: 2020 Benedict Harcourt <ben.harcourt@harcourtprogramming.co.uk>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "xml/node.h"

struct state_node;

struct render_state
{
	struct state_node * node;
};

int render(
	union html_node const * node,
	char const * const * replacements,
	struct render_state * state
);
