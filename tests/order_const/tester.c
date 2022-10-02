#include <stdio.h>
#include <src_plugins/order/constraint.h>

int verbose = 0;
extern int pcb_ordc_parse_verbose;

static void error_cb(pcb_ordc_ctx_t *ctx, const char *varname, const char *msg, void **ucache)
{
	printf("Constraint error: %s: %s\n", varname, msg);
}

static void var_cb(pcb_ordc_ctx_t *ctx, pcb_ordc_val_t *dst, const char *varname, void **ucache)
{
	if (strcmp(varname, "one") == 0) {
		dst->type = PCB_ORDC_VLNG;
		dst->val.l = 1;
		return;
	}
	if (strcmp(varname, "pi") == 0) {
		dst->type = PCB_ORDC_VDBL;
		dst->val.d = 3.141592654;
		return;
	}
	if (strcmp(varname, "hello") == 0) {
		dst->type = PCB_ORDC_VCSTR;
		dst->val.s = "10 hello world";
		return;
	}
}

int main(int argc, char *argv[])
{
	pcb_ordc_ctx_t ctx = {0};
	char script[8192], *fn = argv[1];
	FILE *f;
	int r;

	ctx.error_cb = error_cb;
	ctx.var_cb = var_cb;

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

	printf("------ AST ------\n");
	if (ctx.root != NULL)
		pcb_ordc_print_tree(stdout, &ctx, ctx.root, 0);
	else
		printf("<no root>\n");


	printf("------ exec ------\n");
	r = pcb_ordc_exec(&ctx);
	if (r < 0)
		printf("Runtime error.\n");

	pcb_ordc_uninit(&ctx);

	return 0;
}
