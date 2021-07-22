#include <stdio.h>
#include <string.h>
#include "pcbdoc_ascii.h"

#undef fopen
FILE *rnd_fopen(rnd_hidlib_t *hidlib, const char *fn, const char *mode)
{
	return fopen(fn, mode);
}

long rnd_file_size(rnd_hidlib_t *hidlib, const char *path)
{
	struct stat st;
	if (stat(path, &st) != 0)
		return -1;
	if (st.st_size > LONG_MAX)
		return -1;
	return st.st_size;
}


static void dump_tree(altium_tree_t *tree)
{
	altium_record_t *rec;
	int n;

	for(n = 0; n < altium_kw_record_SPHASH_MAXVAL+1; n++) {
		printf("Records in %s:\n", altium_kw_keyname(n));
		for(rec = gdl_first(&tree->rec[n]); rec != NULL; rec = gdl_next(&tree->rec[n], rec)) {
			altium_field_t *field;
			printf(" %s\n", rec->type_s);
			for(field = gdl_first(&rec->fields); field != NULL; field = gdl_next(&rec->fields, field)) {
				printf("  %s=%s\n", field->key, field->val);
			}
		}
	}
}

int main(int argc, char *argv[])
{
	const char *fn = "A.PcbDoc";
	FILE *f;
	altium_tree_t tree = {0};
	int res;

	if (argc > 1)
		fn = argv[1];

	f = fopen(fn, "r");
	if (f == NULL) {
		fprintf(stderr, "can't open %s for read\n", fn);
		return -1;
	}
	res = pcbdoc_ascii_test_parse(NULL, 0, fn, f);
	fclose(f);

	if (!res) {
		fprintf(stderr, "test parse says '%s' is not a PcbDoc\n", fn);
		return -1;
	}

	if (pcbdoc_ascii_parse_file(NULL, &tree, fn) != 0) {
		altium_tree_free(&tree);
		fprintf(stderr, "failed to parse '%s'\n", fn);
		return -1;
	}


	dump_tree(&tree);

	altium_tree_free(&tree);
	return 0;
}
