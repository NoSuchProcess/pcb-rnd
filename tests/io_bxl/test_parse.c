#include <stdio.h>
#include <string.h>

#include "bxl.h"
#include "bxl_lex.h"
#include "bxl_gram.h"

int verbose = 0;

void pcb_bxl_error(pcb_bxl_ctx_t *ctx, pcb_bxl_STYPE tok, const char *s)
{
	fprintf(stderr, "(ignored) %s at %ld:%ld\n", s, tok.line, tok.first_col);
}


int main(int argc, char *argv[])
{
	int chr, bad = 0;
	pcb_bxl_ureglex_t lctx;
	pcb_bxl_yyctx_t yyctx;
	pcb_bxl_ctx_t bctx;

	pcb_bxl_lex_init(&lctx, pcb_bxl_rules);
	pcb_bxl_parse_init(&yyctx);

	while((chr = fgetc(stdin)) > 0) {
		pcb_bxl_STYPE lval;
		int res = pcb_bxl_lex_char(&lctx, &lval, chr);
		if (res == UREGLEX_MORE)
			continue;
		if (res >= 0) {
			pcb_bxl_res_t yres;

			printf("token: %d ", res);
			lval.line = lctx.loc_line[0];
			lval.first_col = lctx.loc_col[0];
			yres = pcb_bxl_parse(&yyctx, &bctx, res, &lval);
			if ((bctx.in_error) && ((res == T_ID) || (res == T_QSTR)))
				free(lval.un.s);
			
			printf("yres=%d\n", yres);
			if (yres != 0) {
				printf("Stopped at %ld:%ld\n", lval.line, lval.first_col);
				bad = 1;
				break;
			}
		}
		if ((res >= 0) && verbose) {
			int n;
			printf("TOKEN: rule %d\n", res);
			for(n = 0; n < lctx.num_rules; n++) {
				ureglex_t *s = &lctx.state[n];
				printf(" %sres=%d", res == n ? "*" : " ", s->exec_state);
				if (s->exec_state > 0) {
					printf(" '%s' '%s'", s->bopat[0], s->eopat[0]);
					printf(" at %ld:%ld\n", lctx.loc_line[0], lctx.loc_col[0]);
				}
				else
					printf("\n");
			}
/*			printf(" buf=%d/'%s'\n", ctx.buff_used, ctx.buff);*/
		}
		else if (res < 0)
			printf("ERROR %d\n", res);
		pcb_bxl_lex_reset(&lctx);
	}

	if (bad)
		printf("Parse failed.\n");

	return 0;
}
