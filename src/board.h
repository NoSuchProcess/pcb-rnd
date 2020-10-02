/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2004 Thomas Nau
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* definition of types */
#ifndef PCB_BOARD_H
#define PCB_BOARD_H

#include "config.h"

enum {
	PCB_NETLIST_INPUT  = 0,  /* index of the original netlist as imported */
	PCB_NETLIST_EDITED = 1,  /* index of the netlist produced by applying netlist patches on [PCB_NETLIST_INPUT] */
	PCB_NUM_NETLISTS         /* so that we know how many netlists we are dealing with */
};

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include <genht/htsp.h>

#include <librnd/core/global_typedefs.h>
#include "vtroutestyle.h"
#include "layer.h"
#include "layer_grp.h"
#include "attrib.h"
#include "rats_patch.h"
#include "font.h"
#include <librnd/core/hidlib.h>

typedef htsp_t pcb_netlist_t;

/* The pcb_board_t struct holds information about board layout most of which is
   saved with the layout. */
struct pcb_board_s {
	rnd_hidlib_t hidlib;

	long ID;
	char *PrintFilename;                            /* from print dialog */
	char *Netlistname;                              /* name of netlist file */

	rnd_bool Changed;                               /* layout has been changed */

	rnd_bool pstk_on, RatOn, InvisibleObjectsOn,
	         SubcOn, SubcPartsOn,
	         padstack_mark_on, hole_on;             /* visibility flags */
	rnd_bool RatDraw;                               /* we're drawing rats */

	rnd_bool loose_subc;                            /* when set, subc parts are not locked into the subc */

	double ThermScale;                              /* scale factor used with thermals */

	pcb_fontkit_t fontkit;
	pcb_layer_stack_t LayerGroups;
	vtroutestyle_t RouteStyle;
	pcb_netlist_t netlist[PCB_NUM_NETLISTS];
	/* htsp_t netlist_subc[PCB_NUM_NETLISTS]; hierarchic netlists */
	pcb_ratspatch_line_t *NetlistPatches, *NetlistPatchLast;
	pcb_attribute_list_t Attributes;

	pcb_data_t *Data;                               /* entire database */
	rnd_bool is_footprint;                          /* If set, the user has loaded a footprint, not a pcb. */

	const pcb_attribute_list_t *pen_attr;

/* netlist states */
	int netlist_frozen;                             /* counter */
	unsigned netlist_needs_update:1;

	/* delayed trace optimization */
	int line_mod_merge_inhibit;
	struct vtp0_s *line_mod_merge;

	/* random */
	unsigned suppress_warn_missing_font:1;          /* do not warn for missing font; useful for alien formats not having embedded font */
};

extern pcb_board_t *PCB; /* the board being edited */

/* free memory used by a board */
void pcb_board_free(pcb_board_t *pcb);

/* creates a new PCB - low level */
pcb_board_t *pcb_board_new_(rnd_bool SetDefaultNames);

/* creates a new PCB - high level (uses a template board); does not generate
   new board event if inhibit_events is set */
pcb_board_t *pcb_board_new(int inhibit_events);

/* Called after PCB->Data->LayerN is set.  Returns non-zero on error */
int pcb_board_new_postproc(pcb_board_t *pcb, int use_defaults);

/* Set the color field of each layer that does not yet have an explicit
   color, from the config, using default colors per layer type. If
   force is non-zero, overwrite even existing colors. */
void pcb_layer_colors_from_conf(pcb_board_t *pcb, int force);

/* counts the number of plated and unplated holes or slots in the design within
   a given area of the board. To count for the whole board, pass NULL within_area. */
void pcb_board_count_holes(pcb_board_t *pcb, int *plated, int *unplated, const rnd_box_t *within_area);
void pcb_board_count_slots(pcb_board_t *pcb, int *plated, int *unplated, const rnd_box_t *within_area);

#define	PCB_SWAP_X(x)		(RND_SWAP_SIGN_X(x))
#define	PCB_SWAP_Y(y)		(PCB->hidlib.size_y +RND_SWAP_SIGN_Y(y))

#define	PCB_CSWAP_X(x, w, cond)		((cond) ? (RND_SWAP_SIGN_X(x)) : (x))
#define	PCB_CSWAP_Y(y, h, cond)		((cond) ? (h+RND_SWAP_SIGN_Y(y)) : (y))

/* Conditionally allow subc parts to be reached directly in search masks */
#define PCB_LOOSE_SUBC(pcb) ((pcb)->loose_subc ? PCB_OBJ_SUBC_PART : 0)


/* changes the name of a layout; Name is allocated by the caller (no strdup() is made) */
rnd_bool pcb_board_change_name(char *Name);

/* changes the maximum size of a layout, notifies the GUI
 * and adjusts the cursor confinement box */
void pcb_board_resize(rnd_coord_t Width, rnd_coord_t Height, int undoable);


/* free the board and remove its undo list */
void pcb_board_remove(pcb_board_t *Ptr);

/* sets a new line thickness */
void pcb_board_set_line_width(rnd_coord_t Size);

/* sets a new via thickness */
void pcb_board_set_via_size(rnd_coord_t Size, rnd_bool Force);

/* sets a new via drilling hole */
void pcb_board_set_via_drilling_hole(rnd_coord_t Size, rnd_bool Force);

/* sets a clearance width */
void pcb_board_set_clearance(rnd_coord_t Width);

/* sets a text scaling */
void pcb_board_set_text_scale(int Scale);

/* make sure the data of the board fits the PCB (move it out from negative,
   make the board large enough); returns -1 on error, the number of changes on success */
int pcb_board_normalize(pcb_board_t *pcb);

/* sets or resets changed flag and redraws status */
void pcb_board_set_changed_flag(pcb_board_t *pcb, rnd_bool New);

/* Shorthand for emitting a board changed event (RND_EVENT_BOARD_CHANGED) */
void pcb_board_changed(int reverted);


/* pcb_board_t field accessors - do not use; required for path.[ch] to not depend on board.h */
const char *pcb_board_get_filename(void);
const char *pcb_board_get_name(void);

/* Update the bounding box of the subc being edited as board; if the
   board is not a subc, this call is a NOP; implemented in obj_subc.c */
void pcb_subc_as_board_update(pcb_board_t *pcb);

/*** thickness ***/
typedef enum pcb_board_thickness_flags_e { /* bitfield */
	PCB_BRDTHICK_TOLERANT = 1,               /* do not return -1 if any substrate layer lacks thickness */
	PCB_BRDTHICK_PRINT_ERROR = 2             /* print an error message for missing thickness attributes */
} pcb_board_thickness_flags_t;

/* Return the total thickness of the stack between groups 'from' and 'to';
   whether the own thickness of 'from' and 'to' are included in the
   calculation depends on the corresponding include_ argument. Only groups
   with type matching 'accept' will be considered. Return -1 on error. */
rnd_coord_t pcb_stack_thickness(pcb_board_t *pcb, const char *namespace, pcb_board_thickness_flags_t flags, rnd_layergrp_id_t from, rnd_bool include_from, rnd_layergrp_id_t to, rnd_bool include_to, pcb_layer_type_t accept);

/* Return the board thickness or -1 on error */
rnd_coord_t pcb_board_thickness(pcb_board_t *pcb, const char *namespace, pcb_board_thickness_flags_t flags);


#define PCB_ACT_BOARD ((pcb_board_t *)argv[0].val.argv0.user_call_ctx)

#endif
