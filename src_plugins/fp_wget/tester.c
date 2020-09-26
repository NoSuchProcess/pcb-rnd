#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "gedasymbols.h"

#undef strdup
char *rnd_strdup(const char *s) { return strdup(s); }

pcb_plug_fp_t *pcb_plug_fp_chain = NULL;

pcb_library_t ltmp;
pcb_library_t *pcb_fp_mkdir_p(const char *path)
{
	printf("lib mkdir: '%s'\n", path);
	return (library_t *)&ltmp;
}

pcb_library_t *pcb_fp_append_entry(library_t *parent, const char *name, pcb_fp_type_t type, void *tags[], rnd_bool dup_tags)
{
	printf("lib entry: '%s'\n", name);
	return (library_t *)&ltmp;
}

int main()
{
	pcb_fp_fopen_ctx_t fctx;
	FILE *f;
	char line[1024];

/*	fp_gedasymbols_load_dir(NULL, "gedasymbols://", 1); */
	f = fp_gedasymbols_fopen(NULL, NULL, "wget@gedasymbols/user/sean_depagnier/footprints/HDMI_CONN.fp", &fctx);

	while(fgets(line, sizeof(line), f) != NULL)
		printf("|%s", line);

	fp_gedasymbols_fclose(NULL, f, &fctx);

}

