
#ifdef YY_QUERY_API_VER
#define YY_BYACCIC
#define YY_API_MAJOR 1
#define YY_API_MINOR 0
#endif /*YY_QUERY_API_VER*/

#define pcb_ordc_EMPTY        (-1)
#define pcb_ordc_clearin      (pcb_ordc_chr = pcb_ordc_EMPTY)
#define pcb_ordc_errok        (pcb_ordc_errflag = 0)
#define pcb_ordc_RECOVERING() (pcb_ordc_errflag != 0)
#define pcb_ordc_ENOMEM       (-2)
#define pcb_ordc_EOF          0
#line 16 "../src_plugins/order/const_gram.c"
#include "../src_plugins/order/const_gram.h"
static const pcb_ordc_int_t pcb_ordc_lhs[] = {                    -1,
    0,    0,    5,    5,    5,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    4,    2,    3,
};
static const pcb_ordc_int_t pcb_ordc_len[] = {                     2,
    0,    2,    1,    1,    1,    2,    3,    3,    3,    3,
    3,    1,    1,    1,    2,    3,    5,    7,
};
static const pcb_ordc_int_t pcb_ordc_defred[] = {                  0,
    0,    0,    0,    0,    3,    4,    5,    0,    0,    0,
    0,    2,   12,   13,   14,    0,    0,    0,    0,    0,
   16,    6,    0,   15,    0,    0,    0,    0,    0,    0,
    7,    0,    0,   10,   11,   17,    0,    0,   18,
};
static const pcb_ordc_int_t pcb_ordc_dgoto[] = {                   4,
   19,    5,    6,    7,    8,
};
static const pcb_ordc_int_t pcb_ordc_sindex[] = {               -122,
  -30,  -29, -122,    0,    0,    0,    0, -122,  -24, -239,
 -106,    0,    0,    0,    0,  -24,  -24, -237,  -41,  -18,
    0,    0,  -38,    0,  -24,  -24,  -24,  -24, -122, -233,
    0,  -28,  -28,    0,    0,    0,  -13,  -27,    0,
};
static const pcb_ordc_int_t pcb_ordc_rindex[] = {                 29,
    0,    0,    0,    0,    0,    0,    0,   29,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  -35,  -33,    0,    0,    0,    0,    0,    0,
};
static const pcb_ordc_int_t pcb_ordc_gindex[] = {                 22,
   -3,    0,    0,    0,    6,
};
#define pcb_ordc_TABLESIZE 236
static const pcb_ordc_int_t pcb_ordc_table[] = {                  29,
    3,   27,   31,   28,   27,    8,   28,    9,   11,    9,
   10,   18,   22,   23,   27,   17,   28,   20,   21,   24,
   16,   32,   33,   34,   35,   30,   37,   38,    1,   12,
    0,   39,    0,    0,   36,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    1,    2,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   25,   26,    0,   25,   26,    0,    8,
    8,    9,    9,   13,   14,   15,
};
static const pcb_ordc_int_t pcb_ordc_check[] = {                  41,
  123,   43,   41,   45,   43,   41,   45,   41,    3,   40,
   40,   36,   16,   17,   43,   40,   45,  257,  125,  257,
   45,   25,   26,   27,   28,   44,  260,   41,    0,    8,
   -1,   59,   -1,   -1,   29,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  261,  262,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  265,  266,   -1,  265,  266,   -1,  265,
  266,  265,  266,  258,  259,  260,
};
#define pcb_ordc_FINAL 4
#define pcb_ordc_MAXTOKEN 269
#define pcb_ordc_UNDFTOKEN 277
#define pcb_ordc_TRANSLATE(a) ((a) > pcb_ordc_MAXTOKEN ? pcb_ordc_UNDFTOKEN : (a))
#if pcb_ordc_DEBUG
static const char *const pcb_ordc_name[] = {

"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"'!'",0,0,"'$'","'%'",0,0,"'('","')'","'*'","'+'","','","'-'",0,"'/'",0,0,0,0,0,
0,0,0,0,0,0,"';'","'<'",0,"'>'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'{'",0,
"'}'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,"T_ID","T_INTEGER","T_FLOAT","T_QSTR","T_IF",
"T_ERROR","T_INTVAL","T_FLOATVAL","\"==\"","\">=\"","\"<=\"","\"||\"","\"&&\"",
0,0,0,0,0,0,0,"illegal-symbol",
};
static const char *const pcb_ordc_rule[] = {
"$accept : file",
"file :",
"file : statement file",
"statement : stmt_if",
"statement : stmt_error",
"statement : stmt_block",
"expr : '-' expr",
"expr : '(' expr ')'",
"expr : expr \"==\" expr",
"expr : expr \">=\" expr",
"expr : expr '+' expr",
"expr : expr '-' expr",
"expr : T_INTEGER",
"expr : T_FLOAT",
"expr : T_QSTR",
"expr : '$' T_ID",
"stmt_block : '{' statement '}'",
"stmt_if : T_IF '(' expr ')' statement",
"stmt_error : T_ERROR '(' T_ID ',' T_QSTR ')' ';'",

};
#endif


#if pcb_ordc_DEBUG
#include <stdio.h> /* needed for printf */
#endif

#include <stdlib.h> /* needed for malloc, etc */
#include <string.h> /* needed for memset */

/* allocate initial stack or double stack size, up to yyctx->stack_max_depth */
static int pcb_ordc_growstack(pcb_ordc_yyctx_t *yyctx, pcb_ordc_STACKDATA *data)
{
	int i;
	unsigned newsize;
	pcb_ordc_int_t *newss;
	pcb_ordc_STYPE *newvs;

	if ((newsize = data->stacksize) == 0)
		newsize = pcb_ordc_INITSTACKSIZE;
	else if (newsize >= yyctx->stack_max_depth)
		return pcb_ordc_ENOMEM;
	else if ((newsize *= 2) > yyctx->stack_max_depth)
		newsize = yyctx->stack_max_depth;

	i = (int)(data->s_mark - data->s_base);
	newss = (pcb_ordc_int_t *) realloc(data->s_base, newsize * sizeof(*newss));
	if (newss == 0)
		return pcb_ordc_ENOMEM;

	data->s_base = newss;
	data->s_mark = newss + i;

	newvs = (pcb_ordc_STYPE *) realloc(data->l_base, newsize * sizeof(*newvs));
	if (newvs == 0)
		return pcb_ordc_ENOMEM;

	data->l_base = newvs;
	data->l_mark = newvs + i;

	data->stacksize = newsize;
	data->s_last = data->s_base + newsize - 1;
	return 0;
}

static void pcb_ordc_freestack(pcb_ordc_STACKDATA *data)
{
	free(data->s_base);
	free(data->l_base);
	memset(data, 0, sizeof(*data));
}

#define pcb_ordc_ABORT  goto yyabort
#define pcb_ordc_REJECT goto yyabort
#define pcb_ordc_ACCEPT goto yyaccept
#define pcb_ordc_ERROR  goto yyerrlab

int pcb_ordc_parse_init(pcb_ordc_yyctx_t *yyctx)
{
#if pcb_ordc_DEBUG
	const char *yys;

	if ((yys = getenv("pcb_ordc_DEBUG")) != 0) {
		yyctx->yyn = *yys;
		if (yyctx->yyn >= '0' && yyctx->yyn <= '9')
			yyctx->debug = yyctx->yyn - '0';
	}
#endif

	memset(&yyctx->val, 0, sizeof(yyctx->val));
	memset(&yyctx->lval, 0, sizeof(yyctx->lval));

	yyctx->yym = 0;
	yyctx->yyn = 0;
	yyctx->nerrs = 0;
	yyctx->errflag = 0;
	yyctx->chr = pcb_ordc_EMPTY;
	yyctx->state = 0;

	memset(&yyctx->stack, 0, sizeof(yyctx->stack));

	yyctx->stack_max_depth = pcb_ordc_INITSTACKSIZE > 10000 ? pcb_ordc_INITSTACKSIZE : 10000;
	if (yyctx->stack.s_base == NULL && pcb_ordc_growstack(yyctx, &yyctx->stack) == pcb_ordc_ENOMEM)
		return -1;
	yyctx->stack.s_mark = yyctx->stack.s_base;
	yyctx->stack.l_mark = yyctx->stack.l_base;
	yyctx->state = 0;
	*yyctx->stack.s_mark = 0;
	yyctx->jump = 0;
	return 0;
}


#define pcb_ordc_GETCHAR(labidx) \
do { \
	if (used)               { yyctx->jump = labidx; return pcb_ordc_RES_NEXT; } \
	getchar_ ## labidx:;    yyctx->chr = tok; yyctx->lval = *lval; used = 1; \
} while(0)

pcb_ordc_res_t pcb_ordc_parse(pcb_ordc_yyctx_t *yyctx, pcb_ordc_ctx_t *ctx, int tok, pcb_ordc_STYPE *lval)
{
	int used = 0;
#if pcb_ordc_DEBUG
	const char *yys;
#endif

yyloop:;
	if (yyctx->jump == 1) { yyctx->jump = 0; goto getchar_1; }
	if (yyctx->jump == 2) { yyctx->jump = 0; goto getchar_2; }

	if ((yyctx->yyn = pcb_ordc_defred[yyctx->state]) != 0)
		goto yyreduce;
	if (yyctx->chr < 0) {
		pcb_ordc_GETCHAR(1);
		if (yyctx->chr < 0)
			yyctx->chr = pcb_ordc_EOF;
#if pcb_ordc_DEBUG
		if (yyctx->debug) {
			if ((yys = pcb_ordc_name[pcb_ordc_TRANSLATE(yyctx->chr)]) == NULL)
				yys = pcb_ordc_name[pcb_ordc_UNDFTOKEN];
			printf("yyctx->debug: state %d, reading %d (%s)\n", yyctx->state, yyctx->chr, yys);
		}
#endif
	}
	if (((yyctx->yyn = pcb_ordc_sindex[yyctx->state]) != 0) && (yyctx->yyn += yyctx->chr) >= 0 && yyctx->yyn <= pcb_ordc_TABLESIZE && pcb_ordc_check[yyctx->yyn] == (pcb_ordc_int_t) yyctx->chr) {
#if pcb_ordc_DEBUG
		if (yyctx->debug)
			printf("yyctx->debug: state %d, shifting to state %d\n", yyctx->state, pcb_ordc_table[yyctx->yyn]);
#endif
		if (yyctx->stack.s_mark >= yyctx->stack.s_last && pcb_ordc_growstack(yyctx, &yyctx->stack) == pcb_ordc_ENOMEM)
			goto yyoverflow;
		yyctx->state = pcb_ordc_table[yyctx->yyn];
		*++yyctx->stack.s_mark = pcb_ordc_table[yyctx->yyn];
		*++yyctx->stack.l_mark = yyctx->lval;
		yyctx->chr = pcb_ordc_EMPTY;
		if (yyctx->errflag > 0)
			--yyctx->errflag;
		goto yyloop;
	}
	if (((yyctx->yyn = pcb_ordc_rindex[yyctx->state]) != 0) && (yyctx->yyn += yyctx->chr) >= 0 && yyctx->yyn <= pcb_ordc_TABLESIZE && pcb_ordc_check[yyctx->yyn] == (pcb_ordc_int_t) yyctx->chr) {
		yyctx->yyn = pcb_ordc_table[yyctx->yyn];
		goto yyreduce;
	}
	if (yyctx->errflag != 0)
		goto yyinrecovery;

	pcb_ordc_error(ctx, yyctx->lval, "syntax error");

	goto yyerrlab;	/* redundant goto avoids 'unused label' warning */
yyerrlab:
	++yyctx->nerrs;

yyinrecovery:
	if (yyctx->errflag < 3) {
		yyctx->errflag = 3;
		for(;;) {
			if (((yyctx->yyn = pcb_ordc_sindex[*yyctx->stack.s_mark]) != 0) && (yyctx->yyn += pcb_ordc_ERRCODE) >= 0 && yyctx->yyn <= pcb_ordc_TABLESIZE && pcb_ordc_check[yyctx->yyn] == (pcb_ordc_int_t) pcb_ordc_ERRCODE) {
#if pcb_ordc_DEBUG
				if (yyctx->debug)
					printf("yyctx->debug: state %d, error recovery shifting to state %d\n", *yyctx->stack.s_mark, pcb_ordc_table[yyctx->yyn]);
#endif
				if (yyctx->stack.s_mark >= yyctx->stack.s_last && pcb_ordc_growstack(yyctx, &yyctx->stack) == pcb_ordc_ENOMEM)
					goto yyoverflow;
				yyctx->state = pcb_ordc_table[yyctx->yyn];
				*++yyctx->stack.s_mark = pcb_ordc_table[yyctx->yyn];
				*++yyctx->stack.l_mark = yyctx->lval;
				goto yyloop;
			}
			else {
#if pcb_ordc_DEBUG
				if (yyctx->debug)
					printf("yyctx->debug: error recovery discarding state %d\n", *yyctx->stack.s_mark);
#endif
				if (yyctx->stack.s_mark <= yyctx->stack.s_base)
					goto yyabort;
				--yyctx->stack.s_mark;
				--yyctx->stack.l_mark;
			}
		}
	}
	else {
		if (yyctx->chr == pcb_ordc_EOF)
			goto yyabort;
#if pcb_ordc_DEBUG
		if (yyctx->debug) {
			if ((yys = pcb_ordc_name[pcb_ordc_TRANSLATE(yyctx->chr)]) == NULL)
				yys = pcb_ordc_name[pcb_ordc_UNDFTOKEN];
			printf("yyctx->debug: state %d, error recovery discards token %d (%s)\n", yyctx->state, yyctx->chr, yys);
		}
#endif
		yyctx->chr = pcb_ordc_EMPTY;
		goto yyloop;
	}

yyreduce:
#if pcb_ordc_DEBUG
	if (yyctx->debug)
		printf("yyctx->debug: state %d, reducing by rule %d (%s)\n", yyctx->state, yyctx->yyn, pcb_ordc_rule[yyctx->yyn]);
#endif
	yyctx->yym = pcb_ordc_len[yyctx->yyn];
	if (yyctx->yym > 0)
		yyctx->val = yyctx->stack.l_mark[1 - yyctx->yym];
	else
		memset(&yyctx->val, 0, sizeof yyctx->val);

	switch (yyctx->yyn) {
case 6:
#line 95 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = unop(PCB_ORDC_NEG, yyctx->stack.l_mark[0].un.tree); }
break;
case 7:
#line 96 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = yyctx->stack.l_mark[-1].un.tree; }
break;
case 8:
#line 97 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = binop(PCB_ORDC_EQ, yyctx->stack.l_mark[-2].un.tree, yyctx->stack.l_mark[0].un.tree); }
break;
case 9:
#line 98 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = binop(PCB_ORDC_GE, yyctx->stack.l_mark[-2].un.tree, yyctx->stack.l_mark[0].un.tree); }
break;
case 10:
#line 100 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = binop(PCB_ORDC_ADD, yyctx->stack.l_mark[-2].un.tree, yyctx->stack.l_mark[0].un.tree); }
break;
case 11:
#line 101 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = binop(PCB_ORDC_ADD, yyctx->stack.l_mark[-2].un.tree, yyctx->stack.l_mark[0].un.tree); }
break;
case 12:
#line 103 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = NULL; }
break;
case 13:
#line 104 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = NULL; }
break;
case 14:
#line 105 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = NULL; }
break;
case 15:
#line 106 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = NULL; }
break;
case 16:
#line 111 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = yyctx->stack.l_mark[-1].un.tree; }
break;
case 17:
#line 114 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = NULL; }
break;
case 18:
#line 118 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = NULL; }
break;
#line 412 "../src_plugins/order/const_gram.c"
	}
	yyctx->stack.s_mark -= yyctx->yym;
	yyctx->state = *yyctx->stack.s_mark;
	yyctx->stack.l_mark -= yyctx->yym;
	yyctx->yym = pcb_ordc_lhs[yyctx->yyn];
	if (yyctx->state == 0 && yyctx->yym == 0) {
#if pcb_ordc_DEBUG
		if (yyctx->debug)
			printf("yyctx->debug: after reduction, shifting from state 0 to state %d\n", pcb_ordc_FINAL);
#endif
		yyctx->state = pcb_ordc_FINAL;
		*++yyctx->stack.s_mark = pcb_ordc_FINAL;
		*++yyctx->stack.l_mark = yyctx->val;
		if (yyctx->chr < 0) {
			pcb_ordc_GETCHAR(2);
			if (yyctx->chr < 0)
				yyctx->chr = pcb_ordc_EOF;
#if pcb_ordc_DEBUG
			if (yyctx->debug) {
				if ((yys = pcb_ordc_name[pcb_ordc_TRANSLATE(yyctx->chr)]) == NULL)
					yys = pcb_ordc_name[pcb_ordc_UNDFTOKEN];
				printf("yyctx->debug: state %d, reading %d (%s)\n", pcb_ordc_FINAL, yyctx->chr, yys);
			}
#endif
		}
		if (yyctx->chr == pcb_ordc_EOF)
			goto yyaccept;
		goto yyloop;
	}
	if (((yyctx->yyn = pcb_ordc_gindex[yyctx->yym]) != 0) && (yyctx->yyn += yyctx->state) >= 0 && yyctx->yyn <= pcb_ordc_TABLESIZE && pcb_ordc_check[yyctx->yyn] == (pcb_ordc_int_t) yyctx->state)
		yyctx->state = pcb_ordc_table[yyctx->yyn];
	else
		yyctx->state = pcb_ordc_dgoto[yyctx->yym];
#if pcb_ordc_DEBUG
	if (yyctx->debug)
		printf("yyctx->debug: after reduction, shifting from state %d to state %d\n", *yyctx->stack.s_mark, yyctx->state);
#endif
	if (yyctx->stack.s_mark >= yyctx->stack.s_last && pcb_ordc_growstack(yyctx, &yyctx->stack) == pcb_ordc_ENOMEM)
		goto yyoverflow;
	*++yyctx->stack.s_mark = (pcb_ordc_int_t) yyctx->state;
	*++yyctx->stack.l_mark = yyctx->val;
	goto yyloop;

yyoverflow:
	pcb_ordc_error(ctx, yyctx->lval, "yacc stack overflow");

yyabort:
	pcb_ordc_freestack(&yyctx->stack);
	return pcb_ordc_RES_ABORT;

yyaccept:
	pcb_ordc_freestack(&yyctx->stack);
	return pcb_ordc_RES_DONE;
}
