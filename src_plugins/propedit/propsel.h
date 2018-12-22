/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

/* This API builds a prop list by looking at all selected objects on a design. */
void pcb_propsel_map_core(pcb_propedit_t *ctx);

typedef struct set_ctx_s {
	const char *s; /* only for string */
	pcb_coord_t c; /* also int */
	double d;
	pcb_bool c_absolute, d_absolute, c_valid, d_valid;

	/* private */
	unsigned is_trace:1;
	unsigned is_attr:1;
	pcb_board_t *pcb;
	const char *name;
	int set_cnt;
} pcb_propset_ctx_t;


int pcb_propsel_set_str(pcb_propedit_t *ctx, const char *prop, const char *value);
int pcb_propsel_set(pcb_propedit_t *ctx, const char *prop, pcb_propset_ctx_t *sctx);
int pcb_propsel_del(pcb_propedit_t *ctx, const char *attr_name);

/* Allocate new string and print the value using current unit */
char *pcb_propsel_printval(pcb_prop_type_t type, const pcb_propval_t *val);

