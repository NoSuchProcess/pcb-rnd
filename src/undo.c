/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* functions used to undo operations
 *
 * Description:
 * There are two lists which hold
 *   - information about a command
 *   - data of removed objects
 * Both lists are organized as first-in-last-out which means that the undo
 * list can always use the last entry of the remove list.
 * A serial number is incremented whenever an operation is completed.
 * An operation itself may consist of several basic instructions.
 * E.g.: removing all selected objects is one operation with exactly one
 * serial number even if the remove function is called several times.
 *
 * a lock flag ensures that no infinite loops occur
 */

#include "config.h"

#include <assert.h>
#include <libuundo/uundo.h>
#include <libuundo/uundo_debug.h>

#include "board.h"
#include "change.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "insert.h"
#include "polygon.h"
#include "remove.h"
#include "rotate.h"
#include "search.h"
#include "undo.h"
#include "undo_old.h"
#include "flag_str.h"
#include "conf_core.h"
#include "compat_misc.h"
#include "compat_nls.h"

#include "obj_elem_draw.h"
#include "obj_poly_draw.h"

#define STEP_REMOVELIST 500
#define STEP_UNDOLIST   500

static pcb_bool between_increment_and_restore = pcb_false;
static pcb_bool added_undo_between_increment_and_restore = pcb_false;

#include "undo_old_str.h"

pcb_data_t *RemoveList = NULL;	/* list of removed objects */
static UndoListTypePtr UndoList = NULL;	/* list of operations */
static int Serial = 1,					/* serial number */
	SavedSerial;
static size_t UndoN, RedoN,			/* number of entries */
  UndoMax;
static pcb_bool Locked = pcb_false;			/* do not add entries if */
pcb_bool pcb_undo_and_draw = pcb_true;
										/* flag is set; prevents from */
										/* infinite loops */

/* ---------------------------------------------------------------------------
 * adds a command plus some data to the undo list
 */
void *GetUndoSlot(int CommandType, int ID, int Kind, size_t item_len)
{
	UndoListTypePtr ptr;
	size_t limit = ((size_t)conf_core.editor.undo_warning_size) * 1024;

#ifdef DEBUG_ID
	if (pcb_search_obj_by_id(PCB->Data, &ptr1, &ptr2, &ptr3, ID, Kind) == PCB_TYPE_NONE)
		pcb_message(PCB_MSG_ERROR, "hace: ID (%d) and Type (%x) mismatch in AddObject...\n", ID, Kind);
#endif

	/* allocate memory */
	if (UndoN >= UndoMax) {
		size_t size;

		UndoMax += STEP_UNDOLIST;
		size = UndoMax * item_len;
		UndoList = (UndoListTypePtr) realloc(UndoList, size);
		memset(&UndoList[UndoN], 0, STEP_REMOVELIST * item_len);

		/* ask user to flush the table because of it's size */
		if (size > limit) {
			size_t l2;
			l2 = (size / limit + 1) * limit;
			pcb_message(PCB_MSG_INFO, _("Size of 'undo-list' exceeds %li kb\n"), (long) (l2 >> 10));
		}
	}

	/* free structures from the pruned redo list */

	for (ptr = &UndoList[UndoN]; RedoN; ptr++, RedoN--)
		pcb_undo_old_free(ptr);

	if (between_increment_and_restore)
		added_undo_between_increment_and_restore = pcb_true;

	/* copy typefield and serial number to the list */
	ptr = &UndoList[UndoN++];
	ptr->Type = CommandType;
	ptr->Kind = Kind;
	ptr->ID = ID;
	ptr->Serial = Serial;
	return (ptr);
}

/* ---------------------------------------------------------------------------
 * undo of any 'hard to recover' operation
 *
 * returns the bitfield for the types of operations that were undone
 */
int pcb_undo(pcb_bool draw)
{
	UndoListTypePtr ptr;
	int Types = 0;
	int unique;
	pcb_bool error_undoing = pcb_false;

	unique = conf_core.editor.unique_names;
	conf_force_set_bool(conf_core.editor.unique_names, 0);

	pcb_undo_and_draw = draw;

	if (Serial == 0) {
		pcb_message(PCB_MSG_ERROR, _("ERROR: Attempt to pcb_undo() with Serial == 0\n" "       Please save your work and report this bug.\n"));
		return 0;
	}

	if (UndoN == 0) {
		pcb_message(PCB_MSG_INFO, _("Nothing to undo - buffer is empty\n"));
		return 0;
	}

	Serial--;

	ptr = &UndoList[UndoN - 1];

	if (ptr->Serial > Serial) {
		pcb_message(PCB_MSG_ERROR, _("ERROR: Bad undo serial number %d in undo stack - expecting %d or lower\n"
							"       Please save your work and report this bug.\n"), ptr->Serial, Serial);

	/* It is likely that the serial number got corrupted through some bad
		 * use of the pcb_undo_save_serial() / pcb_undo_restore_serial() APIs.
		 *
		 * Reset the serial number to be consistent with that of the last
		 * operation on the undo stack in the hope that this might clear
		 * the problem and  allow the user to hit Undo again.
		 */
		Serial = ptr->Serial + 1;
		return 0;
	}

	pcb_undo_lock();										/* lock undo module to prevent from loops */

	/* Loop over all entries with the correct serial number */
	for (; UndoN && ptr->Serial == Serial; ptr--, UndoN--, RedoN++) {
		int undid = pcb_undo_old_perform(ptr);
		if (undid == 0)
			error_undoing = pcb_true;
		Types |= undid;
	}

	pcb_undo_unlock();

	if (error_undoing)
		pcb_message(PCB_MSG_ERROR, _("ERROR: Failed to undo some operations\n"));

	if (Types && pcb_undo_and_draw)
		pcb_draw();

	/* restore the unique flag setting */
	conf_force_set_bool(conf_core.editor.unique_names, unique);

	return Types;
}

/* ---------------------------------------------------------------------------
 * redo of any 'hard to recover' operation
 *
 * returns the number of operations redone
 */
int pcb_redo(pcb_bool draw)
{
	UndoListTypePtr ptr;
	int Types = 0;
	pcb_bool error_undoing = pcb_false;

	pcb_undo_and_draw = draw;

	if (RedoN == 0) {
		pcb_message(PCB_MSG_INFO, _("Nothing to redo. Perhaps changes have been made since last undo\n"));
		return 0;
	}

	ptr = &UndoList[UndoN];

	if (ptr->Serial < Serial) {
		pcb_message(PCB_MSG_ERROR, _("ERROR: Bad undo serial number %d in redo stack - expecting %d or higher\n"
							"       Please save your work and report this bug.\n"), ptr->Serial, Serial);

		/* It is likely that the serial number got corrupted through some bad
		 * use of the pcb_undo_save_serial() / pcb_undo_restore_serial() APIs.
		 *
		 * Reset the serial number to be consistent with that of the first
		 * operation on the redo stack in the hope that this might clear
		 * the problem and  allow the user to hit Redo again.
		 */
		Serial = ptr->Serial;
		return 0;
	}

	pcb_undo_lock();										/* lock undo module to prevent from loops */

	/* and loop over all entries with the correct serial number */
	for (; RedoN && ptr->Serial == Serial; ptr++, UndoN++, RedoN--) {
		int undid = pcb_undo_old_perform(ptr);
		if (undid == 0)
			error_undoing = pcb_true;
		Types |= undid;
	}

	/* Make next serial number current */
	Serial++;

	pcb_undo_unlock();

	if (error_undoing)
		pcb_message(PCB_MSG_ERROR, _("ERROR: Failed to redo some operations\n"));

	if (Types && pcb_undo_and_draw)
		pcb_draw();

	return Types;
}

/* ---------------------------------------------------------------------------
 * restores the serial number of the undo list
 */
void pcb_undo_restore_serial(void)
{
	if (added_undo_between_increment_and_restore)
		pcb_message(PCB_MSG_ERROR, _("ERROR: Operations were added to the Undo stack with an incorrect serial number\n"));
	between_increment_and_restore = pcb_false;
	added_undo_between_increment_and_restore = pcb_false;
	Serial = SavedSerial;
}

/* ---------------------------------------------------------------------------
 * saves the serial number of the undo list
 */
void pcb_undo_save_serial(void)
{
	pcb_bumped = pcb_false;
	between_increment_and_restore = pcb_false;
	added_undo_between_increment_and_restore = pcb_false;
	SavedSerial = Serial;
}

/* ---------------------------------------------------------------------------
 * increments the serial number of the undo list
 * it's not done automatically because some operations perform more
 * than one request with the same serial #
 */
void pcb_undo_inc_serial(void)
{
	if (!Locked) {
		/* Set the changed flag if anything was added prior to this bump */
		if (UndoN > 0 && UndoList[UndoN - 1].Serial == Serial)
			pcb_board_set_changed_flag(pcb_true);
		Serial++;
		pcb_bumped = pcb_true;
		between_increment_and_restore = pcb_true;
	}
}

/* ---------------------------------------------------------------------------
 * releases memory of the undo- and remove list
 */
void pcb_undo_clear_list(pcb_bool Force)
{
	UndoListTypePtr undo;

	if (UndoN && (Force || pcb_gui->confirm_dialog("OK to clear 'undo' buffer?", 0))) {
		/* release memory allocated by objects in undo list */
		for (undo = UndoList; UndoN; undo++, UndoN--)
			pcb_undo_old_free(undo);

		free(UndoList);
		UndoList = NULL;
		if (RemoveList) {
			pcb_data_free(RemoveList);
			free(RemoveList);
			RemoveList = NULL;
		}

		/* reset some counters */
		UndoN = UndoMax = RedoN = 0;
	}

	/* reset counter in any case */
	Serial = 1;
}

/* ---------------------------------------------------------------------------
 * set lock flag
 */
void pcb_undo_lock(void)
{
	Locked = pcb_true;
}

/* ---------------------------------------------------------------------------
 * reset lock flag
 */
void pcb_undo_unlock(void)
{
	Locked = pcb_false;
}

/* ---------------------------------------------------------------------------
 * return undo lock state
 */
pcb_bool pcb_undoing(void)
{
	return (Locked);
}

int undo_check(void)
{
	size_t n;
	int last_serial = -2;
	for(n = 0; n < UndoN; n++) {
		if (last_serial != UndoList[n].Serial) {
			if (last_serial > UndoList[n].Serial) {
#				ifndef NDEBUG
				printf("Undo broken check #1:\n");
				undo_dump();
#				endif
				return 1;
			}
			last_serial = UndoList[n].Serial;
		}
	}
	if (Serial < last_serial) {
#		ifndef NDEBUG
		printf("Undo broken check #2:\n");
		undo_dump();
#		endif
		return 1;
	}
	return 0;
}

#ifndef NDEBUG
void undo_dump(void)
{
	size_t n;
	int last_serial = -2;
	printf("Serial=%d\n", Serial);
	for(n = 0; n < UndoN; n++) {
		if (last_serial != UndoList[n].Serial) {
			printf("--- serial=%d\n", UndoList[n].Serial);
			last_serial = UndoList[n].Serial;
		}
		printf(" type=%s kind=%d ID=%d\n", undo_type2str(UndoList[n].Type), UndoList[n].Kind, UndoList[n].ID);
	}
}
#endif
