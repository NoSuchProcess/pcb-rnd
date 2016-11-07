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
		int res, c = getchar();
		if (c == EOF)
			return -1;
		if (c == '\n') {
			col = 1;
			line++;
		}
		else
			col++;
		res = parse_char(&pctx, c);
		if (res < 0)
			printf("parse error at %d:%d\n", line, col);
	}
}
