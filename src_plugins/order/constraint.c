#include "config.h"
#include <stdlib.h>
#include <stdio.h>
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

