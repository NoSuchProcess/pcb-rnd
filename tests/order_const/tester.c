#include <stdio.h>
#include <src_plugins/order/constraint.h>

int verbose = 0;
extern int pcb_ordc_parse_verbose;

int main(int argc, char *argv[])
{
	pcb_ordc_ctx_t ctx;
	char script[8192], *fn = argv[1];
	FILE *f;
	int r;

	if (strcmp(argv[1], "-v") == 0) {
		verbose = pcb_ordc_parse_verbose = 1;
		fn = argv[2];
	}

	if (fn == NULL) {
		fprintf(stderr, "Error: need script file name\n");
		return 1;
	}

	f = fopen(fn, "r");
	if (f == NULL) {
		fprintf(stderr, "Error: failed to open the script file (%s)\n", fn);
		return 1;
	}
	fread(script, 1, sizeof(script), f);
	fclose(f);

	r = pcb_ordc_parse_str(&ctx, script);
	if (r != 0) {
		fprintf(stderr, "Error: failed to parse the script (exiting)\n");
		return 1;
	}


	return 0;
}
