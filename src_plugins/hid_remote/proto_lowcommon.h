/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2015 Tibor 'Igor2' Palinkas
 *
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdlib.h>
#include <ctype.h>

typedef enum {
	PST_MSG,
	PST_COMMENT,
	PST_CMD,
	PST_TSTR,
	PST_BSTR,
	PST_LIST
} parse_state_t;


typedef struct proto_node_s  proto_node_t;

struct proto_node_s {
	unsigned is_list:1;  /* whether current node is a list */
	unsigned is_bin:1;   /* whether current node is a binary - {} list or len=str */
	unsigned in_len:1;   /* for parser internal use */
	proto_node_t *parent;
	proto_node_t *next;                      /* next sibling on the current level (singly linked list) */
	union {
		struct { /* if ->is_list == 0  */
			unsigned long int len; /* length of the string */
			unsigned long int pos; /* for parser internal use */
			char *str;
		} s;
		struct { /* if ->is_list == 1  */
			proto_node_t *first_child, *last_child;
		} l;
	} data;
};

typedef struct {
	int depth;    /* parenthesis nesting depth */
	int bindepth; /* how many of the inmost levels are binary */
	unsigned long int subseq_tab;

	/* parser */
	parse_state_t pst;
	int pdepth;
	char pcmd[18];
	int pcmd_next;
	proto_node_t *targ, *tcurr;
	size_t palloc; /* field allocation total */
} proto_ctx_t;

static int chr_is_bin(char c)
{
	if (isalnum(c))
		return 0;
	if ((c == '-') || (c == '+') || (c == '.') || (c == '_') || (c == '#'))
		return 0;
	return 1;
}
