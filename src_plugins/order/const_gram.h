#ifndef _pcb_ordc__defines_h_
#define _pcb_ordc__defines_h_

typedef short pcb_ordc_int_t;
#define pcb_ordc_chr yyctx->chr
#define pcb_ordc_val yyctx->val
#define pcb_ordc_lval yyctx->lval
#define pcb_ordc_stack yyctx->stack
#define pcb_ordc_debug yyctx->debug
#define pcb_ordc_nerrs yyctx->nerrs
#define pcb_ordc_errflag yyctx->errflag
#define pcb_ordc_state yyctx->state
#define pcb_ordc_yyn yyctx->yyn
#define pcb_ordc_yym yyctx->yym
#define pcb_ordc_jump yyctx->jump
#line 18 "../src_plugins/order/const_gram.y"
/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  order plugin - constraint language grammar
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */
#include <stdio.h>
#include "../src_plugins/order/const_gram.h"
#include "../src_plugins/order/constraint.h"

#define binop(a, b, c) NULL
#define unop(a, b) NULL

#line 8 "../src_plugins/order/const_gram.y"
typedef union pcb_ordc_tokunion_u
{
	double d;
	int i;
	char *s;
	void *tree;
} pcb_ordc_tokunion_t;
#line 3 "../src_plugins/order/const_gram.y"
typedef struct pcb_ordc_tokstruct_s
{
	pcb_ordc_tokunion_t un;
	long line, first_col, last_col;
} pcb_ordc_tokstruct_t;
typedef pcb_ordc_tokstruct_t pcb_ordc_STYPE;


#define T_ID 257
#define T_INTEGER 258
#define T_FLOAT 259
#define T_QSTR 260
#define T_EQ 261
#define T_NEQ 262
#define T_GE 263
#define T_IF 264
#define T_ERROR 265
#define T_INTVAL 266
#define T_FLOATVAL 267
#define T_LE 268
#define T_GT 269
#define T_LT 270
#define pcb_ordc_ERRCODE 256

#ifndef pcb_ordc_INITSTACKSIZE
#define pcb_ordc_INITSTACKSIZE 200
#endif

typedef struct {
	unsigned stacksize;
	pcb_ordc_int_t *s_base;
	pcb_ordc_int_t *s_mark;
	pcb_ordc_int_t *s_last;
	pcb_ordc_STYPE *l_base;
	pcb_ordc_STYPE *l_mark;
#if pcb_ordc_DEBUG
	int debug;
#endif
} pcb_ordc_STACKDATA;

typedef struct {
	int errflag;
	int chr;
	pcb_ordc_STYPE val;
	pcb_ordc_STYPE lval;
	int nerrs;
	int yym, yyn, state;
	int jump;
	int stack_max_depth;
	int debug;

	/* variables for the parser stack */
	pcb_ordc_STACKDATA stack;
} pcb_ordc_yyctx_t;

typedef enum { pcb_ordc_RES_NEXT, pcb_ordc_RES_DONE, pcb_ordc_RES_ABORT } pcb_ordc_res_t;

extern int pcb_ordc_parse_init(pcb_ordc_yyctx_t *yyctx);
extern pcb_ordc_res_t pcb_ordc_parse(pcb_ordc_yyctx_t *yyctx, pcb_ordc_ctx_t *ctx, int tok, pcb_ordc_STYPE *lval);
extern void pcb_ordc_error(pcb_ordc_ctx_t *ctx, pcb_ordc_STYPE tok, const char *msg);


#endif /* _pcb_ordc__defines_h_ */
