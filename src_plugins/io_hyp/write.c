/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include "config.h"

#include "write.h"

#include "board.h"
#include "data.h"
#include "compat_misc.h"
#include "padstack_hash.h"
#include "netmap.h"

typedef struct hyp_wr_s {
	pcb_board_t *pcb;
	FILE *f;
	const char *fn;

	const char *ln_top, *ln_bottom; /* "layer name" for top and bottom groups */
	pcb_pshash_hash_t psh;
	struct {
		unsigned elliptic:1;
	} warn;
} hyp_wr_t;


static int write_hdr(hyp_wr_t *wr)
{
	char dt[128];

	pcb_print_utc(dt, sizeof(dt), 0);

	fprintf(wr->f, "* %s exported by pcb-rnd " PCB_VERSION " (" PCB_REVISION ") on %s\n", wr->pcb->Filename, dt);
	fprintf(wr->f, "* Board: %s\n", wr->pcb->Name);
	fprintf(wr->f, "{VERSION=2.0}\n");
	fprintf(wr->f, "{DATA_MODE=DETAILED}\n");
	fprintf(wr->f, "{UNITS=METRIC LENGTH}\n");
	return 0;
}

static int write_foot(hyp_wr_t *wr)
{
	fprintf(wr->f, "{END}\n");
	return 0;
}

static const char *get_layer_name(hyp_wr_t *wr, pcb_parenttype_t pt, pcb_layer_t *l)
{
	pcb_layergrp_id_t gid;
	pcb_layergrp_t *g;

	if (pt != PCB_PARENT_LAYER)
		return NULL;

	gid = pcb_layer_get_group_(l);
	if (gid < 0)
		return NULL;

	g = pcb_get_layergrp(wr->pcb, gid);
	return g->name;
}

static void write_pr_line(hyp_wr_t *wr, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	pcb_fprintf(wr->f, "  (PERIMETER_SEGMENT X1=%me Y1=%me X2=%me Y2=%me)\n", x1, y1, x2, y2);
}

static void write_pv(hyp_wr_t *wr, pcb_pin_t *pin)
{
	if (pin->type == PCB_OBJ_PIN) {
		pcb_fprintf(wr->f, "  (PIN X=%me Y=%me R=\"%s.%s\" P=%[4])\n",
			pin->X, pin->Y,
			PCB_ELEM_NAME_REFDES((pcb_element_t *)pin->Element), pin->Number,
			pcb_pshash_pin(&wr->psh, pin, NULL));
	}
	else {
		pcb_fprintf(wr->f, "  (VIA X=%me Y=%me P=%[4])\n",
			pin->X, pin->Y,
			pcb_pshash_pin(&wr->psh, pin, NULL));
	}
}

static void write_pad(hyp_wr_t *wr, pcb_pad_t *pad)
{
	pcb_fprintf(wr->f, "  (PIN X=%me Y=%me R=\"%s.%s\" P=%[4])\n",
		(pad->Point1.X + pad->Point2.X)/2, (pad->Point1.Y + pad->Point2.Y)/2,
		PCB_ELEM_NAME_REFDES((pcb_element_t *)pad->Element), pad->Number,
		pcb_pshash_pad(&wr->psh, pad, NULL));
}


static void write_line(hyp_wr_t *wr, pcb_line_t *line)
{
	pcb_fprintf(wr->f, "  (SEG X1=%me Y1=%me X2=%me Y2=%me W=%me L=%[4])\n",
		line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y,
		line->Thickness, get_layer_name(wr, line->parent_type, line->parent.layer));
}

static void write_arc_(hyp_wr_t *wr, const char *cmd, pcb_arc_t *arc, const char *layer)
{
	pcb_coord_t x1, y1, x2, y2;

	if (arc->Width != arc->Height) {
		if (!wr->warn.elliptic) {
			pcb_message(PCB_MSG_WARNING, "Elliptic arcs are not supported - omitting all elliptic arcs\n");
			wr->warn.elliptic = 1;
		}
		return;
	}

	if (arc->Delta >= 0) {
		pcb_arc_get_end(arc, 0, &x1, &y1);
		pcb_arc_get_end(arc, 1, &x2, &y2);
	}
	else {
		pcb_arc_get_end(arc, 1, &x1, &y1);
		pcb_arc_get_end(arc, 0, &x2, &y2);
	}

	pcb_fprintf(wr->f, "(%s X1=%me Y1=%me X2=%me Y2=%me XC=%me YC=%me R=%me", cmd, x1, y1, x2, y2, arc->X, arc->Y, arc->Width);
	if (layer != NULL)
		pcb_fprintf(wr->f, " L=%[4]", layer);
	fprintf(wr->f, ")\n");
}

static void write_pr_arc(hyp_wr_t *wr, pcb_arc_t *arc)
{
	fprintf(wr->f, "  ");
	write_arc_(wr, "PERIMETER_ARC", arc, NULL);
}

static void write_arc(hyp_wr_t *wr, pcb_arc_t *arc)
{
	fprintf(wr->f, "  ");
	write_arc_(wr, "CURVE", arc, get_layer_name(wr, arc->parent_type, arc->parent.layer));
}

static int write_board(hyp_wr_t *wr)
{
	pcb_layer_id_t lid;

	fprintf(wr->f, "{BOARD\n");

	if (pcb_layer_list(PCB_LYT_OUTLINE, &lid, 1) != 1) {
		/* implicit outline */
		fprintf(wr->f, "* implicit outline derived from board width and height\n");
		write_pr_line(wr, 0, 0, PCB->MaxWidth, 0);
		write_pr_line(wr, 0, 0, 0, PCB->MaxHeight);
		write_pr_line(wr, PCB->MaxWidth, 0, PCB->MaxWidth, PCB->MaxHeight);
		write_pr_line(wr, 0, PCB->MaxHeight, PCB->MaxWidth, PCB->MaxHeight);
	}
	else {
		/* explicit outline */
		pcb_layer_t *l = pcb_get_layer(lid);
		gdl_iterator_t it;
		pcb_line_t *line;
		pcb_arc_t *arc;

		linelist_foreach(&l->Line, &it, line) {
			write_pr_line(wr, line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y);
		}
		arclist_foreach(&l->Arc, &it, arc) {
			write_pr_arc(wr, arc);
		}
	}
	fprintf(wr->f, "}\n");
	return 0;
}

static int write_lstack(hyp_wr_t *wr)
{
	int n;

	fprintf(wr->f, "{STACKUP\n");
	for(n = 0; n < wr->pcb->LayerGroups.len; n++) {
		pcb_layergrp_t *grp = &wr->pcb->LayerGroups.grp[n];
		const char *name = grp->name;

		if (grp->type & PCB_LYT_COPPER) {
			pcb_fprintf(wr->f, "  (SIGNAL T=0.003500 L=%[4])\n", name);
			if (grp->type & PCB_LYT_TOP)
				wr->ln_top = name;
			else if (grp->type & PCB_LYT_BOTTOM)
				wr->ln_bottom = name;
		}
		else if (grp->type & PCB_LYT_SUBSTRATE) {
			char tmp[128];
			if (name == NULL) {
				sprintf(tmp, "dielectric layer %d", n);
				name = tmp;
			}
			pcb_fprintf(wr->f, "  (DIELECTRIC T=0.160000 L=%[4])\n", name);
		}
	}

	fprintf(wr->f, "}\n");
	return 0;
}

static int write_devices(hyp_wr_t *wr)
{
	gdl_iterator_t it;
	pcb_element_t *elem;
	int cnt;

	fprintf(wr->f, "{DEVICES\n");

	cnt = 0;
	elementlist_foreach(&wr->pcb->Data->Element, &it, elem) {
		const char *layer;
		if (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, elem))
			layer = wr->ln_bottom;
		else
			layer = wr->ln_top;
		pcb_fprintf(wr->f, "  (? REF=%[4] NAME=%[4] L=%[4])\n", PCB_ELEM_NAME_REFDES(elem), PCB_ELEM_NAME_DESCRIPTION(elem), layer);
		cnt++;
	}

	if (cnt == 0)
		pcb_message(PCB_MSG_WARNING, "There is no element on the board - this limites the use of the resulting .hyp file, as it won't be able to connect to a simulation\n");

	fprintf(wr->f, "}\n");
	return 0;
}

static void write_padstack_pv(hyp_wr_t *wr, const pcb_pin_t *pin)
{
	int new_item;
	const char *name = pcb_pshash_pin(&wr->psh, pin, &new_item);
	if (!new_item)
		return;

	pcb_fprintf(wr->f, "{PADSTACK=%s, %me\n", name, pin->DrillingHole);
#warning TODO: pin shapes and thermal
	pcb_fprintf(wr->f, "  (MDEF, 0, %me, %me, 0, M)\n", pin->Thickness, pin->Thickness);
	fprintf(wr->f, "}\n");
}

static void write_padstack_pad(hyp_wr_t *wr, const pcb_pad_t *pad)
{
	int new_item;
	const char *name = pcb_pshash_pad(&wr->psh, pad, &new_item), *side;
	if (!new_item)
		return;

	side = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) ? wr->ln_bottom : wr->ln_top;

	fprintf(wr->f, "{PADSTACK=%s,\n", name);
	pcb_fprintf(wr->f, "  (%[4], 1, %me, %me, 0, M)\n", side, PCB_ABS(pad->Point1.X - pad->Point2.X) + pad->Thickness, PCB_ABS(pad->Point1.Y - pad->Point2.Y) + pad->Thickness);
	fprintf(wr->f, "}\n");
}

static int write_padstack(hyp_wr_t *wr)
{
	gdl_iterator_t it, it2;
	pcb_element_t *elem;
	pcb_pin_t *pin;
	pcb_pad_t *pad;

	elementlist_foreach(&wr->pcb->Data->Element, &it, elem) {
		pinlist_foreach(&elem->Pin, &it2, pin) {
			write_padstack_pv(wr, pin);
		}
		padlist_foreach(&elem->Pad, &it2, pad) {
			write_padstack_pad(wr, pad);
		}
	}
	pinlist_foreach(&wr->pcb->Data->Via, &it, pin) {
		write_padstack_pv(wr, pin);
	}
}

static int write_nets(hyp_wr_t *wr)
{
	htpp_entry_t *e;
	pcb_netmap_t map;

	pcb_netmap_init(&map, wr->pcb);
	for(e = htpp_first(&map.n2o); e != NULL; e = htpp_next(&map.n2o, e)) {
		dyn_obj_t *o;
		pcb_lib_menu_t *net = e->key;
		pcb_fprintf(wr->f, "{NET=%[4],\n", net->Name);
		for(o = e->value; o != NULL; o = o->next) {
			switch(o->obj->type) {
				case PCB_OBJ_LINE: write_line(wr, (pcb_line_t *)o->obj); break;
				case PCB_OBJ_ARC:  write_arc(wr, (pcb_arc_t *)o->obj); break;
				case PCB_OBJ_PIN:
				case PCB_OBJ_VIA:  write_pv(wr, (pcb_pin_t *)o->obj); break;
				case PCB_OBJ_PAD:  write_pad(wr, (pcb_pad_t *)o->obj); break;

				case PCB_OBJ_POLYGON:
				case PCB_OBJ_RAT:
					break; /* not yet done */

				case PCB_OBJ_TEXT:
				case PCB_OBJ_ELEMENT:
				case PCB_OBJ_SUBC:
					break; /* silenlty ignore these */
			}
		}
		fprintf(wr->f, "}\n");
	}

	pcb_netmap_uninit(&map);
}

int io_hyp_write_pcb(pcb_plug_io_t *ctx, FILE *f, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	hyp_wr_t wr;

	memset(&wr, 0, sizeof(wr));
	wr.pcb = PCB;
	wr.f = f;
	wr.fn = new_filename;

	pcb_pshash_init(&wr.psh);

	pcb_printf_slot[4] = "%{{\\}\\()\t\r\n \"}mq";

	if (write_hdr(&wr) != 0)
		goto err;

	if (write_board(&wr) != 0)
		goto err;

	if (write_lstack(&wr) != 0)
		goto err;

	if (write_devices(&wr) != 0)
		goto err;

	if (write_padstack(&wr) != 0)
		goto err;

	if (write_nets(&wr) != 0)
		goto err;

	if (write_foot(&wr) != 0)
		goto err;


	pcb_pshash_uninit(&wr.psh);
	return 0;

	err:;
	pcb_pshash_uninit(&wr.psh);
	return -1;
}

