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

#include "footprint.h"

#include "unit.h"
#include "error.h"
#include "data.h"
#include "board.h"
#include "obj_elem.h"

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
		fprintf(f, "begin footprint v1 %s\n", element->Name->TextString);
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
			fprintf(f, "term %s %s signal %s\n", pnum, pnum, pad->Name);
			fprintf(f, "polygon %s copper %s ---\n", lloc, pnum);
		}
		PCB_END_LOOP;
		fprintf(f, "end footprint\n");
		break;
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
