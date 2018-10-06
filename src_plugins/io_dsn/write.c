/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  dsn IO plugin - file format write
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include "plug_io.h"
#include "error.h"
#include "pcb_bool.h"
#include "board.h"

#include "write.h"

typedef struct {
	FILE *f;
	pcb_board_t *pcb;
} dsn_write_t;


int dsn_write_board(dsn_write_t *wctx)
{
	const char *s;

	fprintf(wctx->f, "(pcb ");
	if ((wctx->pcb->Name != NULL) && (*wctx->pcb->Name != '\0')) {
		for(s = wctx->pcb->Name; *s != '\0'; s++) {
			if (isalnum(*s))
				fputc(*s, wctx->f);
			else
				fputc('_', wctx->f);
		}
		fputc('\n', wctx->f);
	}
	else
		fprintf(wctx->f, "anon\n");

	fprintf(wctx->f, "  (parser\n");
	fprintf(wctx->f, "    (string_quote \")\n");
	fprintf(wctx->f, "    (space_in_quoted_tokens on)\n");
	fprintf(wctx->f, "    (host_cad \"pcb-rnd/io_dsn\")\n");
	fprintf(wctx->f, "    (host_version \"%s\")\n", PCB_VERSION);
	fprintf(wctx->f, "  )\n");

	fprintf(wctx->f, ")\n");
}


int io_dsn_write_pcb(pcb_plug_io_t *ctx, FILE *FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	dsn_write_t wctx;

	wctx.f = FP;
	wctx.pcb = PCB;

	return dsn_write_board(&wctx);
}
