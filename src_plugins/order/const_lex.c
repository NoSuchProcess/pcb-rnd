static const unsigned char pcb_ordc_nfa_0[] = {4,3,0,0,0,0,0,32,255,131,0,0,0,40,0,0,0,0,11,3,0,0,0,0,0,32,255,131,0,0,0,40,0,0,0,0,0,0,0};
static const unsigned char pcb_ordc_bittab_0[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const unsigned char pcb_ordc_chrtyp_0[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0};
static const unsigned char pcb_ordc_nfa_1[] = {4,11,3,0,0,0,0,0,32,255,131,0,0,0,40,0,0,0,0,0,3,0,0,0,0,0,64,0,0,0,0,0,0,0,0,0,0,11,3,0,0,0,0,0,40,255,3,32,0,0,0,32,0,0,0,0,0,0,0};
static const unsigned char pcb_ordc_nfa_2[] = {4,3,0,0,0,0,0,0,0,0,254,255,255,7,254,255,255,7,11,3,0,0,0,0,0,32,255,3,254,255,255,135,254,255,255,7,0,0,0};
static const unsigned char pcb_ordc_nfa_3[] = {4,1,34,6,1,11,3,255,255,255,255,251,255,255,255,255,255,255,255,255,255,255,255,0,7,1,1,34,0,0};
static const unsigned char pcb_ordc_nfa_4[] = {4,3,0,0,0,0,98,191,0,112,0,0,0,0,0,0,0,16,0};
static const unsigned char pcb_ordc_nfa_5[] = {4,3,0,38,0,0,1,0,0,0,0,0,0,0,0,0,0,0,11,3,0,38,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
/* strtree.h BEGIN { */
#ifndef UREGLEX_STRTREE_H
#define UREGLEX_STRTREE_H 
typedef enum {ULX_REQ = 1, ULX_BRA, ULX_FIN, ULX_BAD} ureglex_stree_op_t;
typedef struct ureglex_strtree_s { int *code, *ip; } ureglex_strtree_t;
int ureglex_strtree_exec(ureglex_strtree_t *ctx, int chr);
#define UREGLEX_STRTREE_MORE -5
#endif
/* strtree.h END } */

#include <stdlib.h>
#include "ureglex/exec.h"
int pcb_ordc_strings[] = {2,101,34,2,102,10,2,105,27,4,1,108,1,111,1,97,1,116,1,118,1,97,1,108,3,9,4,2,102,56,2,110,45,4,1,114,1,114,1,111,1,114,3,7,4,1,116,1,118,1,97,1,108,3,8,4,3,6,4};
ureglex_precomp_t pcb_ordc_rules[] = {
	{pcb_ordc_nfa_0, pcb_ordc_bittab_0, pcb_ordc_chrtyp_0, 1.000000},
	{pcb_ordc_nfa_1, pcb_ordc_bittab_0, pcb_ordc_chrtyp_0, 1.000000},
	{pcb_ordc_nfa_2, pcb_ordc_bittab_0, pcb_ordc_chrtyp_0, 1.000000},
	{pcb_ordc_nfa_3, pcb_ordc_bittab_0, pcb_ordc_chrtyp_0, 1.000000},
	{pcb_ordc_nfa_4, pcb_ordc_bittab_0, pcb_ordc_chrtyp_0, 1.000000},
	{pcb_ordc_nfa_5, pcb_ordc_bittab_0, pcb_ordc_chrtyp_0, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{ureglex_nfa_str, NULL, NULL, 1.000000},
	{NULL, NULL, NULL, 0.0}
};
#define pcb_ordc_num_rules 10
typedef struct pcb_ordc_ureglex_s {
	ureglex_precomp_t *rules;
	char buff[256];
	int num_rules, buff_used, step_back_to, buff_save_term, by_len;
	long loc_offs[2], loc_line[2], loc_col[2];
	ureglex_t state[pcb_ordc_num_rules];
	const char *sp;
	int strtree_state, strtree_len, strtree_score;
	ureglex_strtree_t strtree;
	ureglex_t *pending_intcode;
} pcb_ordc_ureglex_t;

/* TOP CODE BEGIN { */
#line  5 "const_lex.ul"
	/*
	 *                            COPYRIGHT
	 *
	 *  pcb-rnd, interactive printed circuit board design
	 *
	 *  order plugin - constraint language lexer
	 *  pcb-rnd Copyright (C) 2022 Tibor 'Igor2' Palinkas
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
	#include <stdlib.h>
	#include <string.h>
	#include <librnd/core/compat_misc.h>
	#include "../src_plugins/order/const_gram.h"
	#define lval  ((pcb_ordc_STYPE *)(user_ctx))
/* TOP CODE END } */

/* exec_spec.h BEGIN { */
void pcb_ordc_lex_reset(pcb_ordc_ureglex_t *ctx);
void pcb_ordc_lex_init(pcb_ordc_ureglex_t *ctx, ureglex_precomp_t *rules);
int pcb_ordc_lex_char(pcb_ordc_ureglex_t *ctx, void *user_ctx, int chr);
/* exec_spec.h END } */

static int pcb_ordc_user_code(pcb_ordc_ureglex_t *ctx, void * user_ctx, int ruleid) {
	ureglex_t *rule = &ctx->state[ruleid];
	(void)rule;
	switch(ruleid) {
		case 0:{
#line  41 "const_lex.ul"
	lval->un.i = atoi(ULX_BUF);
	return T_INTEGER;

		}
		return UREGLEX_NOP;
		case 1:{
#line  47 "const_lex.ul"
	lval->un.d = strtod(ULX_BUF, NULL);
	return T_FLOAT;

		}
		return UREGLEX_NOP;
		case 2:{
#line  53 "const_lex.ul"
	lval->un.s = rnd_strdup(ULX_BUF);
	return T_ID;

		}
		return UREGLEX_NOP;
		case 3:{
#line  59 "const_lex.ul"
	lval->un.s = rnd_strndup(ULX_TAGP(1), ULX_TAGL(1));
	return T_QSTR;

		}
		return UREGLEX_NOP;
		case 4:{
#line  65 "const_lex.ul"
	return *ULX_BUF;

		}
		return UREGLEX_NOP;
		case 5:{
#line  70 "const_lex.ul"
	ULX_IGNORE;

		}
		return UREGLEX_NOP;
		case 6:{
               return T_IF;
		}
		return UREGLEX_NOP;
		case 7:{
            return T_ERROR;
		}
		return UREGLEX_NOP;
		case 8:{
           return T_INTVAL;
		}
		return UREGLEX_NOP;
		case 9:{
         return T_FLOATVAL;
		}
		return UREGLEX_NOP;
	}
	return UREGLEX_NO_MATCH;
	goto ureglex_ignore;
	ureglex_ignore:;
	pcb_ordc_lex_reset(ctx);
	return UREGLEX_MORE;
}

/* exec_spec.c BEGIN { */
void pcb_ordc_lex_reset(pcb_ordc_ureglex_t *ctx)
{
 int n = 0;
 if ((ctx->step_back_to >= 0) && (ctx->step_back_to < ctx->buff_used)) {
  if (ctx->buff_save_term > 0)
   ctx->buff[ctx->step_back_to] = ctx->buff_save_term;
  n = ctx->buff_used - ctx->step_back_to;
  memmove(ctx->buff, ctx->buff + ctx->step_back_to, n+1);
 }
 ctx->buff_used = n;
 for(n = 0; n < ctx->num_rules; n++)
  ureglex_exec_init(&ctx->state[n], ctx->buff, ctx->buff_used);
 ctx->buff_save_term = ctx->step_back_to = -1;
 ctx->loc_offs[0] = ctx->loc_offs[1];
 ctx->loc_line[0] = ctx->loc_line[1];
 ctx->loc_col[0] = ctx->loc_col[1];
#if 1
 ctx->strtree_state = UREGLEX_STRTREE_MORE;
 ctx->strtree_len = ctx->strtree_score = 0;
 ctx->strtree.ip = ctx->strtree.code = pcb_ordc_strings;
 ctx->sp = ctx->buff;
#endif
}
void pcb_ordc_lex_init(pcb_ordc_ureglex_t *ctx, ureglex_precomp_t *rules)
{
 ureglex_precomp_t *p;
 ctx->rules = rules;
 ctx->num_rules = 0;
 ctx->buff_save_term = ctx->step_back_to = -1;
 for(p = pcb_ordc_rules; p->nfa != NULL; p++)
  ctx->state[ctx->num_rules++].pc = p;
 ctx->by_len = (p->weight > 0.0);
 ctx->loc_offs[1] = ctx->loc_line[1] = ctx->loc_col[1] = 1;
 pcb_ordc_lex_reset(ctx);
 ctx->loc_offs[1] = ctx->loc_col[1] = 0;
 ctx->pending_intcode = NULL;
}
int pcb_ordc_lex_char(pcb_ordc_ureglex_t *ctx, void *user_ctx, int chr)
{
 ureglex_t *best = NULL;
 int n, working = 0;
 if (ctx->buff_used >= (sizeof(ctx->buff)-1))
  return UREGLEX_TOO_LONG;
 ctx->buff[ctx->buff_used++] = chr;
 ctx->buff[ctx->buff_used] = '\0';
 ctx->loc_offs[1]++;
 if (chr == '\n') {
  ctx->loc_line[1]++;
  ctx->loc_col[1] = 0;
 }
 else
  ctx->loc_col[1]++;
#if 0
 if (ctx->pending_intcode != NULL) {
  best = ctx->pending_intcode;
  best->eopat[0] = ctx->buff + ctx->buff_used-1;
  ctx->pending_intcode = NULL;
  goto skip;
 }
#endif
#if 1
 while((ctx->strtree_state == UREGLEX_STRTREE_MORE) && (ctx->sp < (&ctx->buff[ctx->buff_used]))) {
  ctx->strtree_state = ureglex_strtree_exec(&ctx->strtree, *ctx->sp++);
  if (ctx->strtree_state == UREGLEX_STRTREE_MORE)
   working++;
  if (ctx->strtree_state > 0) {
   ureglex_t *s = &ctx->state[ctx->strtree_state];
   ctx->strtree_len = ctx->buff_used;
   s->exec_state = s->score = ctx->strtree_score = (int)((double)(ctx->strtree_len-1) * s->pc->weight * 100);
   s->bopat[0] = ctx->buff;
   s->eopat[0] = ctx->buff + ctx->strtree_len - 1;
  }
 }
#endif
 for(n = 0; n < ctx->num_rules; n++) {
  ureglex_t *s = &ctx->state[n];
  if ((s->pc->bittab != NULL) && (s->exec_state < 0)) {
   s->exec_state = ureglex_exec(s);
   if (s->exec_state < 0)
    working++;
   else if ((s->exec_state > 0) && (s->pc->weight != 1.0))
    s->exec_state = s->score = (int)((double)s->exec_state * s->pc->weight);
  }
 }
 if (working != 0)
  return UREGLEX_MORE;
 for(n = 0; n < ctx->num_rules; n++) {
  ureglex_t *s = &ctx->state[n];
  if ((s->pc->bittab != NULL) && (s->exec_state > 0)) {
   if (best == NULL)
    best = s;
   else if ((!ctx->by_len) && (s->score > best->score))
    best = s;
   else if ((ctx->by_len) && (s->eopat[0] - s->eopat[1] > best->eopat[0] - best->eopat[1]))
    best = s;
  }
 }
#if 1
 if (ctx->strtree_state > 0) {
  if (best == NULL)
   best = &ctx->state[ctx->strtree_state];
  else if ((!ctx->by_len) && (ctx->strtree_score > best->score))
   best = &ctx->state[ctx->strtree_state];
  else if ((ctx->by_len) && (ctx->strtree_len > best->eopat[0] - best->eopat[1]))
   best = &ctx->state[ctx->strtree_state];
 }
#endif
#if 0
 ctx->pending_intcode = pcb_ordc_intcode_lookup(ctx, chr);
#endif
 if (best == NULL) {
  ctx->step_back_to = ctx->buff_used-1;
  return UREGLEX_NO_MATCH;
 }
#if 0
 skip:;
#endif
 ctx->step_back_to = best->eopat[0] - ctx->buff;
 ctx->buff_save_term = ctx->buff[ctx->step_back_to];
 ctx->buff[ctx->step_back_to] = '\0';
 return pcb_ordc_user_code(ctx, user_ctx, best - ctx->state);
}
/* exec_spec.c END } */

