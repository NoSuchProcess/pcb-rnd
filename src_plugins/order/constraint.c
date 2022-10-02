#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/error.h>
#include "const_gram.h"
#include "constraint.h"
#include "const_gram.h"
#include "const_lex.h"

/* Error is handled on the push side */
void pcb_ordc_error(pcb_ordc_ctx_t *ctx, pcb_ordc_STYPE tok, const char *s) { }

int pcb_ordc_parse_verbose = 0;

int pcb_ordc_parse_str(pcb_ordc_ctx_t *octx, const char *script)
{
	pcb_ordc_yyctx_t yyctx;
	pcb_ordc_STYPE lval;
	pcb_ordc_ureglex_t lctx;
	int tok, yres;

	if (octx->root == NULL) {
		octx->root = calloc(sizeof(pcb_ordc_node_t), 1);
		octx->root->type = PCB_ORDC_BLOCK;
	}

	pcb_ordc_lex_init(&lctx, pcb_ordc_rules);
	pcb_ordc_parse_init(&yyctx);

	/* read all bytes of the binary file */
	for(;*script != '\0';script++) {
		/* feed the lexer */
		if (pcb_ordc_parse_verbose) printf("in: '%c'\n", *script);
		tok = pcb_ordc_lex_char(&lctx, &lval, *script);
		if (tok == UREGLEX_MORE)
			continue;
		if (pcb_ordc_parse_verbose) printf(" tok=%d\n", tok);


		/* feed the grammar */
		lval.line = lctx.loc_line[0];
		lval.first_col = lctx.loc_col[0];
		yres = pcb_ordc_parse(&yyctx, octx, tok, &lval);

		if (pcb_ordc_parse_verbose) printf("  yy=%d\n", yres);

#if 0
		/* see io_bxl */
		if ((octx.in_error) && ((tok == T_ID) || (tok == T_QSTR)))
			free(lval.un.s);
#endif

		if (yres != 0) {
			fprintf(stderr, "order constraint script syntax error at %ld:%ld\n", lval.line, lval.first_col);
			goto error;
		}
		pcb_ordc_lex_reset(&lctx); /* prepare for the next token */
	}

	/* flush pending token with an EOF */
	pcb_ordc_parse(&yyctx, octx, EOF, &lval);


	return 0;

	error:;
TODO("free octx tree");
	return -1;
}

static void print_ind(FILE *f, int lev)
{
	for(;lev > 0; lev--)
		fputc(' ', f);
}

void pcb_ordc_print_tree(FILE *f, pcb_ordc_ctx_t *ctx, pcb_ordc_node_t *node, int indlev)
{
	pcb_ordc_node_t *n;

	print_ind(f, indlev);

	switch(node->type) {
		case PCB_ORDC_BLOCK:   printf("block\n"); break;

		case PCB_ORDC_IF:      printf("if()\n"); break;
		case PCB_ORDC_ERROR:   printf("error()\n"); break;

		case PCB_ORDC_CINT:    printf("const int %ld\n", node->val.l); break;
		case PCB_ORDC_CFLOAT:  printf("const float %f\n", node->val.d); break;
		case PCB_ORDC_QSTR:    printf("const qstr '%s'\n", node->val.s); break;
		case PCB_ORDC_ID:      printf("const id '%s'\n", node->val.s); break;
		case PCB_ORDC_VAR:     printf("var '$%s'\n", node->val.s); break;

		case PCB_ORDC_INT:     printf("int()\n"); break;
		case PCB_ORDC_FLOAT:   printf("float()\n"); break;
		case PCB_ORDC_STRING:  printf("string()\n"); break;

		case PCB_ORDC_NEG:     printf("neg\n"); break;
		case PCB_ORDC_EQ:      printf("eq\n"); break;
		case PCB_ORDC_NEQ:     printf("neq\n"); break;
		case PCB_ORDC_GE:      printf("ge\n"); break;
		case PCB_ORDC_LE:      printf("le\n"); break;
		case PCB_ORDC_GT:      printf("gt\n"); break;
		case PCB_ORDC_LT:      printf("lt\n"); break;

		case PCB_ORDC_AND:     printf("and\n"); break;
		case PCB_ORDC_OR:      printf("or\n"); break;
		case PCB_ORDC_NOT:     printf("not\n"); break;

		case PCB_ORDC_ADD:     printf("add\n"); break;
		case PCB_ORDC_SUB:     printf("sub\n"); break;
		case PCB_ORDC_MULT:    printf("mult\n"); break;
		case PCB_ORDC_DIV:     printf("div\n"); break;
		case PCB_ORDC_MOD:     printf("mod\n"); break;

		default:
			printf("UNKNONW %d\n", node->type);
	}

	for(n = node->ch_first; n != NULL; n = n->next)
		pcb_ordc_print_tree(f, ctx, n, indlev+1);
}


void pcb_ordc_free_tree(pcb_ordc_ctx_t *ctx, pcb_ordc_node_t *node)
{
	pcb_ordc_node_t *n, *next;

	switch(node->type) {
		case PCB_ORDC_QSTR:
		case PCB_ORDC_ID:
		case PCB_ORDC_VAR:
			free(node->val.s);
			break;
		default:
			break;
	}

	for(n = node->ch_first; n != NULL; n = next) {
		next = n->next;
		pcb_ordc_free_tree(ctx, n);
	}

	free(node);
}

void pcb_ordc_uninit(pcb_ordc_ctx_t *ctx)
{
	pcb_ordc_free_tree(ctx, ctx->root);
	ctx->root = NULL;
}

/* Returns 0 or 1 for valid bool values, -1 for error */
static int val2bool(pcb_ordc_val_t v)
{
	switch(v.type) {
		case PCB_ORDC_VLNG: return !!v.val.l;
		case PCB_ORDC_VDBL: return v.val.l != 0;
		case PCB_ORDC_VCSTR: return *v.val.s != '\0';
		case PCB_ORDC_VDSTR: return *v.val.s != '\0';
		case PCB_ORDC_VERR: return -1;
	}
	return -1;
}

static void val_free(pcb_ordc_val_t *v)
{
	if (v->type == PCB_ORDC_VDSTR) {
		free(v->val.s);
		v->val.s = NULL;
	}
}

static void conv2bool(pcb_ordc_val_t *dst, pcb_ordc_val_t src)
{
	int b = val2bool(src);
	if (b >= 0) {
		dst->type = PCB_ORDC_VLNG; dst->val.l = b;
	}
	else
		dst->type = PCB_ORDC_VERR;
}

static void conv2lng(pcb_ordc_val_t *dst, pcb_ordc_val_t src)
{
	dst->type = PCB_ORDC_VLNG;

	switch(src.type) {
		case PCB_ORDC_VLNG: dst->val.l = src.val.l; break;
		case PCB_ORDC_VDBL: dst->val.l = rnd_round(src.val.d); break;
		case PCB_ORDC_VCSTR:
		case PCB_ORDC_VDSTR:
			dst->val.l = strtol(src.val.s, NULL, 10);
			break;
		case PCB_ORDC_VERR: dst->type = PCB_ORDC_VERR;
	}
}

static void conv2dbl(pcb_ordc_val_t *dst, pcb_ordc_val_t src)
{
	dst->type = PCB_ORDC_VDBL;

	switch(src.type) {
		case PCB_ORDC_VLNG: dst->val.d = src.val.l; break;
		case PCB_ORDC_VDBL: dst->val.d = src.val.d; break;
		case PCB_ORDC_VCSTR:
		case PCB_ORDC_VDSTR:
			dst->val.d = strtod(src.val.s, NULL);
			break;
		case PCB_ORDC_VERR: dst->type = PCB_ORDC_VERR;
	}
}

static void conv2str(pcb_ordc_val_t *dst, pcb_ordc_val_t src)
{
	dst->type = PCB_ORDC_VDSTR;

	switch(src.type) {
		case PCB_ORDC_VLNG: dst->val.s = rnd_strdup_printf("%ld", src.val.l); break;
		case PCB_ORDC_VDBL: dst->val.s = rnd_strdup_printf("%f", src.val.d); break;
		case PCB_ORDC_VCSTR: dst->val.s = src.val.s; dst->type = PCB_ORDC_VCSTR; break;
		case PCB_ORDC_VDSTR: dst->val.s = rnd_strdup(src.val.s); break;
		case PCB_ORDC_VERR: dst->type = PCB_ORDC_VERR;
	}
}

/* convert two child subtrees to operands, then set binop_str.
   If binop_str is 1, load sa and sb to point to the strings.
   If binop_str is 1, load da and db to numbers loaded from the ops.
   return error early; propagate long to double or anything to string */
#define BINOP_GET_OPS \
	do { \
		c.type = PCB_ORDC_VLNG; \
		binop_str = 0; \
		pcb_ordc_exec_node(ctx, &a, node->ch_first); \
		pcb_ordc_exec_node(ctx, &b, node->ch_first->next); \
		if ((a.type == PCB_ORDC_VERR) || (b.type == PCB_ORDC_VERR)) { \
			dst->type = PCB_ORDC_VERR; \
		} \
		else if ((a.type == PCB_ORDC_VCSTR) || (a.type == PCB_ORDC_VDSTR)) { \
			binop_str = 1; \
			sa = a.val.s; \
			if ((b.type != PCB_ORDC_VCSTR) && (b.type != PCB_ORDC_VDSTR)) { \
				conv2str(&c, b); \
				sb = c.val.s; \
			} \
			else \
				sb = b.val.s; \
		} \
		else if ((b.type == PCB_ORDC_VCSTR) || (b.type == PCB_ORDC_VDSTR)) { \
			binop_str = 1; \
			sb = b.val.s; \
			if ((a.type != PCB_ORDC_VCSTR) && (a.type != PCB_ORDC_VDSTR)) { \
				conv2str(&c, a); \
				sa = c.val.s; \
			} \
			else \
				sa = a.val.s; \
		} \
		else { \
			da = (a.type == PCB_ORDC_VDBL) ? a.val.d : a.val.l; \
			db = (b.type == PCB_ORDC_VDBL) ? b.val.d : b.val.l; \
		} \
	} while(0)


/* Free temporary vals used by BINOP_GET_OPS */
#define BINOP_FREE_OPS  do { val_free(&a); val_free(&b); val_free(&c); } while(0)


/* Convert a and b to double and set dst long val executing code on da and db.
   If any op is string, return error */
#define BINOP_NUMERIC(code) \
	do { \
		BINOP_GET_OPS; \
		if (!binop_str) { code; } \
		else { \
			rnd_message(RND_MSG_ERROR, "order: constraint script error: string in numeric op\n"); \
			dst->type = PCB_ORDC_VERR; \
		} \
		BINOP_FREE_OPS; \
	} while(0)

void pcb_ordc_exec_node(pcb_ordc_ctx_t *ctx, pcb_ordc_val_t *dst, pcb_ordc_node_t *node)
{
	pcb_ordc_node_t *n;
	pcb_ordc_val_t a, b, c;
	const char *sa, *sb;
	double da, db;
	int binop_str, r;

	dst->type = PCB_ORDC_VLNG; dst->val.l = 0;

	switch(node->type) {
		case PCB_ORDC_BLOCK:
			for(n = node->ch_first; n != NULL; n = n->next) {
				pcb_ordc_exec_node(ctx, &a, n);
				if (a.type == PCB_ORDC_VERR)
					dst->type = PCB_ORDC_VERR;
				val_free(&a);
			}
			break;

		case PCB_ORDC_IF:
			pcb_ordc_exec_node(ctx, &a, node->ch_first);
			r = val2bool(a);
			if (r == 1) {
				pcb_ordc_exec_node(ctx, &b, node->ch_first->next);
				val_free(&b);
			}
			else if (r == -1)
				dst->type = PCB_ORDC_VERR;
			val_free(&a);
			break;

		case PCB_ORDC_ERROR:
			if (ctx->error_cb == NULL)
				break;

			pcb_ordc_exec_node(ctx, &a, node->ch_first);
			pcb_ordc_exec_node(ctx, &b, node->ch_first->next);
			ctx->error_cb(ctx, a.val.s, b.val.s, &node->ch_first->ucache); /* no need to convert; grammar ensures static strings */
			val_free(&a);
			val_free(&b);
			break;

		case PCB_ORDC_CINT:
			dst->type = PCB_ORDC_VLNG; dst->val.l = node->val.l;
			break;

		case PCB_ORDC_CFLOAT:
			dst->type = PCB_ORDC_VDBL; dst->val.d = node->val.d;
			break;

		case PCB_ORDC_QSTR:
			dst->type = PCB_ORDC_VCSTR; dst->val.s = node->val.s;
			break;

		case PCB_ORDC_ID:
			dst->type = PCB_ORDC_VCSTR; dst->val.s = node->val.s;
			break;

		case PCB_ORDC_VAR:
			dst->type = PCB_ORDC_VERR; /* assume error and let var_cb override it */

			if (ctx->var_cb == NULL) {
				rnd_message(RND_MSG_ERROR, "order: internal error: no var_cb provided\n");
				break; /* everything evaluates to error */
			}

			ctx->var_cb(ctx, dst, node->val.s, &node->ucache); /* no need to convert; grammar ensures static string */
			if (dst->type == PCB_ORDC_VERR)
				rnd_message(RND_MSG_ERROR, "order: constraint script error: no such variable '%s'\n", node->val.s);
			break;

		case PCB_ORDC_INT:
			pcb_ordc_exec_node(ctx, &a, node->ch_first);
			conv2lng(dst, a);
			val_free(&a);
			break;

		case PCB_ORDC_FLOAT:
			pcb_ordc_exec_node(ctx, &a, node->ch_first);
			conv2dbl(dst, a);
			val_free(&a);
			break;

		case PCB_ORDC_STRING:
			pcb_ordc_exec_node(ctx, &a, node->ch_first);
			conv2str(dst, a);
			val_free(&a);
			break;

		case PCB_ORDC_NEG:
			pcb_ordc_exec_node(ctx, &a, node->ch_first);
			conv2bool(dst, a);
			dst->val.l = !dst->val.l;
			val_free(&a);
			break;

		case PCB_ORDC_EQ:
			BINOP_GET_OPS;
			dst->val.l = binop_str ? (strcmp(sa, sb) == 0) : (da == db);
			BINOP_FREE_OPS;
			break;

		case PCB_ORDC_NEQ:
			BINOP_GET_OPS;
			dst->val.l = binop_str ? (strcmp(sa, sb) != 0) : (da != db);
			BINOP_FREE_OPS;

		case PCB_ORDC_GE:
			BINOP_NUMERIC(dst->val.l = (da >= db));
			break;

		case PCB_ORDC_LE:
			BINOP_NUMERIC(dst->val.l = (da <= db));
			break;

		case PCB_ORDC_GT:
			BINOP_NUMERIC(dst->val.l = (da > db));
			break;

		case PCB_ORDC_LT:
			BINOP_NUMERIC(dst->val.l = (da < db));
			break;

		case PCB_ORDC_AND:
			BINOP_GET_OPS;
			dst->val.l = binop_str ? ((*sa != '\0') && (*sb != '\0')) : (!!da && !!db);
			BINOP_FREE_OPS;
			break;

		case PCB_ORDC_OR:
			BINOP_GET_OPS;
			dst->val.l = binop_str ? ((*sa != '\0') || (*sb != '\0')) : (!!da || !!db);
			BINOP_FREE_OPS;
			break;

		case PCB_ORDC_NOT:
			pcb_ordc_exec_node(ctx, &a, node->ch_first);
			r = val2bool(a);
			if (r >= 0)
				dst->val.l = !r;
			else
				dst->type = PCB_ORDC_VERR;
			val_free(&a);
			break;

		case PCB_ORDC_ADD:
			dst->type = PCB_ORDC_VDBL;
			BINOP_NUMERIC(dst->val.d = da + db);
			break;

		case PCB_ORDC_SUB:
			dst->type = PCB_ORDC_VDBL;
			BINOP_NUMERIC(dst->val.d = da - db);
			break;

		case PCB_ORDC_MULT:
			dst->type = PCB_ORDC_VDBL;
			BINOP_NUMERIC(dst->val.d = da * db);
			break;

		case PCB_ORDC_DIV:
			dst->type = PCB_ORDC_VDBL;
			BINOP_NUMERIC(dst->val.d = (db == 0) ? 0 : (da / db));
			break;

		case PCB_ORDC_MOD:
			dst->type = PCB_ORDC_VDBL;
			BINOP_NUMERIC(dst->val.d = (db == 0) ? 0 : fmod(da, db));
			break;

		default:
			dst->type = PCB_ORDC_VERR;
			rnd_message(RND_MSG_ERROR, "order: internal error: uknown instruction\n");
			break;
	}
}

int pcb_ordc_exec(pcb_ordc_ctx_t *ctx)
{
	pcb_ordc_val_t res;

	pcb_ordc_exec_node(ctx, &res, ctx->root);

	return val2bool(res);
}

