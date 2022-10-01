
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
    0,    5,    5,    5,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    4,    6,    6,    2,    3,
};
static const pcb_ordc_int_t pcb_ordc_len[] = {                     2,
    1,    1,    1,    1,    2,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    1,    1,    1,    2,
    4,    4,    4,    3,    0,    2,    5,    7,
};
static const pcb_ordc_int_t pcb_ordc_defred[] = {                  0,
    0,    0,    0,    0,    2,    3,    4,    0,    1,    0,
    0,    0,   26,   17,   18,    0,   19,    0,    0,    0,
    0,    0,    0,    0,   24,    0,    0,    0,    5,    0,
   20,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    6,    0,    0,    0,
    0,    0,    0,    0,    0,   15,   16,   27,    0,   22,
   21,   23,    0,   28,
};
static const pcb_ordc_int_t pcb_ordc_dgoto[] = {                   4,
   23,    5,    6,    7,    8,    9,
};
static const pcb_ordc_int_t pcb_ordc_sindex[] = {               -121,
  -26,  -25, -121,    0,    0,    0,    0, -121,    0,  -36,
 -245, -107,    0,    0,    0,  -20,    0,  -18,  -17,  -36,
  -36, -237,  -35,  -14,    0,  -36,  -36,  -36,    0,  -24,
    0,  -36,  -36,  -36,  -36,  -36,  -36,  -36,  -36,  -36,
  -36, -121, -234,  -16,   -8,    1,    0,   56,   56,   56,
   56,   56,   56,  -32,  -42,    0,    0,    0,  -13,    0,
    0,    0,  -28,    0,
};
static const pcb_ordc_int_t pcb_ordc_rindex[] = {                 32,
    0,    0, -107,    0,    0,    0,    0,    5,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   25,   31,   37,
   43,   49,   55,    9,   17,    0,    0,    0,    0,    0,
    0,    0,    0,    0,
};
static const pcb_ordc_int_t pcb_ordc_gindex[] = {                  0,
   77,    0,    0,    0,   -6,    4,
};
#define pcb_ordc_TABLESIZE 325
static const pcb_ordc_int_t pcb_ordc_table[] = {                  22,
   40,    3,   41,   21,   25,   42,   12,   40,   20,   41,
   40,   13,   41,   10,   11,   24,   47,   25,   40,   26,
   41,   27,   28,   31,   60,   59,   40,   63,   41,   43,
   64,   25,   61,    0,   40,   58,   41,    0,    0,    0,
    0,   62,    0,   40,    0,   41,    0,    0,    0,   13,
    0,    0,    0,    0,    0,    0,    0,   14,    0,    0,
    0,    0,    0,    0,    0,    7,    0,    0,    0,    0,
    0,    8,    0,    0,    0,    0,    0,    9,    0,    0,
    0,    0,    0,   10,    0,    0,    0,    0,    0,   11,
    0,    0,    0,    0,    0,   12,   29,   30,   40,    0,
   41,    0,   44,   45,   46,    0,    0,    0,   48,   49,
   50,   51,   52,   53,   54,   55,   56,   57,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   25,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    1,    2,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   14,   15,   16,   17,    0,   38,   32,   33,   34,   35,
   36,   37,   38,   39,    0,   18,   19,   32,   33,   34,
   35,   36,   37,   38,   39,   32,   33,   34,   35,   36,
   37,   38,   39,   32,   33,   34,   35,   36,   37,   38,
   39,    0,   32,   33,   34,   35,   36,   37,   38,   39,
   13,   13,   13,   13,   13,   13,   13,   13,   14,   14,
   14,   14,   14,   14,    0,   14,    7,    7,    7,    7,
    7,    7,    8,    8,    8,    8,    8,    8,    9,    9,
    9,    9,    9,    9,   10,   10,   10,   10,   10,   10,
   11,   11,   11,   11,   11,   11,   12,   12,   12,   12,
   12,   12,    0,   38,   39,
};
static const pcb_ordc_int_t pcb_ordc_check[] = {                  36,
   43,  123,   45,   40,    0,   41,    3,   43,   45,   45,
   43,    8,   45,   40,   40,  261,   41,  125,   43,   40,
   45,   40,   40,  261,   41,  260,   43,   41,   45,   44,
   59,    0,   41,   -1,   43,   42,   45,   -1,   -1,   -1,
   -1,   41,   -1,   43,   -1,   45,   -1,   -1,   -1,   41,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   41,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   41,   -1,   -1,   -1,   -1,
   -1,   41,   -1,   -1,   -1,   -1,   -1,   41,   -1,   -1,
   -1,   -1,   -1,   41,   -1,   -1,   -1,   -1,   -1,   41,
   -1,   -1,   -1,   -1,   -1,   41,   20,   21,   43,   -1,
   45,   -1,   26,   27,   28,   -1,   -1,   -1,   32,   33,
   34,   35,   36,   37,   38,   39,   40,   41,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  125,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  270,  271,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  257,  258,  259,  260,   -1,  268,  262,  263,  264,  265,
  266,  267,  268,  269,   -1,  272,  273,  262,  263,  264,
  265,  266,  267,  268,  269,  262,  263,  264,  265,  266,
  267,  268,  269,  262,  263,  264,  265,  266,  267,  268,
  269,   -1,  262,  263,  264,  265,  266,  267,  268,  269,
  262,  263,  264,  265,  266,  267,  268,  269,  262,  263,
  264,  265,  266,  267,   -1,  269,  262,  263,  264,  265,
  266,  267,  262,  263,  264,  265,  266,  267,  262,  263,
  264,  265,  266,  267,  262,  263,  264,  265,  266,  267,
  262,  263,  264,  265,  266,  267,  262,  263,  264,  265,
  266,  267,   -1,  268,  269,
};
#define pcb_ordc_FINAL 4
#define pcb_ordc_MAXTOKEN 274
#define pcb_ordc_UNDFTOKEN 283
#define pcb_ordc_TRANSLATE(a) ((a) > pcb_ordc_MAXTOKEN ? pcb_ordc_UNDFTOKEN : (a))
#if pcb_ordc_DEBUG
static const char *const pcb_ordc_name[] = {

"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"'!'",0,0,"'$'","'%'",0,0,"'('","')'","'*'","'+'","','","'-'",0,"'/'",0,0,0,0,0,
0,0,0,0,0,0,"';'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'{'",0,"'}'",0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,"T_CINT","T_CFLOAT","T_STRING","T_QSTR","T_ID","T_EQ",
"T_NEQ","T_GE","T_LE","T_GT","T_LT","T_AND","T_OR","T_IF","T_ERROR","T_INT",
"T_FLOAT","T_STR",0,0,0,0,0,0,0,0,"illegal-symbol",
};
static const char *const pcb_ordc_rule[] = {
"$accept : file",
"file : statements",
"statement : stmt_if",
"statement : stmt_error",
"statement : stmt_block",
"expr : '-' expr",
"expr : '(' expr ')'",
"expr : expr T_EQ expr",
"expr : expr T_NEQ expr",
"expr : expr T_GE expr",
"expr : expr T_LE expr",
"expr : expr T_GT expr",
"expr : expr T_LT expr",
"expr : expr T_AND expr",
"expr : expr T_OR expr",
"expr : expr '+' expr",
"expr : expr '-' expr",
"expr : T_CINT",
"expr : T_CFLOAT",
"expr : T_QSTR",
"expr : '$' T_ID",
"expr : T_INT '(' expr ')'",
"expr : T_STRING '(' expr ')'",
"expr : T_FLOAT '(' expr ')'",
"stmt_block : '{' statements '}'",
"statements :",
"statements : statement statements",
"stmt_if : T_IF '(' expr ')' statement",
"stmt_error : T_ERROR '(' T_ID ',' T_QSTR ')' ';'",

};
#endif

#line 137 "../src_plugins/order/const_gram.y"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static pcb_ordc_node_t *new_node(pcb_ordc_node_type_t ty)
{
	pcb_ordc_node_t *n = calloc(sizeof(pcb_ordc_node_t), 1);
	n->type = ty;
	return n;
}

static pcb_ordc_node_t *binop(pcb_ordc_node_type_t ty, pcb_ordc_node_t *a, pcb_ordc_node_t *b)
{
	pcb_ordc_node_t *n = new_node(ty);
	assert((a == NULL) || (a->next == NULL));
	assert((b == NULL) || (b->next == NULL));
	n->ch_first = a;
	if (a != NULL)
		a->next = b;
	return n;
}

#define unop(ty, a) binop(ty, a, NULL)

static pcb_ordc_node_t *id2node(char *s)
{
	pcb_ordc_node_t *n = new_node(PCB_ORDC_ID);
	n->val.s = s;
	return n;
}

static pcb_ordc_node_t *qstr2node(char *s)
{
	pcb_ordc_node_t *n = new_node(PCB_ORDC_QSTR);
	n->val.s = s;
	return n;
}

static pcb_ordc_node_t *var2node(char *s)
{
	pcb_ordc_node_t *n = new_node(PCB_ORDC_VAR);
	n->val.s = s;
	return n;
}

static pcb_ordc_node_t *int2node(long l)
{
	pcb_ordc_node_t *n = new_node(PCB_ORDC_CINT);
	n->val.l = l;
	return n;
}

static pcb_ordc_node_t *float2node(double d)
{
	pcb_ordc_node_t *n = new_node(PCB_ORDC_CFLOAT);
	n->val.d = d;
	return n;
}
#line 255 "../src_plugins/order/const_gram.c"

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
case 1:
#line 84 "../src_plugins/order/const_gram.y"
	{ ctx->root->ch_first = yyctx->stack.l_mark[0].un.tree; }
break;
case 2:
#line 88 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = yyctx->stack.l_mark[0].un.tree; }
break;
case 3:
#line 89 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = yyctx->stack.l_mark[0].un.tree; }
break;
case 4:
#line 90 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = yyctx->stack.l_mark[0].un.tree; }
break;
case 5:
#line 95 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = unop(PCB_ORDC_NEG, yyctx->stack.l_mark[0].un.tree); }
break;
case 6:
#line 96 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = yyctx->stack.l_mark[-1].un.tree; }
break;
case 7:
#line 97 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = binop(PCB_ORDC_EQ, yyctx->stack.l_mark[-2].un.tree, yyctx->stack.l_mark[0].un.tree); }
break;
case 8:
#line 98 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = binop(PCB_ORDC_NEQ, yyctx->stack.l_mark[-2].un.tree, yyctx->stack.l_mark[0].un.tree); }
break;
case 9:
#line 99 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = binop(PCB_ORDC_GE, yyctx->stack.l_mark[-2].un.tree, yyctx->stack.l_mark[0].un.tree); }
break;
case 10:
#line 100 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = binop(PCB_ORDC_LE, yyctx->stack.l_mark[-2].un.tree, yyctx->stack.l_mark[0].un.tree); }
break;
case 11:
#line 101 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = binop(PCB_ORDC_GT, yyctx->stack.l_mark[-2].un.tree, yyctx->stack.l_mark[0].un.tree); }
break;
case 12:
#line 102 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = binop(PCB_ORDC_LT, yyctx->stack.l_mark[-2].un.tree, yyctx->stack.l_mark[0].un.tree); }
break;
case 13:
#line 103 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = binop(PCB_ORDC_AND, yyctx->stack.l_mark[-2].un.tree, yyctx->stack.l_mark[0].un.tree); }
break;
case 14:
#line 104 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = binop(PCB_ORDC_OR, yyctx->stack.l_mark[-2].un.tree, yyctx->stack.l_mark[0].un.tree); }
break;
case 15:
#line 106 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = binop(PCB_ORDC_ADD, yyctx->stack.l_mark[-2].un.tree, yyctx->stack.l_mark[0].un.tree); }
break;
case 16:
#line 107 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = binop(PCB_ORDC_SUB, yyctx->stack.l_mark[-2].un.tree, yyctx->stack.l_mark[0].un.tree); }
break;
case 17:
#line 109 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = int2node(yyctx->stack.l_mark[0].un.i); }
break;
case 18:
#line 110 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = float2node(yyctx->stack.l_mark[0].un.d); }
break;
case 19:
#line 111 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = qstr2node(yyctx->stack.l_mark[0].un.s); }
break;
case 20:
#line 112 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = var2node(yyctx->stack.l_mark[0].un.s); }
break;
case 21:
#line 114 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = unop(PCB_ORDC_INT, yyctx->stack.l_mark[-1].un.tree); }
break;
case 22:
#line 115 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = unop(PCB_ORDC_STRING, yyctx->stack.l_mark[-1].un.tree); }
break;
case 23:
#line 116 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = unop(PCB_ORDC_FLOAT, yyctx->stack.l_mark[-1].un.tree); }
break;
case 24:
#line 121 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = new_node(PCB_ORDC_BLOCK); yyctx->val.un.tree->ch_first = yyctx->stack.l_mark[-1].un.tree; }
break;
case 25:
#line 124 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = NULL; }
break;
case 26:
#line 125 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = yyctx->stack.l_mark[-1].un.tree; yyctx->val.un.tree->next = yyctx->stack.l_mark[0].un.tree; }
break;
case 27:
#line 129 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = binop(PCB_ORDC_IF, yyctx->stack.l_mark[-2].un.tree, yyctx->stack.l_mark[0].un.tree); }
break;
case 28:
#line 133 "../src_plugins/order/const_gram.y"
	{ yyctx->val.un.tree = binop(PCB_ORDC_ERROR, id2node(yyctx->stack.l_mark[-4].un.s), qstr2node(yyctx->stack.l_mark[-2].un.s)); }
break;
#line 572 "../src_plugins/order/const_gram.c"
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
