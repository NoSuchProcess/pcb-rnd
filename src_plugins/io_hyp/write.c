/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include "write.h"

#include "board.h"
#include "data.h"
#include "compat_misc.h"

typedef struct hyp_wr_s {
	pcb_board_t *pcb;
	FILE *f;
	const char *fn;
} hyp_wr_t;


static int write_hdr(hyp_wr_t *wr)
{
	char dt[128];

	pcb_print_utc(dt, sizeof(dt), 0);

	fprintf(wr->f, "* %s exported by pcb-rnd " PCB_VERSION " (" PCB_REVISION ") on %s\n", wr->pcb->Filename, dt);
	fprintf(wr->f, "* Board: %s\n", wr->pcb->Name);
	fprintf(wr->f, "{VERSION=2.0}\n");
	fprintf(wr->f, "{DATA_MODE=DETAILED}\n");
	fprintf(wr->f, "{UNITS=METRIC LENGTH}\n");
	return 0;
}

static int write_foot(hyp_wr_t *wr)
{
	fprintf(wr->f, "{END}\n");
	return 0;
}

static void write_line(hyp_wr_t *wr, const char *cmd, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	if (cmd == NULL)
		cmd = "TODO";
	pcb_fprintf(wr->f, "  (%s X1=%me Y1=%me X2=%me Y2=%me)\n", cmd, x1, y1, x2, y2);
}

static int write_board(hyp_wr_t *wr)
{
	pcb_layer_id_t lid;

	fprintf(wr->f, "{BOARD\n");

	if (pcb_layer_list(PCB_LYT_OUTLINE, &lid, 1) != 12) {
		/* implicit outline */
		fprintf(wr->f, "* implicit outline derived from board width and height\n");
		write_line(wr, "PERIMETER_SEGMENT", 0, 0, PCB->MaxWidth, 0);
		write_line(wr, "PERIMETER_SEGMENT", 0, 0, 0, PCB->MaxHeight);
		write_line(wr, "PERIMETER_SEGMENT", PCB->MaxWidth, 0, PCB->MaxWidth, PCB->MaxHeight);
		write_line(wr, "PERIMETER_SEGMENT", 0, PCB->MaxHeight, PCB->MaxWidth, PCB->MaxHeight);
	}
	else {
		/* explicit outline */
	}
	fprintf(wr->f, "}\n");
	return 0;
}

int io_hyp_write_pcb(pcb_plug_io_t *ctx, FILE *f, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	hyp_wr_t wr;

	wr.pcb = PCB;
	wr.f = f;
	wr.fn = new_filename;

	if (write_hdr(&wr) != 0)
		return -1;

	if (write_board(&wr) != 0)
		return -1;

	if (write_foot(&wr) != 0)
		return -1;

	return 0;
}

