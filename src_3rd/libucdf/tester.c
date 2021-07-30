#include <string.h>
#include "ucdf.h"

static void print_dir(ucdf_ctx_t *ctx, ucdf_direntry_t *dir, int level)
{
	int n;
	ucdf_direntry_t *d;

	for(n = 0; n < level; n++) putc(' ', stdout);
	printf("%s [%d] %c%ld @%ld\n", dir->name, dir->type, (dir->is_short ? 'S' : 'L'), dir->size, dir->first);
	for(d = dir->children; d != NULL; d = d->next)
		print_dir(ctx, d, level+1);
}

/* return the direntry for /name/Data */
static ucdf_direntry_t *de_find(ucdf_ctx_t *ctx, const char *name)
{
	ucdf_direntry_t *d, *n;
	for(d = ctx->root->children; d != NULL; d = d->next) {
		if (strcmp(d->name, name) == 0) {
			for(n = d->children; n != NULL; n = n->next)
				if (strcmp(n->name, "Data") == 0)
					return n;
			return NULL;
		}
	}

	return NULL;
}

static int dump_file(ucdf_ctx_t *ctx, ucdf_direntry_t *de)
{
	ucdf_file_t fp;
	char tmp[1024];
	long len;

	if (ucdf_fopen(ctx, &fp, de) != 0)
		return -1;

	while((len = ucdf_fread(&fp, tmp, sizeof(tmp))) > 0)
		fwrite(tmp, len, 1, stdout);

	return 0;
}

static int print_file_at(ucdf_ctx_t *ctx, ucdf_direntry_t *de, long offs, long len)
{
	ucdf_file_t fp;
	char tmp[1024];

	if (ucdf_fopen(ctx, &fp, de) != 0)
		return -1;

	printf(" seek to %ld: %d\n", offs, ucdf_fseek(&fp, offs));
	printf(" read %ld: %ld\n", len, ucdf_fread(&fp, tmp, len));
	fwrite(tmp, len, 1, stdout);
	printf("\n");
	return 0;
}

int main(int argc, char *argv[])
{
	char *fn = "A.PcbDoc";
	ucdf_ctx_t ctx = {0};

	if (argc > 1)
		fn = argv[1];

	if (ucdf_test_parse(fn) != 0) {
		printf("%s doesn't look like a CDF file\n", fn);
		return -1;
	}


	if (ucdf_open(&ctx, fn) != 0) {
		printf("error reading %s: %d (%s)\n", fn, ctx.error, ucdf_error_str[ctx.error]);
		return -1;
	}

	printf("CDF header:\n");
	printf("  ver=%04x rev=%04x %s-endian sect_size=%d,%d\n", ctx.file_ver, ctx.file_rev, ctx.litend ? "little" : "big", ctx.sect_size, ctx.short_sect_size);
	printf("  sat_len: %ld; dir1=%ld ssat=%ld+%ld msat=%ld+%ld\n", ctx.sat_len, ctx.dir_first, ctx.ssat_first, ctx.ssat_len, ctx.msat_first, ctx.msat_len);

	print_dir(&ctx, ctx.root, 0);

	printf("--NETS:\n");
	{
		ucdf_direntry_t *de = de_find(&ctx, "Nets6");
		print_dir(&ctx, de, 0);
		printf("==\n");
		dump_file(&ctx, de);
		printf("==\n");
		print_file_at(&ctx, de, 39384, 10);
		print_file_at(&ctx, de, 39385, 10);
		print_file_at(&ctx, de, 39386, 10);
		print_file_at(&ctx, de, 39387, 10);
	}

	printf("--Arcs:\n");
	{
		ucdf_direntry_t *de = de_find(&ctx, "Arcs6");
		print_dir(&ctx, de, 0);
		printf("==\n");
		dump_file(&ctx, de);
	}

	ucdf_close(&ctx);
	return 0;
}
