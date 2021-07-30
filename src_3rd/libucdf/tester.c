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

int main(int argc, char *argv[])
{
	char *fn = "A.PcbDoc";
	ucdf_ctx_t ctx = {0};

	if (argc > 1)
		fn = argv[1];

	if (ucdf_open(&ctx, fn) != 0) {
		printf("error reading %s: %d (%s)\n", fn, ctx.error, ucdf_error_str[ctx.error]);
		return -1;
	}

	printf("CDF header:\n");
	printf("  ver=%04x rev=%04x %s-endian sect_size=%d,%d\n", ctx.file_ver, ctx.file_rev, ctx.litend ? "little" : "big", ctx.sect_size, ctx.short_sect_size);
	printf("  sat_len: %ld; dir1=%ld ssat=%ld+%ld msat=%ld+%ld\n", ctx.sat_len, ctx.dir_first, ctx.ssat_first, ctx.ssat_len, ctx.msat_first, ctx.msat_len);

	print_dir(&ctx, ctx.root, 0);

	return 0;
}
