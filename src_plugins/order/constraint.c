#include "config.h"
#include <stdlib.h>
#include "const_gram.h"
#include "constraint.h"
#include "const_gram.h"
#include "const_lex.h"

/* Error is handled on the push side */
void pcb_ordc_error(pcb_ordc_ctx_t *ctx, pcb_ordc_STYPE tok, const char *s) { }

int pcb_ordc_parse_verbose = 0;

int pcb_ordc_parse_str(pcb_ordc_ctx_t *ctx, const char *script)
{
	pcb_ordc_yyctx_t yyctx;
	pcb_ordc_ctx_t octx;
	pcb_ordc_STYPE lval;
	pcb_ordc_ureglex_t lctx;
	int tok, yres;

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
		yres = pcb_ordc_parse(&yyctx, &octx, tok, &lval);

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

	return 0;

	error:;
TODO("free octx tree");
	return -1;
}
