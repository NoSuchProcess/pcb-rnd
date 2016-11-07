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

#include <unistd.h>
#include <stdarg.h>
#include "pcb-printf.h"
#include "base64.h"

#define MAX_FIELD_ALLOC 1024*1024

static proto_node_t *list_new(proto_ctx_t *ctx, proto_node_t *parent, int is_bin)
{
	proto_node_t *l;

	if (ctx->palloc + sizeof(proto_node_t) > MAX_FIELD_ALLOC)
		return NULL;
	ctx->palloc += sizeof(proto_node_t);

	l = calloc(sizeof(proto_node_t), 1);
	if (l != NULL) {
		l->is_list = 1;
		l->is_bin = is_bin;
		l->parent = parent;
	}
	return l;
}

static proto_node_t *str_new(proto_ctx_t *ctx, proto_node_t *parent, int is_bin)
{
	proto_node_t *l;

	if (ctx->palloc + sizeof(proto_node_t) > MAX_FIELD_ALLOC)
		return NULL;
	ctx->palloc += sizeof(proto_node_t);

	l = calloc(sizeof(proto_node_t), 1);
	if (l != NULL) {
		l->is_list = 0;
		l->is_bin = is_bin;
		l->in_len = 1;
		l->parent = parent;
	}
	return l;
}

static void list_append(proto_ctx_t *ctx, proto_node_t *l)
{
	if (ctx->tcurr->data.l.last_child != NULL) {
		ctx->tcurr->data.l.last_child->data.l.next = l;
		ctx->tcurr->data.l.last_child = l;
	}
	else
		ctx->tcurr->data.l.first_child = ctx->tcurr->data.l.last_child = l;
	ctx->tcurr = l;
}

static void pop(proto_ctx_t *ctx)
{
	ctx->tcurr = ctx->tcurr->parent;
	ctx->pst = PST_LIST;
}

static int parse_char(proto_ctx_t *ctx, int c)
{
	switch(ctx->pst) {
		case PST_MSG:
			if (c == '#') {
				ctx->pst = PST_COMMENT;
				break;
			}
			if ((c == '\r') || (c == '\n') || (c == ' ') || (c == '\t')) /* ignore empty lines */
				break;
			if (chr_is_bin(c)) /* invalid command */
				return -1;
			ctx->pst = PST_CMD;
			ctx->palloc = 0;
			ctx->pcmd[0] = c;
			ctx->pcmd_next = 1;
			break;

		case PST_COMMENT:
			if (c == '\n')
				ctx->pst = PST_MSG;
			break;

		case PST_CMD:
			if ((c == '(') || (c == '{')) {
				ctx->targ = ctx->tcurr = list_new(ctx, NULL, c == '{');
				if (ctx->tcurr == NULL)
					return -1;
				ctx->pst = PST_LIST;
				break;
			}
			if (chr_is_bin(c)) /* invalid command */
				return -1;
			if (ctx->pcmd_next >= 16) /* command name too long */
				return -1;
			ctx->pcmd[ctx->pcmd_next++] = c;
			break;

		case PST_LIST:
			do_list:;
			if ((c == ')') || (c == '}')) { /* close current list */
				pop(ctx);
				break;
			}
			if (c == '\n') {
				if (ctx->tcurr != NULL) /* unbalanced: newline before enough closes */
					return -1;
				ctx->pst = PST_MSG;
			}

			if (ctx->tcurr == NULL) /* unbalanced: too many closes */
				return -1;

			if ((c == '(') || (c == '{')) { /* open new list */
				proto_node_t *l = list_new(ctx, ctx->tcurr, c == '{');
				if (l == NULL)
					return -1;
				list_append(ctx, l);
				ctx->pst = PST_LIST;
				break;
			}
			if (c == ' ') /* field separator - may be after a bin-string, but tolerate for now */
				break;
			
			/* we are in a list and got a string char. It's the first char of a
			   text string or the first character of a binary string length, depending
			   on the list context */
			if (ctx->tcurr->is_bin) {
				proto_node_t *s = str_new(ctx, ctx->tcurr, 1);
				if (s == NULL)
					return -1;
				list_append(ctx, s);
				goto do_bstr;
			}
			else {
				proto_node_t *s = str_new(ctx, ctx->tcurr, 0);
				if (s == NULL)
					return -1;
				s->data.s.str = malloc(18);
				list_append(ctx, s);
				if (s->data.s.str)
					goto error;
				goto do_tstr;
			}

		case PST_TSTR:
			do_tstr:;
			if (c == ' ') {
				ctx->tcurr->data.s.str[ctx->tcurr->data.s.len] = '\0';
				ctx->pst = PST_LIST;
				break;
			}
			if ((c == ')') || (c == '}')) {
				ctx->tcurr->data.s.str[ctx->tcurr->data.s.len] = '\0';
				pop(ctx);
				goto do_list;
			}
			if (chr_is_bin(c)) /* invalid: binary char in text string */
				return -1;
			if (ctx->tcurr->data.s.len >= 16) /* invalid: text string too long */
				return -1;
			ctx->tcurr->data.s.str[ctx->tcurr->data.s.len++] = c;
			break;

		case PST_BSTR:
			do_bstr:;
			if (ctx->tcurr->in_len) { /* parsing length */
				int res = base64_parse_grow(&ctx->tcurr->data.s.len, c, '=');
				if (res < 0) /* invalid length field */
					return -1;
				if (res == 0) /* proceed with len parsing */
					break;
				/* seen terminator, len parsing finished */
				if (ctx->palloc + ctx->tcurr->data.s.len > MAX_FIELD_ALLOC) /* limit allocation */
					return -1;
				ctx->tcurr->in_len = 0;
				if (ctx->tcurr->data.s.len == 0) {
					pop(ctx);
					break;
				}
				ctx->palloc += ctx->tcurr->data.s.len;
				ctx->tcurr->data.s.str = malloc(ctx->tcurr->data.s.len);
				if (ctx->tcurr->data.s.str)
					return -1;
			}
			else { /* parsing the actual string */
				ctx->tcurr->data.s.str[ctx->tcurr->data.s.pos++] = c;
				if (ctx->tcurr->data.s.pos >= ctx->tcurr->data.s.len) {
					pop(ctx);
					break;
				}
			}
			break;
	}


	error:;
#warning TODO: free args
	return -1;
}

