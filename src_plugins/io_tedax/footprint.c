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
#include <genht/htsp.h>
#include <genht/hash.h>

#include "footprint.h"

#include "unit.h"
#include "error.h"
#include "data.h"
#include "board.h"
#include "pcb-printf.h"
#include "compat_misc.h"
#include "obj_elem.h"
#include "obj_line.h"
#include "obj_arc.h"
#include "obj_pad.h"
#include "obj_pinvia.h"

static void print_sqpad_coords(FILE *f, pcb_pad_t *Pad, pcb_coord_t cx, pcb_coord_t cy)
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

	pcb_fprintf(f, " %.9mm %.9mm", (pcb_coord_t)(x1 - vx * width + nx * width) - cx, (pcb_coord_t)(y1 - vy * width + ny * width) - cy);
	pcb_fprintf(f, " %.9mm %.9mm", (pcb_coord_t)(x1 - vx * width - nx * width) - cx, (pcb_coord_t)(y1 - vy * width - ny * width) - cy);
	pcb_fprintf(f, " %.9mm %.9mm", (pcb_coord_t)(x2 + vx * width + nx * width) - cx, (pcb_coord_t)(y2 + vy * width + ny * width) - cy);
	pcb_fprintf(f, " %.9mm %.9mm", (pcb_coord_t)(x2 + vx * width - nx * width) - cx, (pcb_coord_t)(y2 + vy * width - ny * width) - cy);
}

#define elem_layer(elem, obj) \
	((PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, (elem)) == PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, (obj))) ? "primary" : "secondary")

#define safe_term_num(out, obj, buff) \
do { \
	out = obj->Number; \
	if ((out == NULL) || (*out == '\0')) { \
		sprintf(buff, "%p", (void *)obj); \
		out = buff; \
		if (out[1] == 'x') \
			out+=2; \
	} \
} while(0)

#define print_term(num, obj) \
do { \
	if (htsp_get(&terms, pnum) == NULL) { \
		htsp_set(&terms, pcb_strdup(pnum), obj); \
		fprintf(f, "	term %s %s - %s\n", pnum, pnum, obj->Name); \
	} \
} while(0)

int tedax_fp_save(pcb_data_t *data, const char *fn)
{
	FILE *f;
	char buff[64];
	htsp_t terms;
	htsp_entry_t *e;

	f = fopen(fn, "w");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't open %s for writing\n", fn);
		return -1;
	}

	htsp_init(&terms, strhash, strkeyeq);

	fprintf(f, "tEDAx v1\n");

	PCB_ELEMENT_LOOP(data)
	{
		fprintf(f, "\nbegin footprint v1 %s\n", element->Name->TextString);
		PCB_PAD_LOOP(element)
		{
			char *lloc, *pnum;
			lloc = elem_layer(element, pad);
			safe_term_num(pnum, pad, buff);
			print_term(pnum, pad);
			if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, pad)) { /* sqaure cap pad -> poly */
				fprintf(f, "	polygon %s copper %s 4", lloc, pnum);
				print_sqpad_coords(f, pad,  element->MarkX, element->MarkY);
				pcb_fprintf(f, " %mm\n", pad->Clearance);
			}
			else { /* round cap pad -> line */
				pcb_fprintf(f, "	line %s copper %s %mm %mm %mm %mm %mm %mm\n", lloc, pnum, pad->Point1.X - element->MarkX, pad->Point1.Y - element->MarkY, pad->Point2.X - element->MarkX, pad->Point2.Y - element->MarkY, pad->Thickness, pad->Clearance);
			}
		}
		PCB_END_LOOP;

		PCB_PIN_LOOP(element)
		{
			char *pnum;
			safe_term_num(pnum, pin, buff);
			print_term(pnum, pin);
			pcb_fprintf(f, "	fillcircle all copper %s %mm %mm %mm %mm\n", pnum, pin->X - element->MarkX, pin->Y - element->MarkY, pin->Thickness/2, pin->Clearance);
			pcb_fprintf(f, "	hole %s %mm %mm %mm\n", pnum, pin->X - element->MarkX, pin->Y - element->MarkY, pin->DrillingHole/2);
		}
		PCB_END_LOOP;

		PCB_ELEMENT_PCB_LINE_LOOP(element)
		{
			char *lloc= elem_layer(element, line);
			pcb_fprintf(f, "	line %s silk - %mm %mm %mm %mm %mm %mm\n", lloc,
				line->Point1.X - element->MarkX, line->Point1.Y - element->MarkY, line->Point2.X - element->MarkX, line->Point2.Y - element->MarkY, line->Thickness, line->Clearance);
		}
		PCB_END_LOOP;

		PCB_ELEMENT_ARC_LOOP(element)
		{
			char *lloc= elem_layer(element, arc);
			pcb_fprintf(f, "	arc %s silk - %mm %mm %mm %f %f %mm %mm\n", lloc,
				arc->X - element->MarkX, arc->Y - element->MarkY, (arc->Width + arc->Height)/2, arc->StartAngle, arc->Delta,
				arc->Thickness, arc->Clearance);
		}
		PCB_END_LOOP;

		fprintf(f, "end footprint\n");

		for (e = htsp_first(&terms); e; e = htsp_next(&terms, e)) {
			free(e->key);
			htsp_delentry(&terms, e);
		}

	}
	PCB_END_LOOP;

	htsp_uninit(&terms);

	fclose(f);

	return 0;
}
