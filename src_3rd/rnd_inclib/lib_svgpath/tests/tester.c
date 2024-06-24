/*
    svg path parser for Ringdove - unit test program
    Copyright (C) 2024 Tibor 'Igor2' Palinkas

    (Supported by NLnet NGI0 Entrust Fund in 2024)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

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
	cfg.curve_approx_seglen = 5;
	fout = stdout;

	fprintf(fout, "<?xml version=\"1.0\"?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.0\" width=\"1000\" height=\"1000\" viewBox=\"0 0 1000 1000\">\n");
	fprintf(fout, "<path d=\"%s\" style=\"stroke:green;stroke-width:3;fill:none;\" />\n\n\n", path);

	res = svgpath_render(&cfg, fout, path);

	fprintf(fout, "\n<!-- res=%d -->\n", res);
	fprintf(fout, "</svg>\n");

	return res;
}
