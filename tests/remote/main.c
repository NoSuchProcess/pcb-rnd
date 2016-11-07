#include <string.h>
#include "base64.c"
#include "proto_lowcommon.h"
#include "proto_lowparse.h"
#include "proto_lowdbg.h"

int main()
{
	int line = 1, col = 1;
	proto_ctx_t pctx;

	memset(&pctx, 0, sizeof(pctx));

	for(;;) {
		parse_res_t res;
		int c = getchar();
		if (c == EOF)
			return -1;
		if (c == '\n') {
			col = 1;
			line++;
		}
		else
			col++;
		res = parse_char(&pctx, c);
		switch(res) {
			case PRES_ERR:
				printf("parse error at %d:%d\n", line, col);
				return 1;
			case PRES_GOT_MSG:
				printf("msg '%s'\n", pctx.pcmd);
				proto_node_print(pctx.targ, 1);
				printf("\n");
				break;
			case PRES_PROCEED:
				break;
		}
	}
}
