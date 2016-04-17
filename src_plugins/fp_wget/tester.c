#include <stdlib.h>
#include "global.h"
#include "gedasymbols.h"

plug_fp_t *plug_fp_chain = NULL;

library_t ltmp;
library_t *fp_mkdir_p(const char *path)
{
	printf("lib mkdir: '%s'\n", path);
	return (library_t *)&ltmp;
}

library_t *fp_append_entry(library_t *parent, const char *name, fp_type_t type, void *tags[])
{
	printf("lib entry: '%s'\n", name);
	return (library_t *)&ltmp;
}

int main()
{
	fp_fopen_ctx_t fctx;
	FILE *f;
	char line[1024];

//	fp_gedasymbols_load_dir(NULL, "gedasymbols://");
	f = fp_gedasymbols_fopen(NULL, NULL, "wget@gedasymbols/user/sean_depagnier/footprints/HDMI_CONN.fp", &fctx);

	while(fgets(line, sizeof(line), f) != NULL)
		printf("|%s", line);

	fp_gedasymbols_fclose(NULL, f, &fctx);

}

