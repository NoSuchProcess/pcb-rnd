#include "ucdf.h"
int main(int argc, char *argv[])
{
	char *fn = "A.PcbDoc";
	ucdf_file_t ctx = {0};

	if (argc > 1)
		fn = argv[1];

	if (ucdf_open(&ctx, fn) != 0) {
		printf("error reading %s: %d (%s)\n", fn, ctx.error, ucdf_error_str[ctx.error]);
		return -1;
	}

	printf("CDF header:\n");
	printf("  ver=%04x rev=%04x %s-endian sect_size=%d,%d\n", ctx.file_ver, ctx.file_rev, ctx.litend ? "little" : "big", ctx.sect_size, ctx.short_sect_size);
	printf("  sat: %ld+%ld ssat=%ld+%ld msat=%ld+%ld\n", ctx.sat_first, ctx.sat_len, ctx.ssat_first, ctx.ssat_len, ctx.msat_first, ctx.msat_len);

	return 0;
}
