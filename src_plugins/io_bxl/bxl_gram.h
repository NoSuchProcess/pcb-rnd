#ifndef _pcb_bxl__defines_h_
#define _pcb_bxl__defines_h_

typedef short pcb_bxl_int_t;
#define pcb_bxl_chr yyctx->chr
#define pcb_bxl_val yyctx->val
#define pcb_bxl_lval yyctx->lval
#define pcb_bxl_stack yyctx->stack
#define pcb_bxl_debug yyctx->debug
#define pcb_bxl_nerrs yyctx->nerrs
#define pcb_bxl_errflag yyctx->errflag
#define pcb_bxl_state yyctx->state
#define pcb_bxl_yyn yyctx->yyn
#define pcb_bxl_yym yyctx->yym
#define pcb_bxl_jump yyctx->jump
#line 17 "../../src_plugins/io_bxl/bxl_gram.y"
/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  BXL IO plugin - BXL grammar
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2020)
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
#include "bxl_gram.h"
#include "bxl.h"
#line 8 "../../src_plugins/io_bxl/bxl_gram.y"
typedef union
{
	double d;
	int i;
	char *s;
} pcb_bxl_tokunion_t;
#line 3 "../../src_plugins/io_bxl/bxl_gram.y"
typedef struct
{
	pcb_bxl_tokunion_t un;
	long line, first_col, last_col;
} pcb_bxl_tokstruct_t;
typedef pcb_bxl_tokstruct_t pcb_bxl_STYPE;


#define T_ID 257
#define T_INTEGER 258
#define T_REAL_ONLY 259
#define T_QSTR 260
#define T_TRUE 261
#define T_FALSE 262
#define T_TEXTSTYLE 263
#define T_FONTWIDTH 264
#define T_FONTCHARWIDTH 265
#define T_FONTHEIGHT 266
#define T_PADSTACK 267
#define T_ENDPADSTACK 268
#define T_SHAPES 269
#define T_PADSHAPE 270
#define T_HOLEDIAM 271
#define T_SURFACE 272
#define T_PLATED 273
#define T_WIDTH 274
#define T_HEIGHT 275
#define T_PADTYPE 276
#define T_LAYER 277
#define T_PATTERN 278
#define T_ENDPATTERN 279
#define T_DATA 280
#define T_ENDDATA 281
#define T_ORIGINPOINT 282
#define T_PICKPOINT 283
#define T_GLUEPOINT 284
#define T_PAD 285
#define T_NUMBER 286
#define T_PINNAME 287
#define T_PADSTYLE 288
#define T_ORIGINALPADSTYLE 289
#define T_ORIGIN 290
#define T_ORIGINALPINNUMBER 291
#define T_ROTATE 292
#define T_POLY 293
#define T_LINE 294
#define T_ENDPOINT 295
#define T_ATTRIBUTE 296
#define T_ATTR 297
#define T_JUSTIFY 298
#define T_ARC 299
#define T_RADIUS 300
#define T_STARTANGLE 301
#define T_SWEEPANGLE 302
#define T_TEXT 303
#define T_ISVISIBLE 304
#define T_PROPERTY 305
#define T_WIZARD 306
#define T_VARNAME 307
#define T_VARDATA 308
#define T_TEMPLATEDATA 309
#define T_ISFLIPPED 310
#define T_NOPASTE 311
#define T_SYMBOL 312
#define T_ENDSYMBOL 313
#define T_COMPONENT 314
#define T_ENDCOMPONENT 315
#define pcb_bxl_ERRCODE 256

#ifndef pcb_bxl_INITSTACKSIZE
#define pcb_bxl_INITSTACKSIZE 200
#endif

typedef struct {
	unsigned stacksize;
	pcb_bxl_int_t *s_base;
	pcb_bxl_int_t *s_mark;
	pcb_bxl_int_t *s_last;
	pcb_bxl_STYPE *l_base;
	pcb_bxl_STYPE *l_mark;
#if pcb_bxl_DEBUG
	int debug;
#endif
} pcb_bxl_STACKDATA;

typedef struct {
	int errflag;
	int chr;
	pcb_bxl_STYPE val;
	pcb_bxl_STYPE lval;
	int nerrs;
	int yym, yyn, state;
	int jump;
	int stack_max_depth;
	int debug;

	/* variables for the parser stack */
	pcb_bxl_STACKDATA stack;
} pcb_bxl_yyctx_t;

typedef enum { pcb_bxl_RES_NEXT, pcb_bxl_RES_DONE, pcb_bxl_RES_ABORT } pcb_bxl_res_t;

extern int pcb_bxl_parse_init(pcb_bxl_yyctx_t *yyctx);
extern pcb_bxl_res_t pcb_bxl_parse(pcb_bxl_yyctx_t *yyctx, pcb_bxl_ctx_t *ctx, int tok, pcb_bxl_STYPE *lval);
extern void pcb_bxl_error(pcb_bxl_ctx_t *ctx, pcb_bxl_STYPE tok, const char *msg);


#endif /* _pcb_bxl__defines_h_ */
