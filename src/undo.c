/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* functions used to undo operations
 *
 * Description:
 * There are two lists which hold
 *   - the uundo list (information about a command)
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
#include <libuundo/uundo_debug.h>

#include "board.h"
#include "change.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "event.h"
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

#include "obj_poly_draw.h"

static pcb_bool between_increment_and_restore = pcb_false;
static pcb_bool added_undo_between_increment_and_restore = pcb_false;

pcb_data_t *pcb_removelist = NULL; /* lists of removed objects */
static pcb_bool Locked = pcb_false; /* do not add entries if */
pcb_bool pcb_undo_and_draw = pcb_true; /* flag is set; prevents from infinite loops */
uundo_list_t pcb_uundo; /* only the undo dialog box should have access to it */

void *pcb_undo_alloc(pcb_board_t *pcb, const uundo_oper_t *oper, size_t data_len)
{
	return uundo_append(&pcb_uundo, oper, data_len);
}

int pcb_undo(pcb_bool draw)
{
	int res;

	pcb_undo_and_draw = draw;

	if (pcb_uundo.num_undo == 0) {
		pcb_message(PCB_MSG_INFO, "Nothing to undo - buffer is empty\n");
		return -1;
	}

	if (pcb_uundo.serial == 0) {
		pcb_message(PCB_MSG_ERROR, "ERROR: Attempt to pcb_undo() with Serial == 0\n       Please save your work and report this bug.\n");
		return -1;
	}

	if ((pcb_uundo.tail != NULL) && (pcb_uundo.tail->serial > pcb_uundo.serial)) {
		pcb_message(PCB_MSG_ERROR, "ERROR: Bad undo serial number %d in undo stack - expecting %d or lower\n"
							"       Please save your work and report this bug.\n", pcb_uundo.tail->serial, pcb_uundo.serial);

	/* It is likely that the serial number got corrupted through some bad
		 * use of the pcb_undo_save_serial() / pcb_undo_restore_serial() APIs.
		 *
		 * Reset the serial number to be consistent with that of the last
		 * operation on the undo stack in the hope that this might clear
		 * the problem and  allow the user to hit Undo again.
		 */
		pcb_uundo.serial = pcb_uundo.tail->serial + 1;
		return -1;
	}

	pcb_undo_lock(); /* lock undo module to prevent from loops */
	res = uundo_undo(&pcb_uundo);
	pcb_undo_unlock();

	if (res != 0)
		pcb_message(PCB_MSG_ERROR, "ERROR: Failed to undo some operations\n");
	else if (pcb_undo_and_draw)
		pcb_draw();

	pcb_event(PCB_EVENT_UNDO_POST, "i", PCB_UNDO_EV_UNDO);

	return res;
}

int pcb_undo_above(uundo_serial_t s_min)
{
	return uundo_undo_above(&pcb_uundo, s_min);
}

int pcb_redo(pcb_bool draw)
{
	int res;

	pcb_undo_and_draw = draw;

	if (pcb_uundo.num_redo == 0) {
		pcb_message(PCB_MSG_INFO, "Nothing to redo. Perhaps changes have been made since last undo\n");
		return 0;
	}

	if ((pcb_uundo.tail != NULL) && (pcb_uundo.tail->next != NULL) && (pcb_uundo.tail->next->serial > pcb_uundo.serial)) {

		pcb_message(PCB_MSG_ERROR, "ERROR: Bad undo serial number %d in redo stack - expecting %d or higher\n"
							"       Please save your work and report this bug.\n", pcb_uundo.tail->next->serial, pcb_uundo.serial);

		/* It is likely that the serial number got corrupted through some bad
		 * use of the pcb_undo_save_serial() / pcb_undo_restore_serial() APIs.
		 *
		 * Reset the serial number to be consistent with that of the first
		 * operation on the redo stack in the hope that this might clear
		 * the problem and  allow the user to hit Redo again.
		 */

		pcb_uundo.serial = pcb_uundo.tail->next->serial;
		return 0;
	}

	pcb_undo_lock(); /* lock undo module to prevent from loops */
	uundo_freeze_add(&pcb_uundo);
	res = uundo_redo(&pcb_uundo);
	uundo_unfreeze_add(&pcb_uundo);
	pcb_undo_unlock();

	if (res != 0)
		pcb_message(PCB_MSG_ERROR, "ERROR: Failed to redo some operations\n");
	else if (pcb_undo_and_draw)
		pcb_draw();

	pcb_event(PCB_EVENT_UNDO_POST, "i", PCB_UNDO_EV_REDO);

	return res;
}

/* ---------------------------------------------------------------------------
 * restores the serial number of the undo list
 */
void pcb_undo_restore_serial(void)
{
	if (added_undo_between_increment_and_restore)
		pcb_message(PCB_MSG_ERROR, "ERROR: Operations were added to the Undo stack with an incorrect serial number\n");
	between_increment_and_restore = pcb_false;
	added_undo_between_increment_and_restore = pcb_false;
	uundo_restore_serial(&pcb_uundo);
}

/* ---------------------------------------------------------------------------
 * saves the serial number of the undo list
 */
void pcb_undo_save_serial(void)
{
	pcb_bumped = pcb_false;
	between_increment_and_restore = pcb_false;
	added_undo_between_increment_and_restore = pcb_false;
	uundo_save_serial(&pcb_uundo);
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
		if ((pcb_uundo.tail != NULL) && (pcb_uundo.tail->serial == pcb_uundo.serial))
			pcb_board_set_changed_flag(pcb_true);

		uundo_inc_serial(&pcb_uundo);
		pcb_bumped = pcb_true;
		between_increment_and_restore = pcb_true;
	}
}

/* ---------------------------------------------------------------------------
 * releases memory of the undo- and remove list
 */
void pcb_undo_clear_list(pcb_bool Force)
{
	if (pcb_uundo.num_undo && (Force || pcb_hid_message_box("warning", "clear undo buffer", "Do you reall want to clear 'undo' buffer?", "yes", 1, "no", 0, NULL) == 1)) {
		uundo_list_clear(&pcb_uundo);
		pcb_event(PCB_EVENT_UNDO_POST, "i", PCB_UNDO_EV_CLEAR_LIST);
	}
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
	return Locked;
}

uundo_serial_t pcb_undo_serial(void)
{
	return pcb_uundo.serial;
}


void pcb_undo_truncate_from(uundo_serial_t sfirst)
{
	uundo_list_truncate_from(&pcb_uundo, sfirst);
	pcb_event(PCB_EVENT_UNDO_POST, "i", PCB_UNDO_EV_TRUNCATE);
}

int undo_check(void)
{
	const char *res = uundo_check(&pcb_uundo, NULL);

#ifndef NDEBUG
	if (res != NULL) {
		printf("Undo broken: %s\n", res);
		uundo_dump(&pcb_uundo, NULL, NULL);
	}
#endif

	return (res != NULL);
}

#ifndef NDEBUG
void undo_dump(void)
{
	uundo_dump(&pcb_uundo, NULL, NULL);
}
#endif

size_t pcb_num_undo(void)
{
	return pcb_uundo.num_undo;
}

void pcb_undo_freeze_serial(void)
{
	uundo_freeze_serial(&pcb_uundo);
}

void pcb_undo_unfreeze_serial(void)
{
	uundo_unfreeze_serial(&pcb_uundo);
}


void pcb_undo_freeze_add(void)
{
	uundo_freeze_add(&pcb_uundo);
}

void pcb_undo_unfreeze_add(void)
{
	uundo_unfreeze_add(&pcb_uundo);
}

