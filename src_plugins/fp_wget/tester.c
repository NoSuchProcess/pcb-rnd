#include <stdlib.h>
#include "global.h"
#include "gedasymbols.h"

plug_fp_t *plug_fp_chain = NULL;

int main()
{
	fp_fopen_ctx_t fctx;
	FILE *f;
	char line[1024];

//	fp_gedasymbols_load_dir(NULL, "gedasymbols://");
	f = fp_gedasymbols_fopen(NULL, "user/sean_depagnier/footprints", "HDMI_CONN.fp", &fctx);

	while(fgets(line, sizeof(line), f) != NULL)
		printf("|%s", line);

	fp_gedasymbols_fclose(NULL, f, &fctx);

}

