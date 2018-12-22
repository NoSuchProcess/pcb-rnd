/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2018 Tibor 'Igor2' Palinkas
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

/* This API builds and maintains a collection of values for each named property.
   A value can be added any time. All values ever seen for a property is stored.
   Duplicate values per property are stored only once, but number of occurence
   per value (per property) is kept track on. Typically at the end of
   a query, but generally ny time, the caller may query for:
    - all known properties (it's a htsp)
    - all known values of a property
    - statistics of values of a property
*/

#include <limits.h>
#include "global_typedefs.h"
#include <genht/htsp.h>
#include <genvector/vtl0.h>

#include "idpath.h"

typedef enum {
	PCB_PROPT_invalid,
	PCB_PROPT_STRING,
	PCB_PROPT_COORD,
	PCB_PROPT_ANGLE,
	PCB_PROPT_INT,
	PCB_PROPT_max
} pcb_prop_type_t;

typedef union {
	const char *string;
	pcb_coord_t coord;
	pcb_angle_t angle;
	int i;
} pcb_propval_t;

typedef pcb_propval_t htprop_key_t;
typedef unsigned long int htprop_value_t;
#define HT(x) htprop_ ## x
#include <genht/ht.h>
#undef HT

typedef struct {
	pcb_prop_type_t type;
	htprop_t values;
	unsigned core:1;  /* 1 if it is a core property */
} pcb_props_t;

typedef struct {
	htsp_t props;

	/* scope */
	pcb_board_t *pcb;

	/* target objects */
	pcb_idpath_list_t objs;
	vtl0_t layers;             /* layer IDs */
	vtl0_t layergrps;          /* layer group IDs */
	unsigned selection:1;      /* all selected objects on the current pcb */
	unsigned board:1;          /* run on the board too */
} pcb_propedit_t;

/* A property list (props) is a string->pcb_props_t. Each entry is a named
   property with a value that's a type and a value hash (vhash). vhash's
   key is each value that the property ever took, and vhash's value is an
   integer value of how many times the given property is taken */
void pcb_props_init(pcb_propedit_t *ctx, pcb_board_t *pcb);
void pcb_props_uninit(pcb_propedit_t *ctx);

/* Free and remove all items from the hash (ctx->props) */
void pcb_props_reset(pcb_propedit_t *ctx);

/* Add a value of a named property; if the value is already known, its counter
   is increased. If propname didn't exist, create it. Returns NULL on error.
   Error conditions:
    - invalid type
    - mismatching type for the property (all values of a given property must be the same)
*/
pcb_props_t *pcb_props_add(pcb_propedit_t *ctx, const char *propname, pcb_prop_type_t type, pcb_propval_t val);

/* Retrieve values for a prop - returns NULL if propname doesn't exist */
pcb_props_t *pcb_props_get(pcb_propedit_t *ctx, const char *propname);


/* Return the type name of a property type or NULL on error. */
const char *pcb_props_type_name(pcb_prop_type_t type);

/* Look up property p and calculate statistics for all values occured so far.
   Any of most_common, min, max and avg can be NULL. Returns non-zero if propname
   doesn't exist or stat values that can not be calculated for the given type
   are not NULL. Invalid type/stat combinations:
     type=string   min, max, avg
*/
int pcb_props_stat(pcb_propedit_t *ctx, pcb_props_t *p, pcb_propval_t *most_common, pcb_propval_t *min, pcb_propval_t *max, pcb_propval_t *avg);

/* Return a key=NULL terminated array of all entries from the hash, alphabetically sorted */
htsp_entry_t *pcb_props_sort(pcb_propedit_t *ctx);
