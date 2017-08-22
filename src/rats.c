/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
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

/* rats nest routines */

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <assert.h>

#include "board.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "find.h"
#include "polygon.h"
#include "rats.h"
#include "search.h"
#include "undo.h"
#include "stub_mincut.h"
#include "route_style.h"
#include "compat_misc.h"
#include "netlist.h"
#include "compat_nls.h"
#include "vtptr.h"
#include "obj_rat_draw.h"
#include "obj_term.h"
#include "obj_subc_parent.h"

#define STEP_POINT 100

#define TRIEDFIRST 0x1
#define BESTFOUND 0x2

/* ---------------------------------------------------------------------------
 * some forward declarations
 */
static pcb_bool pcb_term_find_name_ppt(const char *, const char *, pcb_connection_t *, pcb_bool);
static pcb_bool ParseConnection(const char *, char *, char *);
static pcb_bool DrawShortestRats(pcb_netlist_t *,
														 void (*)(register pcb_connection_t *, register pcb_connection_t *, register pcb_route_style_t *));
static pcb_bool GatherSubnets(pcb_netlist_t *, pcb_bool, pcb_bool);
static pcb_bool CheckShorts(pcb_lib_menu_t *);
static void TransferNet(pcb_netlist_t *, pcb_net_t *, pcb_net_t *);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static pcb_bool badnet = pcb_false;
static pcb_layergrp_id_t Sgrp = -1, Cgrp = -1; /* layer group holding solder/component side */

/* ---------------------------------------------------------------------------
 * parse a connection description from a string
 * puts the element name in the string and the pin number in
 * the number.  If a valid connection is found, it returns the
 * number of characters processed from the string, otherwise
 * it returns 0
 */
static pcb_bool ParseConnection(const char *InString, char *ElementName, char *PinNum)
{
	int i, j;

	/* copy element name portion */
	for (j = 0; InString[j] != '\0' && InString[j] != '-'; j++)
		ElementName[j] = InString[j];
	if (InString[j] == '-') {
		for (i = j; i > 0 && ElementName[i - 1] >= 'a'; i--);
		ElementName[i] = '\0';
		for (i = 0, j++; InString[j] != '\0'; i++, j++)
			PinNum[i] = InString[j];
		PinNum[i] = '\0';
		return (pcb_false);
	}
	else {
		ElementName[j] = '\0';
		pcb_message(PCB_MSG_ERROR, _("Bad net-list format encountered near: \"%s\"\n"), ElementName);
		return (pcb_true);
	}
}

/* ---------------------------------------------------------------------------
 * Find a particular terminal from an element/subc name and pin/pad/terminal number
 */
#warning term TODO: once pins and pads are gone, move this to obj_term.c
static pcb_bool pcb_term_find_name_ppt(const char *ElementName, const char *PinNum, pcb_connection_t * conn, pcb_bool Same)
{
	pcb_element_t *element;
	gdl_iterator_t it;
	pcb_pad_t *pad;
	pcb_pin_t *pin;
	pcb_any_obj_t *obj;

	/* first check for subcircuits; this is the only one thing we'll need to do
	   once elements are removed */
	obj = pcb_term_find_name(PCB, PCB->Data, PCB_LYT_COPPER, ElementName, PinNum, Same, (pcb_subc_t **)&conn->ptr1, &conn->group);
	if (obj != NULL) {
		conn->obj = obj;
		pcb_obj_center(obj, &conn->X, &conn->Y);
		return pcb_true;
	}

	if ((element = pcb_search_elem_by_name(PCB->Data, ElementName)) == NULL)
		return pcb_false;

	padlist_foreach(&element->Pad, &it, pad) {
		if (PCB_NSTRCMP(PinNum, pad->Number) == 0 && (!Same || !PCB_FLAG_TEST(PCB_FLAG_DRC, pad))) {
			conn->ptr1 = element;
			conn->obj = (pcb_any_obj_t *)pad;
			conn->group = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) ? Sgrp : Cgrp;

			if (PCB_FLAG_TEST(PCB_FLAG_EDGE2, pad)) {
				conn->X = pad->Point2.X;
				conn->Y = pad->Point2.Y;
			}
			else {
				conn->X = pad->Point1.X;
				conn->Y = pad->Point1.Y;
			}
			return pcb_true;
		}
	}

	padlist_foreach(&element->Pin, &it, pin) {
		if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, pin) && pin->Number && PCB_NSTRCMP(PinNum, pin->Number) == 0 && (!Same || !PCB_FLAG_TEST(PCB_FLAG_DRC, pin))) {
			conn->ptr1 = element;
			conn->obj = (pcb_any_obj_t *)pin;
			conn->group = Sgrp;			/* any layer will do */
			conn->X = pin->X;
			conn->Y = pin->Y;
			return pcb_true;
		}
	}

	return pcb_false;
}

/*--------------------------------------------------------------------------
 * parse a netlist menu entry and locate the corresponding pad
 * returns pcb_true if found, and fills in Connection information
 */
pcb_bool pcb_rat_seek_pad(pcb_lib_entry_t * entry, pcb_connection_t * conn, pcb_bool Same)
{
	int j;
	char ElementName[256];
	char PinNum[256];

	if (ParseConnection(entry->ListEntry, ElementName, PinNum))
		return (pcb_false);
	for (j = 0; PinNum[j] != '\0'; j++);
	if (j == 0) {
		pcb_message(PCB_MSG_ERROR, _("Error! Netlist file is missing pin!\n" "white space after \"%s-\"\n"), ElementName);
		badnet = pcb_true;
	}
	else {
		if (pcb_term_find_name_ppt(ElementName, PinNum, conn, Same))
			return (pcb_true);
		if (Same)
			return (pcb_false);
		if (PinNum[j - 1] < '0' || PinNum[j - 1] > '9') {
			pcb_message(PCB_MSG_WARNING, "WARNING! Pin number ending with '%c'"
							" encountered in netlist file\n" "Probably a bad netlist file format\n", PinNum[j - 1]);
		}
	}
	pcb_message(PCB_MSG_WARNING, _("Can't find %s pin %s called for in netlist.\n"), ElementName, PinNum);
	return (pcb_false);
}

static const char *get_refdes(void *ptr1)
{
	pcb_any_obj_t *obj = ptr1;
	switch(obj->type) {
		case PCB_OBJ_ELEMENT: return PCB_ELEM_NAME_REFDES((pcb_element_t *)ptr1);
		case PCB_OBJ_SUBC:
			if (((pcb_subc_t *)ptr1)->refdes != NULL)
				return ((pcb_subc_t *)ptr1)->refdes;
			return "<anon>";
		default:
			break;
	}
	return "<invalid>";
}

static const char *get_termid(pcb_any_obj_t *obj)
{
	switch(obj->type) {
		case PCB_OBJ_PIN: return ((pcb_pin_t *)obj)->Number;
		case PCB_OBJ_PAD: return ((pcb_pad_t *)obj)->Number;
		default: return obj->term;
	}
}

static void clear_drc_flag(int clear_ratconn)
{
	pcb_rtree_it_t it;
	pcb_box_t *n;
	int li;
	pcb_layer_t *l;

	for(n = pcb_r_first(PCB->Data->pin_tree, &it); n != NULL; n = pcb_r_next(&it)) {
		if (clear_ratconn)
			((pcb_pin_t *)n)->ratconn = NULL;
		PCB_FLAG_CLEAR(PCB_FLAG_DRC, (pcb_pin_t *)n);
	}
	pcb_r_end(&it);

	for(n = pcb_r_first(PCB->Data->via_tree, &it); n != NULL; n = pcb_r_next(&it))
		PCB_FLAG_CLEAR(PCB_FLAG_DRC, (pcb_pin_t *)n);
	pcb_r_end(&it);

	for(n = pcb_r_first(PCB->Data->pad_tree, &it); n != NULL; n = pcb_r_next(&it)) {
		if (clear_ratconn)
			((pcb_pad_t *)n)->ratconn = NULL;
		PCB_FLAG_CLEAR(PCB_FLAG_DRC, (pcb_pad_t *)n);
	}
	pcb_r_end(&it);

	for(li = 0, l = PCB->Data->Layer; li < PCB->Data->LayerN; li++,l++) {
		for(n = pcb_r_first(l->line_tree, &it); n != NULL; n = pcb_r_next(&it))
			PCB_FLAG_CLEAR(PCB_FLAG_DRC, (pcb_line_t *)n);
		pcb_r_end(&it);

		for(n = pcb_r_first(l->arc_tree, &it); n != NULL; n = pcb_r_next(&it))
			PCB_FLAG_CLEAR(PCB_FLAG_DRC, (pcb_arc_t *)n);
		pcb_r_end(&it);

		for(n = pcb_r_first(l->polygon_tree, &it); n != NULL; n = pcb_r_next(&it))
			PCB_FLAG_CLEAR(PCB_FLAG_DRC, (pcb_polygon_t *)n);
		pcb_r_end(&it);

		for(n = pcb_r_first(l->text_tree, &it); n != NULL; n = pcb_r_next(&it))
			PCB_FLAG_CLEAR(PCB_FLAG_DRC, (pcb_text_t *)n);
		pcb_r_end(&it);
	}
}

/* ---------------------------------------------------------------------------
 * Read the library-netlist build a pcb_true Netlist structure
 */
pcb_netlist_t *pcb_rat_proc_netlist(pcb_lib_t *net_menu)
{
	pcb_connection_t *connection;
	pcb_connection_t LastPoint;
	pcb_net_t *net;
	static pcb_netlist_t *Wantlist = NULL;

	if (!net_menu->MenuN)
		return (NULL);
	pcb_netlist_free(Wantlist);
	free(Wantlist);
	badnet = pcb_false;

	/* find layer groups of the component side and solder side */
	Sgrp = Cgrp = -1;
	pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &Sgrp, 1);
	pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &Cgrp, 1);

	Wantlist = (pcb_netlist_t *) calloc(1, sizeof(pcb_netlist_t));
	if (Wantlist) {
		clear_drc_flag(1);
		PCB_MENU_LOOP(net_menu);
		{
			if (menu->Name[0] == '*' || menu->flag == 0) {
				badnet = pcb_true;
				continue;
			}
			net = pcb_net_new(Wantlist);
			if (menu->Style) {
				int idx = pcb_route_style_lookup(&PCB->RouteStyle, -1, -1, -1, -1, menu->Style);
				if (idx >= 0)
					net->Style = PCB->RouteStyle.array+idx;
			}
			else											/* default to NULL if none found */
				net->Style = NULL;
			PCB_ENTRY_LOOP(menu);
			{
				if (pcb_rat_seek_pad(entry, &LastPoint, pcb_false)) {
					if (PCB_FLAG_TEST(PCB_FLAG_DRC, (pcb_pin_t *) LastPoint.obj))
						pcb_message(PCB_MSG_ERROR, _
										("Error! Element %s pin %s appears multiple times in the netlist file.\n"),
										get_refdes(LastPoint.ptr1), get_termid(LastPoint.obj));
					else {
						connection = pcb_rat_connection_alloc(net);
						*connection = LastPoint;
						/* indicate expect net */
						connection->menu = menu;
						/* mark as visited */
						PCB_FLAG_SET(PCB_FLAG_DRC, (pcb_pin_t *) LastPoint.obj);
						LastPoint.obj->ratconn = (void *)menu;
					}
				}
				else
					badnet = pcb_true;
				/* check for more pins with the same number */
				for (; pcb_rat_seek_pad(entry, &LastPoint, pcb_true);) {
					connection = pcb_rat_connection_alloc(net);
					*connection = LastPoint;
					/* indicate expect net */
					connection->menu = menu;
					/* mark as visited */
					PCB_FLAG_SET(PCB_FLAG_DRC, (pcb_pin_t *) LastPoint.obj);
					LastPoint.obj->ratconn = (void *)menu;
				}
			}
			PCB_END_LOOP;
		}
		PCB_END_LOOP;
	}

	/* clear all visit marks */
	clear_drc_flag(0);

	return (Wantlist);
}

/*
 * copy all connections from one net into another
 * and then remove the first net from its netlist
 */
static void TransferNet(pcb_netlist_t *Netl, pcb_net_t *SourceNet, pcb_net_t *DestNet)
{
	pcb_connection_t *conn;

	/* It would be worth checking if SourceNet is NULL here to avoid a segfault. Seb James. */
	PCB_CONNECTION_LOOP(SourceNet);
	{
		conn = pcb_rat_connection_alloc(DestNet);
		*conn = *connection;
	}
	PCB_END_LOOP;
	DestNet->Style = SourceNet->Style;
	/* free the connection memory */
	pcb_net_free(SourceNet);
	/* remove SourceNet from its netlist */
	*SourceNet = Netl->Net[--(Netl->NetN)];
	/* zero out old garbage */
	memset(&Netl->Net[Netl->NetN], 0, sizeof(pcb_net_t));
}

static void **found_short(pcb_any_obj_t *parent, pcb_any_obj_t *term, vtptr_t *generic, pcb_lib_menu_t *theNet, void **menu)
{
	pcb_bool newone;
	int i;

	if (!term->ratconn) {
		pcb_message(PCB_MSG_WARNING, _("Warning! Net \"%s\" is shorted to %s terminal %s\n"),
						&theNet->Name[2], PCB_UNKNOWN(get_refdes(parent)), PCB_UNKNOWN(get_termid(term)));
		pcb_stub_rat_found_short(term, &theNet->Name[2]);
		return menu;
	}

	newone = pcb_true;
	for(i = 0; i < vtptr_len(generic); i++) {
		if (generic->array[i] == term->ratconn) {
			newone = pcb_false;
			break;
		}
	}

	if (newone) {
		menu = vtptr_alloc_append(generic, 1);
		*menu = term->ratconn;
		pcb_message(PCB_MSG_WARNING, _("Warning! Net \"%s\" is shorted to net \"%s\"\n"),
						&theNet->Name[2], &((pcb_lib_menu_t *) (term->ratconn))->Name[2]);
		pcb_stub_rat_found_short((pcb_any_obj_t *)term, &theNet->Name[2]);
	}
	return menu;
}

static void **find_shorts_in_subc(pcb_subc_t *subc_in, vtptr_t *generic, pcb_lib_menu_t *theNet, void **menu, pcb_bool *warn)
{
	if (PCB_FLAG_TEST(PCB_FLAG_NONETLIST, subc_in))
		return menu;

	PCB_VIA_LOOP(subc_in->data);
	{
		if (via->term == NULL)
			continue;
		if (PCB_FLAG_TEST(PCB_FLAG_DRC, via)) {
			*warn = pcb_true;
			menu = found_short((pcb_any_obj_t *)subc_in, (pcb_any_obj_t *)via, generic, theNet, menu);
		}
	}
	PCB_END_LOOP;

	PCB_LINE_ALL_LOOP(subc_in->data);
	{
		if (line->term == NULL)
			continue;
		if (PCB_FLAG_TEST(PCB_FLAG_DRC, line)) {
			*warn = pcb_true;
			menu = found_short((pcb_any_obj_t *)subc_in, (pcb_any_obj_t *)line, generic, theNet, menu);
		}
	}
	PCB_ENDALL_LOOP;

	PCB_ARC_ALL_LOOP(subc_in->data);
	{
		if (arc->term == NULL)
			continue;
		if (PCB_FLAG_TEST(PCB_FLAG_DRC, arc)) {
			*warn = pcb_true;
			menu = found_short((pcb_any_obj_t *)subc_in, (pcb_any_obj_t *)arc, generic, theNet, menu);
		}
	}
	PCB_ENDALL_LOOP;

	PCB_POLY_ALL_LOOP(subc_in->data);
	{
		if (polygon->term == NULL)
			continue;
		if (PCB_FLAG_TEST(PCB_FLAG_DRC, polygon)) {
			*warn = pcb_true;
			menu = found_short((pcb_any_obj_t *)subc_in, (pcb_any_obj_t *)polygon, generic, theNet, menu);
		}
	}
	PCB_ENDALL_LOOP;

	PCB_TEXT_ALL_LOOP(subc_in->data);
	{
		if (text->term == NULL)
			continue;
		if (PCB_FLAG_TEST(PCB_FLAG_DRC, text)) {
			*warn = pcb_true;
			menu = found_short((pcb_any_obj_t *)subc_in, (pcb_any_obj_t *)text, generic, theNet, menu);
		}
	}
	PCB_ENDALL_LOOP;

	PCB_SUBC_LOOP(subc_in->data);
	{
		menu = find_shorts_in_subc(subc, generic, theNet, menu, warn);
	}
	PCB_END_LOOP;
	return menu;
}


static pcb_bool CheckShorts(pcb_lib_menu_t *theNet)
{
	pcb_bool warn = pcb_false;
	vtptr_t generic;
	/* the first connection was starting point so
	 * the menu is always non-null
	 */
	void **menu;

	vtptr_init(&generic);
	menu = vtptr_alloc_append(&generic, 1);
	*menu = theNet;
	PCB_PIN_ALL_LOOP(PCB->Data);
	{
		pcb_element_t *e = pin->Element;
/* TODO: should be: !PCB_FLAG_TEST(PCB_FLAG_NONETLIST, (pcb_element_t *)pin->Element)*/
		if ((PCB_FLAG_TEST(PCB_FLAG_DRC, pin)) && (!(e->Flags.f & PCB_FLAG_NONETLIST)) && (!(e->Name->Flags.f & PCB_FLAG_NONETLIST))) {
			warn = pcb_true;
			menu = found_short((pcb_any_obj_t *)element, (pcb_any_obj_t *)pin, &generic, theNet, menu);
		}
	}
	PCB_ENDALL_LOOP;
	PCB_PAD_ALL_LOOP(PCB->Data);
	{
		pcb_element_t *e = pad->Element;
/* TODO: should be: !PCB_FLAG_TEST(PCB_FLAG_NONETLIST, (pcb_element_t *)pad->Element)*/
		if ((PCB_FLAG_TEST(PCB_FLAG_DRC, pad)) && (!(e->Flags.f & PCB_FLAG_NONETLIST)) && (!(e->Name->Flags.f & PCB_FLAG_NONETLIST))) {
			warn = pcb_true;
			menu = found_short((pcb_any_obj_t *)element, (pcb_any_obj_t *)pad, &generic, theNet, menu);
		}
	}
	PCB_ENDALL_LOOP;
	PCB_SUBC_LOOP(PCB->Data);
	{
		menu = find_shorts_in_subc(subc, &generic, theNet, menu, &warn);
	}
	PCB_END_LOOP;
	vtptr_uninit(&generic);
	return (warn);
}

static pcb_bool gather_subnet_objs(pcb_netlist_t *Netl, pcb_cardinal_t m, pcb_net_t *a)
{
	pcb_net_t *b;
	pcb_cardinal_t n;
	pcb_connection_t *conn;

		/* now anybody connected to the first point has PCB_FLAG_DRC set */
		/* so move those to this subnet */
		PCB_FLAG_CLEAR(PCB_FLAG_DRC, (pcb_pin_t *) a->Connection[0].obj);
		for (n = m + 1; n < Netl->NetN; n++) {
			b = &Netl->Net[n];
			/* There can be only one connection in net b */
			if (PCB_FLAG_TEST(PCB_FLAG_DRC, (pcb_pin_t *) b->Connection[0].obj)) {
				PCB_FLAG_CLEAR(PCB_FLAG_DRC, (pcb_pin_t *) b->Connection[0].obj);
				TransferNet(Netl, b, a);
				/* back up since new subnet is now at old index */
				n--;
			}
		}
		/* now add other possible attachment points to the subnet */
		/* e.g. line end-points and vias */
		/* don't add non-manhattan lines, the auto-router can't route to them */
		PCB_LINE_ALL_LOOP(PCB->Data);
		{
			if (PCB_FLAG_TEST(PCB_FLAG_DRC, line)
					&& ((line->Point1.X == line->Point2.X)
							|| (line->Point1.Y == line->Point2.Y))) {
				conn = pcb_rat_connection_alloc(a);
				conn->X = line->Point1.X;
				conn->Y = line->Point1.Y;
				conn->ptr1 = layer;
				conn->obj = (pcb_any_obj_t *)line;
				conn->group = pcb_layer_get_group_(layer);
				conn->menu = NULL;			/* agnostic view of where it belongs */
				conn = pcb_rat_connection_alloc(a);
				conn->X = line->Point2.X;
				conn->Y = line->Point2.Y;
				conn->ptr1 = layer;
				conn->obj = (pcb_any_obj_t *)line;
				conn->group = pcb_layer_get_group_(layer);
				conn->menu = NULL;
			}
		}
		PCB_ENDALL_LOOP;
		/* add polygons so the auto-router can see them as targets */
		PCB_POLY_ALL_LOOP(PCB->Data);
		{
			if (PCB_FLAG_TEST(PCB_FLAG_DRC, polygon)) {
				conn = pcb_rat_connection_alloc(a);
				/* make point on a vertex */
				conn->X = polygon->Clipped->contours->head.point[0];
				conn->Y = polygon->Clipped->contours->head.point[1];
				conn->ptr1 = layer;
				conn->obj = (pcb_any_obj_t *)polygon;
				conn->group = pcb_layer_get_group_(layer);
				conn->menu = NULL;			/* agnostic view of where it belongs */
			}
		}
		PCB_ENDALL_LOOP;
		PCB_VIA_LOOP(PCB->Data);
		{
			if (PCB_FLAG_TEST(PCB_FLAG_DRC, via)) {
				conn = pcb_rat_connection_alloc(a);
				conn->X = via->X;
				conn->Y = via->Y;
				conn->ptr1 = via;
				conn->obj = (pcb_any_obj_t *)via;
				conn->group = Sgrp;
			}
		}
		PCB_END_LOOP;
}

/* ---------------------------------------------------------------------------
 * Determine existing interconnections of the net and gather into sub-nets
 *
 * initially the netlist has each connection in its own individual net
 * afterwards there can be many fewer nets with multiple connections each
 */
static pcb_bool GatherSubnets(pcb_netlist_t *Netl, pcb_bool NoWarn, pcb_bool AndRats)
{
	pcb_net_t *a;
	pcb_bool Warned = pcb_false;
	pcb_cardinal_t m;

	for (m = 0; Netl->NetN > 0 && m < Netl->NetN; m++) {
		a = &Netl->Net[m];
		pcb_reset_conns(pcb_false);
		pcb_rat_find_hook(a->Connection[0].obj, pcb_false, AndRats);
		gather_subnet_objs(Netl, m, a);
		if (!NoWarn)
			Warned |= CheckShorts(a->Connection[0].menu);
	}
	pcb_reset_conns(pcb_false);
	return (Warned);
}

/* ---------------------------------------------------------------------------
 * Draw a rat net (tree) having the shortest lines
 * this also frees the subnet memory as they are consumed
 *
 * Note that the Netl we are passed is NOT the main netlist - it's the
 * connectivity for ONE net.  It represents the CURRENT connectivity
 * state for the net, with each Netl->Net[N] representing one
 * copper-connected subset of the net.
 */

static pcb_bool
DrawShortestRats(pcb_netlist_t *Netl,
								 void (*funcp) (register pcb_connection_t *, register pcb_connection_t *, register pcb_route_style_t *))
{
	pcb_rat_t *line;
	register float distance, temp;
	register pcb_connection_t *conn1, *conn2, *firstpoint, *secondpoint;
	pcb_polygon_t *polygon;
	pcb_bool changed = pcb_false;
	pcb_bool havepoints;
	pcb_cardinal_t n, m, j;
	pcb_net_t *next, *subnet, *theSubnet = NULL;

	/* This is just a sanity check, to make sure we're passed
	 * *something*.
	 */
	if (!Netl || Netl->NetN < 1)
		return pcb_false;

	/*
	 * Everything inside the NetList Netl should be connected together.
	 * Each Net in Netl is a group of Connections which are already
	 * connected together somehow, either by real wires or by rats we've
	 * already drawn.  Each Connection is a vertex within that blob of
	 * connected items.  This loop finds the closest vertex pairs between
	 * each blob and draws rats that merge the blobs until there's just
	 * one big blob.
	 *
	 * Just to clarify, with some examples:
	 *
	 * Each Netl is one full net from a netlist, like from gnetlist.
	 * Each Netl->Net[N] is a subset of that net that's already
	 * physically connected on the pcb.
	 *
	 * So a new design with no traces yet, would have a huge list of Net[N],
	 * each with one pin in it.
	 *
	 * A fully routed design would have one Net[N] with all the pins
	 * (for that net) in it.
	 */

	/*
	 * We keep doing this do/while loop until everything's connected.
	 * I.e. once per rat we add.
	 */
	distance = 0.0;
	havepoints = pcb_true;						/* so we run the loop at least once */
	while (Netl->NetN > 1 && havepoints) {
		/* This is the top of the "find one rat" logic.  */
		havepoints = pcb_false;
		firstpoint = secondpoint = NULL;

		/* Test Net[0] vs Net[N] for N=1..max.  Find the shortest
		   distance between any two points in different blobs.  */
		subnet = &Netl->Net[0];
		for (j = 1; j < Netl->NetN; j++) {
			/*
			 * Scan between Net[0] blob (subnet) and Net[N] blob (next).
			 * Note the shortest distance we find.
			 */
			next = &Netl->Net[j];
			for (n = subnet->ConnectionN - 1; n != -1; n--) {
				conn1 = &subnet->Connection[n];
				for (m = next->ConnectionN - 1; m != -1; m--) {
					conn2 = &next->Connection[m];
					/*
					 * At this point, conn1 and conn2 are two pins in
					 * different blobs of the same net.  See how far
					 * apart they are, and if they're "closer" than what
					 * we already have.
					 */

					/*
					 * Prefer to connect Connections over polygons to the
					 * polygons (ie assume the user wants a via to a plane,
					 * not a daisy chain).  Further prefer to pick an existing
					 * via in the Net to make that connection.
					 */
					if (conn1->obj->type == PCB_OBJ_POLYGON &&
							(polygon = (pcb_polygon_t *) conn1->obj) &&
							!(distance == 0 &&
								firstpoint && firstpoint->obj->type == PCB_OBJ_VIA) && pcb_poly_is_point_in_p_ignore_holes(conn2->X, conn2->Y, polygon)) {
						distance = 0;
						firstpoint = conn2;
						secondpoint = conn1;
						theSubnet = next;
						havepoints = pcb_true;
					}
					else if (conn2->obj->type == PCB_OBJ_POLYGON &&
									 (polygon = (pcb_polygon_t *) conn2->obj) &&
									 !(distance == 0 &&
										 firstpoint && firstpoint->obj->type == PCB_OBJ_VIA) && pcb_poly_is_point_in_p_ignore_holes(conn1->X, conn1->Y, polygon)) {
						distance = 0;
						firstpoint = conn1;
						secondpoint = conn2;
						theSubnet = next;
						havepoints = pcb_true;
					}
					else if ((temp = PCB_SQUARE(conn1->X - conn2->X) + PCB_SQUARE(conn1->Y - conn2->Y)) < distance || !firstpoint) {
						distance = temp;
						firstpoint = conn1;
						secondpoint = conn2;
						theSubnet = next;
						havepoints = pcb_true;
					}
				}
			}
		}

		/*
		 * If HAVEPOINTS is pcb_true, we've found a pair of points in two
		 * separate blobs of the net, and need to connect them together.
		 */
		if (havepoints) {
			if (funcp) {
				(*funcp) (firstpoint, secondpoint, subnet->Style);
			}
			else {
				/* found the shortest distance subnet, draw the rat */
				if ((line = pcb_rat_new(PCB->Data,
																 firstpoint->X, firstpoint->Y,
																 secondpoint->X, secondpoint->Y,
																 firstpoint->group, secondpoint->group, conf_core.appearance.rat_thickness, pcb_no_flags())) != NULL) {
					if (distance == 0)
						PCB_FLAG_SET(PCB_FLAG_VIA, line);
					pcb_undo_add_obj_to_create(PCB_TYPE_RATLINE, line, line, line);
					pcb_rat_invalidate_draw(line);
					changed = pcb_true;
				}
			}

			/* copy theSubnet into the current subnet */
			TransferNet(Netl, theSubnet, subnet);
		}
	}

	/* presently nothing to do with the new subnet */
	/* so we throw it away and free the space */
	pcb_net_free(&Netl->Net[--(Netl->NetN)]);
	/* Sadly adding a rat line messes up the sorted arrays in connection finder */
	/* hace: perhaps not necessarily now that they aren't stored in normal layers */
	if (changed) {
		pcb_conn_lookup_uninit();
		pcb_conn_lookup_init();
	}
	return (changed);
}


/* ---------------------------------------------------------------------------
 *  AddAllRats puts the rats nest into the layout from the loaded netlist
 *  if SelectedOnly is pcb_true, it will only draw rats to selected pins and pads
 */
pcb_bool
pcb_rat_add_all(pcb_bool SelectedOnly,
					 void (*funcp) (register pcb_connection_t *, register pcb_connection_t *, register pcb_route_style_t *))
{
	pcb_netlist_t *Nets, *Wantlist;
	pcb_net_t *lonesome;
	pcb_connection_t *onepin;
	pcb_bool changed, Warned = pcb_false;

	/* the netlist library has the text form
	 * ProcNetlist fills in the Netlist
	 * structure the way the final routing
	 * is supposed to look
	 */
	Wantlist = pcb_rat_proc_netlist(&(PCB->NetlistLib[PCB_NETLIST_EDITED]));
	if (!Wantlist) {
		pcb_message(PCB_MSG_WARNING, _("Can't add rat lines because no netlist is loaded.\n"));
		return (pcb_false);
	}
	changed = pcb_false;
	/* initialize finding engine */
	pcb_conn_lookup_init();
	pcb_save_find_flag(PCB_FLAG_DRC);
	Nets = (pcb_netlist_t *) calloc(1, sizeof(pcb_netlist_t));
	/* now we build another netlist (Nets) for each
	 * net in Wantlist that shows how it actually looks now,
	 * then fill in any missing connections with rat lines.
	 *
	 * we first assume each connection is separate
	 * (no routing), then gather them into groups
	 * if the net is all routed, the new netlist (Nets)
	 * will have only one net entry.
	 * Note that DrawShortestRats consumes all nets
	 * from Nets, so *Nets is empty after the
	 * DrawShortestRats call
	 */
	PCB_NET_LOOP(Wantlist);
	{
		PCB_CONNECTION_LOOP(net);
		{
			if (!SelectedOnly || PCB_FLAG_TEST(PCB_FLAG_SELECTED, (pcb_pin_t *) connection->obj)) {
				lonesome = pcb_net_new(Nets);
				onepin = pcb_rat_connection_alloc(lonesome);
				*onepin = *connection;
				lonesome->Style = net->Style;
			}
		}
		PCB_END_LOOP;
		Warned |= GatherSubnets(Nets, SelectedOnly, pcb_true);
		if (Nets->NetN > 0)
			changed |= DrawShortestRats(Nets, funcp);
	}
	PCB_END_LOOP;
	pcb_netlist_free(Nets);
	free(Nets);
	pcb_conn_lookup_uninit();
	pcb_restore_find_flag();
	if (funcp)
		return (pcb_true);

	if (Warned || changed) {
		pcb_stub_rat_proc_shorts();
		pcb_draw();
	}

	if (Warned)
		conf_core.temp.rat_warn = pcb_true;

	if (changed) {
		pcb_undo_inc_serial();
		if (ratlist_length(&PCB->Data->Rat) > 0) {
			pcb_message(PCB_MSG_INFO, "%d rat line%s remaining\n", ratlist_length(&PCB->Data->Rat), ratlist_length(&PCB->Data->Rat) > 1 ? "s" : "");
		}
		return (pcb_true);
	}
	if (!SelectedOnly && !Warned) {
		if (!ratlist_length(&PCB->Data->Rat) && !badnet)
			pcb_message(PCB_MSG_INFO, _("Congratulations!!\n" "The layout is complete and has no shorted nets.\n"));
		else
			pcb_message(PCB_MSG_WARNING, _("Nothing more to add, but there are\n"
								"either rat-lines in the layout, disabled nets\n" "in the net-list, or missing components\n"));
	}
	return (pcb_false);
}

/* XXX: This is copied in large part from AddAllRats above; for
 * maintainability, AddAllRats probably wants to be tweaked to use this
 * version of the code so that we don't have duplication. */
pcb_netlist_list_t pcb_rat_collect_subnets(pcb_bool SelectedOnly)
{
	pcb_netlist_list_t result = { 0, 0, NULL };
	pcb_netlist_t *Nets, *Wantlist;
	pcb_net_t *lonesome;
	pcb_connection_t *onepin;

	/* the netlist library has the text form
	 * ProcNetlist fills in the Netlist
	 * structure the way the final routing
	 * is supposed to look
	 */
	Wantlist = pcb_rat_proc_netlist(&(PCB->NetlistLib[PCB_NETLIST_EDITED]));
	if (!Wantlist) {
		pcb_message(PCB_MSG_WARNING, _("Can't add rat lines because no netlist is loaded.\n"));
		return result;
	}
	/* initialize finding engine */
	pcb_conn_lookup_init();
	pcb_save_find_flag(PCB_FLAG_DRC);
	/* now we build another netlist (Nets) for each
	 * net in Wantlist that shows how it actually looks now,
	 * then fill in any missing connections with rat lines.
	 *
	 * we first assume each connection is separate
	 * (no routing), then gather them into groups
	 * if the net is all routed, the new netlist (Nets)
	 * will have only one net entry.
	 * Note that DrawShortestRats consumes all nets
	 * from Nets, so *Nets is empty after the
	 * DrawShortestRats call
	 */
	PCB_NET_LOOP(Wantlist);
	{
		Nets = pcb_netlist_new(&result);
		PCB_CONNECTION_LOOP(net);
		{
			if (!SelectedOnly || PCB_FLAG_TEST(PCB_FLAG_SELECTED, (pcb_pin_t *) connection->obj)) {
				lonesome = pcb_net_new(Nets);
				onepin = pcb_rat_connection_alloc(lonesome);
				*onepin = *connection;
				lonesome->Style = net->Style;
			}
		}
		PCB_END_LOOP;
		/* Note that AndRats is *pcb_false* here! */
		GatherSubnets(Nets, SelectedOnly, pcb_false);
	}
	PCB_END_LOOP;
	pcb_conn_lookup_uninit();
	pcb_restore_find_flag();
	return result;
}

/*
 * Check to see if a particular name is the name of an already existing rats
 * line
 */
static int rat_used(char *name)
{
	if (name == NULL)
		return -1;

	PCB_MENU_LOOP(&(PCB->NetlistLib[PCB_NETLIST_EDITED]));
	{
		if (menu->Name && (strcmp(menu->Name, name) == 0))
			return 1;
	}
	PCB_END_LOOP;

	return 0;
}

pcb_rat_t *pcb_rat_add_net(void)
{
	static int ratDrawn = 0;
	char name1[256], *name2;
	pcb_cardinal_t group1, group2;
	char ratname[20];
	const char *rd;
	int found;
	void *ptr1, *ptr2, *ptr3;
	pcb_lib_menu_t *menu;
	pcb_lib_entry_t *entry;

	if (pcb_crosshair.AttachedLine.Point1.X == pcb_crosshair.AttachedLine.Point2.X
			&& pcb_crosshair.AttachedLine.Point1.Y == pcb_crosshair.AttachedLine.Point2.Y)
		return (NULL);

	found = pcb_search_obj_by_location(PCB_TYPE_PAD | PCB_TYPE_PIN, &ptr1, &ptr2, &ptr3,
																 pcb_crosshair.AttachedLine.Point1.X, pcb_crosshair.AttachedLine.Point1.Y, 5);
	if (found == PCB_TYPE_NONE) {
		pcb_message(PCB_MSG_ERROR, _("No pad/pin under rat line\n"));
		return (NULL);
	}
	rd = get_refdes(ptr1);
	if ((rd == NULL) || (*rd == 0) || (*rd == '<')) {
		pcb_message(PCB_MSG_ERROR, _("You must name the starting element first\n"));
		return (NULL);
	}

	Sgrp = Cgrp = -1;
	pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &Sgrp, 1);
	pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &Cgrp, 1);

	/* will work for pins to since the FLAG is common */
	group1 = (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, (pcb_pad_t *) ptr2) ? Sgrp : Cgrp);
	strcpy(name1, pcb_connection_name(ptr2));
	found = pcb_search_obj_by_location(PCB_TYPE_PAD | PCB_TYPE_PIN, &ptr1, &ptr2, &ptr3,
																 pcb_crosshair.AttachedLine.Point2.X, pcb_crosshair.AttachedLine.Point2.Y, 5);
	if (found == PCB_TYPE_NONE) {
		pcb_message(PCB_MSG_ERROR, _("No pad/pin under rat line\n"));
		return (NULL);
	}
	rd = get_refdes(ptr1);
	if ((rd == NULL) || (*rd == 0) || (*rd == '<')) {
		pcb_message(PCB_MSG_ERROR, _("You must name the ending element first\n"));
		return (NULL);
	}
	group2 = (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, (pcb_pad_t *) ptr2) ? Sgrp : Cgrp);
	name2 = pcb_connection_name(ptr2);

	menu = pcb_netnode_to_netname(name1);
	if (menu) {
		if (pcb_netnode_to_netname(name2)) {
			pcb_message(PCB_MSG_ERROR, _("Both connections already in netlist - cannot merge nets\n"));
			return (NULL);
		}
		entry = pcb_lib_entry_new(menu);
		entry->ListEntry = pcb_strdup(name2);
		entry->ListEntry_dontfree = 0;
		pcb_netnode_to_netname(name2);
		goto ratIt;
	}
	/* ok, the first name did not belong to a net */
	menu = pcb_netnode_to_netname(name2);
	if (menu) {
		entry = pcb_lib_entry_new(menu);
		entry->ListEntry = pcb_strdup(name1);
		entry->ListEntry_dontfree = 0;
		pcb_netnode_to_netname(name1);
		goto ratIt;
	}

	/*
	 * neither belong to a net, so create a new one.
	 *
	 * before creating a new rats here, we need to search
	 * for a unique name.
	 */
	sprintf(ratname, "  ratDrawn%i", ++ratDrawn);
	while (rat_used(ratname)) {
		sprintf(ratname, "  ratDrawn%i", ++ratDrawn);
	}

	menu = pcb_lib_menu_new(&(PCB->NetlistLib[PCB_NETLIST_EDITED]), NULL);
	menu->Name = pcb_strdup(ratname);
	entry = pcb_lib_entry_new(menu);
	entry->ListEntry = pcb_strdup(name1);
	entry->ListEntry_dontfree = 0;

	entry = pcb_lib_entry_new(menu);
	entry->ListEntry = pcb_strdup(name2);
	entry->ListEntry_dontfree = 0;
	menu->flag = 1;

ratIt:
	pcb_netlist_changed(0);
	return (pcb_rat_new(PCB->Data, pcb_crosshair.AttachedLine.Point1.X,
											 pcb_crosshair.AttachedLine.Point1.Y,
											 pcb_crosshair.AttachedLine.Point2.X,
											 pcb_crosshair.AttachedLine.Point2.Y, group1, group2, conf_core.appearance.rat_thickness, pcb_no_flags()));
}


char *pcb_connection_name(pcb_any_obj_t *obj)
{
	static char name[256];
	const char *num, *parent = NULL;
	pcb_subc_t *subc;

	switch (obj->type) {
	case PCB_OBJ_PIN:
		num = ((pcb_pin_t *)obj)->Number;
		parent = PCB_ELEM_NAME_REFDES((pcb_element_t*)((pcb_pin_t *)obj)->Element);
		break;
	case PCB_OBJ_PAD:
		num = ((pcb_pad_t *)obj)->Number;
		parent = PCB_ELEM_NAME_REFDES((pcb_element_t*)((pcb_pad_t *)obj)->Element);
		break;
	default:
		if (obj->term == NULL)
			return NULL;
		num = obj->term;
		subc = pcb_obj_parent_subc(obj);
		if (subc != NULL)
			parent = subc->refdes;
	}
	strcpy(name, PCB_UNKNOWN(parent));
	strcat(name, "-");
	strcat(name, PCB_UNKNOWN(num));
	return (name);
}

/* ---------------------------------------------------------------------------
 * get next slot for a connection, allocates memory if necessary
 */
pcb_connection_t *pcb_rat_connection_alloc(pcb_net_t *Net)
{
	pcb_connection_t *con = Net->Connection;

	/* realloc new memory if necessary and clear it */
	if (Net->ConnectionN >= Net->ConnectionMax) {
		Net->ConnectionMax += STEP_POINT;
		con = (pcb_connection_t *) realloc(con, Net->ConnectionMax * sizeof(pcb_connection_t));
		Net->Connection = con;
		memset(con + Net->ConnectionN, 0, STEP_POINT * sizeof(pcb_connection_t));
	}
	return (con + Net->ConnectionN++);
}
