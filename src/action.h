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
 *  RCS: $Id$
 */

/* prototypes for action routines
 */

#ifndef	PCB_ACTION_H
#define	PCH_ACTION_H

#include "global.h"

#define CLONE_TYPES LINE_TYPE | ARC_TYPE | VIA_TYPE | POLYGON_TYPE

void ActionAdjustStyle(char *);
void EventMoveCrosshair(int, int);

void AdjustAttachedObjects(void);

void warpNoWhere(void);

/* In gui-misc.c */
bool ActionGetLocation(char *);
void ActionGetXY(char *);

#define action_entry(x) F_ ## x,
typedef enum {
#include "action_funclist.h"
F_END
} FunctionID;
#undef action_entry

#include "action_funchash.h"

#define ARG(n) (argc > (n) ? argv[n] : NULL)

int get_style_size(int funcid, Coord * out, int type, int size_id);

extern int defer_updates;
extern int defer_needs_update;
extern LayerTypePtr lastLayer;

void NotifyLine(void);
void NotifyBlock(void);
void NotifyMode(void);
void ClearWarnings(void);

typedef struct {
	Coord X, Y;
	Cardinal Buffer;
	bool Click;
	bool Moving;									/* selected type clicked on */
	int Hit;											/* move type clicked on */
	void *ptr1;
	void *ptr2;
	void *ptr3;
} action_note_t;

extern action_note_t Note;
extern bool saved_mode;

int ActionExecuteFile(int argc, char **argv, Coord x, Coord y);
void ReleaseMode(void);

#endif
