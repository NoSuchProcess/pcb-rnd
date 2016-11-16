/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

#include "config.h"
#include <stroke.h>
#include "math_helper.h"
#include "board.h"
#include "conf.h"
#include "conf_core.h"
#include "data.h"
#include "crosshair.h"
#include "stub_stroke.h"
#include "rotate.h"
#include "undo.h"
#include "error.h"
#include "plugins.h"
#include "compat_nls.h"

void FinishStroke(void);

pcb_box_t StrokeBox;

/* ---------------------------------------------------------------------------
 * FinishStroke - try to recognize the stroke sent
 */
static void real_stroke_finish(void)
{
	char msg[255];
	unsigned long num;

	pcb_mid_stroke = pcb_false;
	if (stroke_trans(msg)) {
		num = atoi(msg);
		switch (num) {
		case 456:
			if (conf_core.editor.mode == PCB_MODE_LINE) {
				pcb_crosshair_set_mode(PCB_MODE_LINE);
			}
			break;
		case 9874123:
		case 74123:
		case 987412:
		case 8741236:
		case 874123:
			pcb_screen_obj_rotate90(StrokeBox.X1, StrokeBox.Y1, SWAP_IDENT ? 1 : 3);
			break;
		case 7896321:
		case 786321:
		case 789632:
		case 896321:
			pcb_screen_obj_rotate90(StrokeBox.X1, StrokeBox.Y1, SWAP_IDENT ? 3 : 1);
			break;
		case 258:
			pcb_crosshair_set_mode(PCB_MODE_LINE);
			break;
		case 852:
			pcb_crosshair_set_mode(PCB_MODE_ARROW);
			break;
		case 1478963:
			ActionUndo(0, NULL, 0, 0);
			break;
		case 147423:
		case 147523:
		case 1474123:
			pcb_redo(pcb_true);
			break;
		case 148963:
		case 147863:
		case 147853:
		case 145863:
			pcb_crosshair_set_mode(PCB_MODE_VIA);
			break;
		case 951:
		case 9651:
		case 9521:
		case 9621:
		case 9851:
		case 9541:
		case 96521:
		case 96541:
		case 98541:
			PCB->Zoom = 1000;						/* special zoom extents */
			break;
		case 159:
		case 1269:
		case 1259:
		case 1459:
		case 1569:
		case 1589:
		case 12569:
		case 12589:
		case 14589:
			{
				pcb_coord_t x = (StrokeBox.X1 + StrokeBox.X2) / 2;
				pcb_coord_t y = (StrokeBox.Y1 + StrokeBox.Y2) / 2;
				double z;
				/* XXX: PCB->MaxWidth and PCB->MaxHeight may be the wrong
				 *      divisors below. The old code WAS broken, but this
				 *      replacement has not been tested for correctness.
				 */
				z = 1 + log(fabs(StrokeBox.X2 - StrokeBox.X1) / PCB->MaxWidth) / log(2.0);
				z = MAX(z, 1 + log(fabs(StrokeBox.Y2 - StrokeBox.Y1) / PCB->MaxHeight) / log(2.0));
				PCB->Zoom = z;

				pcb_center_display(x, y);
				break;
			}

		default:
			pcb_message(PCB_MSG_DEFAULT, _("Unknown stroke %s\n"), msg);
			break;
		}
	}
	else
		gui->beep();
}

static void real_stroke_record(int ev_x, int ev_y)
{
	StrokeBox.X2 = ev_x;
	StrokeBox.Y2 = ev_y;
	fprintf(stderr, "stroke: %d %d\n", ev_x >> 16, ev_y >> 16);
	stroke_record(ev_x >> 16, ev_y >> 16);
	return;
}

static void real_stroke_start(void)
{
	fprintf(stderr, "stroke: MIID!\n");
	pcb_mid_stroke = pcb_true;
	StrokeBox.X1 = Crosshair.X;
	StrokeBox.Y1 = Crosshair.Y;
}

pcb_uninit_t hid_stroke_init(void)
{
	stroke_init();

	pcb_stub_stroke_record = real_stroke_record;
	pcb_stub_stroke_start = real_stroke_start;
	pcb_stub_stroke_finish = real_stroke_finish;
	return NULL;
}
