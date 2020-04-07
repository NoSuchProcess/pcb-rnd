/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  BXL IO plugin - file format read, parser
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

/* test program: read bxl on stdin, unpack to stdout */

#include <stdio.h>
#include "bxl_decode.h"

int main()
{
	hdecode_t ctx;
	int inch;

	pcb_bxl_decode_init(&ctx);

	while((inch = fgetc(stdin)) != EOF) {
		int n, len;
		len = pcb_bxl_decode_char(&ctx, inch);
/*		for(n = 0; n < len; n++)
			printf("%c %d\n", ctx.out[n], ctx.out[n]);*/
		for(n = 0; n < len; n++)
			printf("%c", ctx.out[n]);
	}

/*fprintf(stderr, "plain len=%lu %lu\n", ctx.plain_len, ctx.ipos);*/
/*fprintf(stderr, "pool used: %d\n", ctx.tree->pool_used);*/

	return 0;
}
