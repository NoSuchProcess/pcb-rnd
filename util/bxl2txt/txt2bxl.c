/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  BXL IO plugin - file format write, encoder
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2020)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* test program: read plain text on stdin, encode to bxl to the named file */

#include <stdio.h>
#include "bxl_decode.h"

void out(FILE *f, hdecode_t *ctx, int len)
{
	int n;
	for(n = 0; n < len; n++)
		fputc(ctx->out[n], f);
}

int main(int argc, char *argv[])
{
	hdecode_t ctx;
	int inch, len, fd;
	FILE *f;

	if (argc < 2) {
		fprintf(stderr, "Need an output file name\n");
		return 1;
	}

	f = fopen(argv[1], "wb");
	if (f == NULL) {
		fprintf(stderr, "Failed to open %s for write\n", argv[1]);
		return 1;
	}

	pcb_bxl_encode_init(&ctx);

	while((inch = fgetc(stdin)) != EOF)
		out(f, &ctx, pcb_bxl_encode_char(&ctx, inch));
	out(f, &ctx, pcb_bxl_encode_eof(&ctx));

	fseek(f, 0, SEEK_SET);
	out(f, &ctx, pcb_bxl_encode_len(&ctx));

	return 0;
}
