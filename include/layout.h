// SPDX-FileCopyrightText: 2020 Benedict Harcourt <ben.harcourt@harcourtprogramming.co.uk>
//
// SPDX-License-Identifier: BSD-2-Clause

#define attrs( ... ) (struct attribute[]){ __VA_ARGS__, { "", { .t = { TOKEN_NULL } } } }
#define at_str( key, value ) { key, { .s = { TOKEN_STRING, value } } }
#define at_rep(  key, value ) { key, { .p = { TOKEN_PARAM,  value } } }
#define children( ... ) (union html_node[]){ __VA_ARGS__, { .b = { NONE } } }
#define element( tag, attrs, children ) { .e = { \
	ELEMENT, \
	attrs, \
	children, \
	tag \
} }
#define text( txt ) { .t = { TEXT, { .s = { TOKEN_STRING, txt } } } }
#define replace( id ) { .t = { TEXT, { .p = { TOKEN_PARAM, id } } } }
#define yield( id ) { .l = { LAYOUT, id } }

