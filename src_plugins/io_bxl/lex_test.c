#include <stdio.h>
#include <string.h>

#include "bxl.h"
#include "bxl_lex.h"
#include "bxl_gram.h"

int verbose = 0;

void pcb_bxl_error(pcb_bxl_ctx_t *ctx, pcb_bxl_STYPE tok, const char *s)
{
	fprintf(stderr, "%s\n", s);
}


int main(int argc, char *argv[])
{
	int chr;
	pcb_bxl_ureglex_t ctx;

	pcb_bxl_lex_init(&ctx, pcb_bxl_rules);

	while((chr = fgetc(stdin)) > 0) {
		int res = pcb_bxl_lex_char(&ctx, NULL, chr);
		if (res == UREGLEX_MORE)
			continue;
		if ((res >= 0) && verbose) {
			int n;
			printf("TOKEN: rule %d\n", res);
			for(n = 0; n < ctx.num_rules; n++) {
				ureglex_t *s = &ctx.state[n];
				printf(" %sres=%d", res == n ? "*" : " ", s->exec_state);
				if (s->exec_state > 0) {
					printf(" '%s' '%s'", s->bopat[0], s->eopat[0]);
					printf(" at %ld:%ld\n", ctx.loc_line[0], ctx.loc_col[0]);
				}
				else
					printf("\n");
			}
/*			printf(" buf=%d/'%s'\n", ctx.buff_used, ctx.buff);*/
		}
		else if (res < 0)
			printf("ERROR %d\n", res);
		pcb_bxl_lex_reset(&ctx);
	}

	return 0;
}
