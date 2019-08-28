/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - electric test export
 *  pcb-rnd Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include "config.h"

#include <stdio.h>
#include <assert.h>

#include "board.h"
#include "data.h"
#include "parse.h"
#include "safe_fs.h"
#include "error.h"
#include "tetest.h"
#include "rtree.h"


int tedax_etest_fsave(pcb_board_t *pcb, const char *etestid, FILE *f)
{
	pcb_box_t *b;
	pcb_rtree_it_t it;

	fprintf(f, "begin etest v1 ");
	tedax_fprint_escape(f, etestid);
	fputc('\n', f);

	for(b = pcb_r_first(pcb->Data->padstack_tree, &it); b != NULL; b = pcb_r_next(&it)) {
		pcb_pstk_t *ps = b;
		assert(ps->type == PCB_OBJ_PSTK);
	}
	pcb_r_end(&it);


	fprintf(f, "end etest\n");
	return 0;
}

int tedax_etest_save(pcb_board_t *pcb, const char *etestid, const char *fn)
{
	int res;
	FILE *f;

	f = pcb_fopen_askovr(&PCB->hidlib, fn, "w", NULL);
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "tedax_etest_save(): can't open %s for writing\n", fn);
		return -1;
	}
	fprintf(f, "tEDAx v1\n");
	res = tedax_etest_fsave(pcb, etestid, f);
	fclose(f);
	return res;
}

