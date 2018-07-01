/*
 * RenumberBlock plug-in for PCB.
 *
 * Copyright (C) 2006 DJ Delorie <dj@delorie.com>
 * Copyright (C) 2018 Tibor 'Igor2' Palinkas for pcb-rnd data model
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
#include "actions.h"
#include "data.h"
#include "hid.h"
#include "rtree.h"
#include "undo.h"
#include "error.h"
#include "change.h"
#include "conf_core.h"

const char pcb_acts_RenumberBlock[] = "RenumberBlock(old_base,new_base)\n";
const char pcb_acth_RenumberBlock[] = "TODO";

fgw_error_t pcb_act_RenumberBlock(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char num_buf[15];
	int old_base, new_base;

	PCB_ACT_CONVARG(1, FGW_INT, RenumberBlock, old_base = argv[1].val.nat_int);
	PCB_ACT_CONVARG(2, FGW_INT, RenumberBlock, new_base = argv[2].val.nat_int);

	conf_set_editor(name_on_pcb, 1);

	PCB_SUBC_LOOP(PCB->Data);
	{
		char *new_ref;
		const char *old_ref, *cp, *refdes_split;
		int num;

		if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc) || (subc->refdes == NULL))
			continue;

		old_ref = subc->refdes;
		for (refdes_split = cp = old_ref; *cp; cp++)
			if (!isdigit(*cp))
				refdes_split = cp + 1;

		num = atoi(refdes_split);
		num += (new_base - old_base);
		sprintf(num_buf, "%d", num);
		new_ref = (char *) malloc(refdes_split - old_ref + strlen(num_buf) + 1);
		memcpy(new_ref, old_ref, refdes_split - old_ref);
		strcpy(new_ref + (refdes_split - old_ref), num_buf);

		pcb_undo_add_obj_to_change_name(PCB_OBJ_SUBC, NULL, NULL, subc, subc->refdes);

		pcb_chg_obj_name(PCB_OBJ_SUBC, subc, subc, subc, new_ref);
	}
	PCB_END_LOOP;
	pcb_undo_inc_serial();
	PCB_ACT_IRES(0);
	return 0;
}

fgw_error_t pcb_act_RenumberBuffer(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv)
{
	PCB_OLD_ACT_BEGIN;
	char num_buf[15];
	int old_base, new_base;

	if (argc < 2) {
		pcb_message(PCB_MSG_ERROR, "Usage: RenumberBuffer oldnum newnum");
		return 1;
	}

	old_base = atoi(argv[0]);
	new_base = atoi(argv[1]);

	conf_set_editor(name_on_pcb, 1);

	PCB_SUBC_LOOP(PCB_PASTEBUFFER->Data);
	{
		const char *refdes_split, *cp, *old_ref;
		char *new_ref;
		int num;

		if (subc->refdes == NULL)
			continue;

		old_ref = subc->refdes;
		for (refdes_split = cp = old_ref; *cp; cp++)
			if (!isdigit(*cp))
				refdes_split = cp + 1;

		num = atoi(refdes_split);
		num += (new_base - old_base);
		sprintf(num_buf, "%d", num);
		new_ref = (char *) malloc(refdes_split - old_ref + strlen(num_buf) + 1);
		memcpy(new_ref, old_ref, refdes_split - old_ref);
		strcpy(new_ref + (refdes_split - old_ref), num_buf);

		pcb_chg_obj_name(PCB_OBJ_SUBC, subc, subc, subc, new_ref);
	}
	PCB_END_LOOP;

	return 0;
	PCB_OLD_ACT_END;
}
