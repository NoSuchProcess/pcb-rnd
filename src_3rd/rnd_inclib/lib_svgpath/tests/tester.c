#include <stdio.h>
#include "../svgpath.c"

static void svg_print_line(void *uctx, double x1, double y1, double x2, double y2)
{
	fprintf((FILE *)uctx, "<line x1=\"%.3f\" y1=\"%.3f\" x2=\"%.3f\" y2=\"%.3f\" stroke=\"black\" />\n", x1, y1, x2, y2);
}

static void svg_print_error(void *uctx, const char *errmsg, long offs)
{
	fprintf((FILE *)uctx, "<!-- ERROR %s at %ld -->\n", errmsg, offs);
}


int main()
{
	FILE *fout;
	char path[8192];
	int len, res;
	svgpath_cfg_t cfg = {0};

	len = fread(path, 1, sizeof(path), stdin);
	if (len <= 0) {
		fprintf(stderr, "Failed to read path from stdin\n");
		return 1;
	}

	cfg.line = svg_print_line;
	cfg.error = svg_print_error;
	fout = stdout;

	fprintf(fout, "<?xml version=\"1.0\"?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.0\" width=\"1000\" height=\"1000\" viewBox=\"0 0 1000 1000\">\n");
	fprintf(fout, "<path d=\"%s\" style=\"stroke:green;stroke-width:3\" />\n\n\n", path);

	res = svgpath_render(&cfg, fout, path);

	fprintf(fout, "\n<!-- res=%d -->\n", res);
	fprintf(fout, "</svg>\n");

	return res;
}
