/*
 * RenumberBlock plug-in for PCB.
 *
 * Copyright (C) 2006 DJ Delorie <dj@delorie.com>
 *
 * Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Original source: http://www.delorie.com/pcb/renumberblock.c
 *
 * Usage: RenumberBlock(oldnum,newnum)
 *
 * All selected elements are renumbered by adding (newnum-oldnum) to
 * the existing number.  I.e. RenumberBlock(100,200) will change R213
 * to R313.
 *
 * Usage: RenumberBuffer(oldnum,newnum)
 *
 * Same, but the paste buffer is renumbered.
 */

#include <stdio.h>
#include <math.h>

#include "config.h"
#include "board.h"
#include "data.h"
#include "hid.h"
#include "rtree.h"
#include "undo.h"
#include "error.h"
#include "change.h"
#include "conf_core.h"

int pcb_act_RenumberBlock(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	char num_buf[15];
	int old_base, new_base;

	if (argc < 2) {
		pcb_message(PCB_MSG_ERROR, "Usage: RenumberBlock oldnum newnum");
		return 1;
	}

	old_base = atoi(argv[0]);
	new_base = atoi(argv[1]);

	conf_set_editor(name_on_pcb, 1);

#warning subc TODO: rewrite
#if 0
	PCB_ELEMENT_LOOP(PCB->Data);
	{
		char *refdes_split, *cp;
		char *old_ref, *new_ref;
		int num;

		if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, element))
			continue;

		old_ref = element->Name[1].TextString;
		for (refdes_split = cp = old_ref; *cp; cp++)
			if (!isdigit(*cp))
				refdes_split = cp + 1;

		num = atoi(refdes_split);
		num += (new_base - old_base);
		sprintf(num_buf, "%d", num);
		new_ref = (char *) malloc(refdes_split - old_ref + strlen(num_buf) + 1);
		memcpy(new_ref, old_ref, refdes_split - old_ref);
		strcpy(new_ref + (refdes_split - old_ref), num_buf);

		pcb_undo_add_obj_to_change_name(PCB_TYPE_ELEMENT, NULL, NULL, element, PCB_ELEM_NAME_REFDES(element));

		pcb_chg_obj_name(PCB_TYPE_ELEMENT, element, NULL, NULL, new_ref);
	}
	PCB_END_LOOP;
#endif
	pcb_undo_inc_serial();
	return 0;
}

int pcb_act_RenumberBuffer(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	char num_buf[15];
	int old_base, new_base;

	if (argc < 2) {
		pcb_message(PCB_MSG_ERROR, "Usage: RenumberBuffer oldnum newnum");
		return 1;
	}

	old_base = atoi(argv[0]);
	new_base = atoi(argv[1]);

	conf_set_editor(name_on_pcb, 1);

#warning subc TODO: rewrite
#if 0
	PCB_ELEMENT_LOOP(PCB_PASTEBUFFER->Data);
	{
		char *refdes_split, *cp;
		char *old_ref, *new_ref;
		int num;

		old_ref = element->Name[1].TextString;
		for (refdes_split = cp = old_ref; *cp; cp++)
			if (!isdigit(*cp))
				refdes_split = cp + 1;

		num = atoi(refdes_split);
		num += (new_base - old_base);
		sprintf(num_buf, "%d", num);
		new_ref = (char *) malloc(refdes_split - old_ref + strlen(num_buf) + 1);
		memcpy(new_ref, old_ref, refdes_split - old_ref);
		strcpy(new_ref + (refdes_split - old_ref), num_buf);

		pcb_chg_obj_name(PCB_TYPE_ELEMENT, element, NULL, NULL, new_ref);
	}
	PCB_END_LOOP;
#endif
	return 0;
}
