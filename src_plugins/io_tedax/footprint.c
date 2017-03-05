/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - footprint import/export
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
#include <stdio.h>
#include <math.h>

#include "footprint.h"

#include "unit.h"
#include "error.h"
#include "data.h"
#include "board.h"
#include "pcb-printf.h"
#include "obj_elem.h"
#include "obj_pad.h"

static void print_sqpad_coords(FILE *f, pcb_pad_t *Pad)
{
	double l, vx, vy, nx, ny, width, x1, y1, x2, y2, dx, dy;

	x1 = Pad->Point1.X;
	y1 = Pad->Point1.Y;
	x2 = Pad->Point2.X;
	y2 = Pad->Point2.Y;

	width = (double)((Pad->Thickness + 1) / 2);
	dx = x2-x1;
	dy = y2-y1;

	if ((dx == 0) && (dy == 0))
		dx = 1;

	l = sqrt((double)dx*(double)dx + (double)dy*(double)dy);

	vx = dx / l;
	vy = dy / l;
	nx = -vy;
	ny = vx;

	pcb_fprintf(f, " %.9mm %.9mm",   (pcb_coord_t)(x1 - vx * width + nx * width), (pcb_coord_t)(y1 - vy * width + ny * width));
	pcb_fprintf(f, " %.9mm %.9mm",   (pcb_coord_t)(x1 - vx * width - nx * width), (pcb_coord_t)(y1 - vy * width - ny * width));
	pcb_fprintf(f, " %.9mm %.9mm",   (pcb_coord_t)(x2 + vx * width + nx * width), (pcb_coord_t)(y2 + vy * width + ny * width));
	pcb_fprintf(f, " %.9mm %.9mm", (pcb_coord_t)(x2 + vx * width - nx * width), (pcb_coord_t)(y2 + vy * width - ny * width));
}


static int fp_save(pcb_data_t *data, const char *fn)
{
	FILE *f;
	f = fopen(fn, "w");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't open %s for writing\n", fn);
		return -1;
	}
	fprintf(f, "tedax v1\n");

	PCB_ELEMENT_LOOP(data)
	{
		fprintf(f, "\nbegin footprint v1 %s\n", element->Name->TextString);
		PCB_PAD_LOOP(element)
		{
			char *lloc, *pnum = pad->Number, buff[64];
			lloc="??";
			if ((pnum == NULL) || (*pnum == '\0')) {
				sprintf(buff, "%p", (void *)pad);
				pnum = buff;
				if (pnum[1] == 'x')
					pnum+=2;
			}
			fprintf(f, "	term %s %s signal %s\n", pnum, pnum, pad->Name);
			if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, pad)) { /* sqaure cap pad -> poly */
				fprintf(f, "	polygon %s copper %s 4", lloc, pnum);
				print_sqpad_coords(f, pad);
				pcb_fprintf(f, " %mm\n", pad->Clearance);
			}
			else { /* round cap pad -> line */
				pcb_fprintf(f, "	line %s copper %s %mm %mm %mm %mm %mm %mm\n", lloc, pnum, pad->Point1.X, pad->Point1.Y, pad->Point2.X, pad->Point2.Y, pad->Thickness, pad->Clearance);
			}
		}
		PCB_END_LOOP;
		fprintf(f, "end footprint\n");
	}
	PCB_END_LOOP;

	fclose(f);

	return 0;
}

const char pcb_acts_Savetedax[] = "SaveTedax(type, filename)";
const char pcb_acth_Savetedax[] = "Saves the specific type of data in a tEDAx file";
int pcb_act_Savetedax(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return fp_save(PCB->Data, argv[1]);
}
