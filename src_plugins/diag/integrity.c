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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include "board.h"
#include "data.h"
#include "error.h"
#include "undo.h"

#define CHK "Broken integrity: "

#define check_type(obj, exp_type) \
	do { \
		pcb_any_obj_t *__obj__ = (pcb_any_obj_t *)(obj); \
		if (__obj__->type != exp_type) \
			pcb_message(PCB_MSG_ERROR, CHK "%s %ld type broken (%d != %d)\n", pcb_obj_type_name(exp_type), __obj__->ID, __obj__->type, exp_type); \
	} while(0)

#define check_parent(name, obj, pt, prnt) \
	do { \
		if (obj->parent_type != pt) \
			pcb_message(PCB_MSG_ERROR, CHK "%s " name " %ld parent type broken (%d != %d)\n", whose, obj->ID, obj->parent_type, pt); \
		else if (obj->parent.any != prnt) \
			pcb_message(PCB_MSG_ERROR, CHK "%s " name " %ld parent type broken (%p != %p)\n", whose, obj->ID, obj->parent.any, prnt); \
	} while(0)

#define check_obj_id(name, data, obj) \
	do { \
		pcb_any_obj_t *__ao__ = htip_get(&data->id2obj, obj->ID); \
		if (__ao__ != (pcb_any_obj_t *)obj) \
			pcb_message(PCB_MSG_ERROR, CHK "%s " name " %ld id hash broken (%p != %p)\n", whose, obj->ID, obj, __ao__); \
		id_chk_cnt++; \
	} while(0)


#define chk_attr(name, obj) \
	do { \
		if (((obj)->Attributes.Number > 0) && ((obj)->Attributes.List == NULL)) \
			pcb_message(PCB_MSG_ERROR, CHK "%s " name " %ld broken empty attribute list\n", whose, (obj)->ID); \
	} while(0)

#define check_field_eq(name, obj, st1, st2, fld, fmt) \
	do { \
		if ((st1)->fld != (st2)->fld) \
			pcb_message(PCB_MSG_ERROR, CHK "%s " name " field ." #fld " value mismatch (" fmt " != " fmt ")\n", whose, obj->ID, (st1)->fld, (st2)->fld); \
	} while(0)

static void chk_layers(const char *whose, pcb_data_t *data, pcb_parenttype_t pt, void *parent, int name_chk);

static void chk_term(const char *whose, pcb_any_obj_t *obj)
{
	const char *aterm = pcb_attribute_get(&obj->Attributes, "term");
	const char *s_intconn = pcb_attribute_get(&obj->Attributes, "intconn");

	if (pcb_obj_id_invalid(aterm))
		pcb_message(PCB_MSG_ERROR, CHK "%s %ld has term attribute '%s' with invalid characters\n", whose, obj->ID, aterm);

	if ((aterm == NULL) && (obj->term == NULL))
		return;
	if (obj->term == NULL) {
		pcb_message(PCB_MSG_ERROR, CHK "%s %ld has term attribute '%s' but no ->term set\n", whose, obj->ID, aterm);
		return;
	}
	if (aterm == NULL) {
		pcb_message(PCB_MSG_ERROR, CHK "%s %ld has ->term '%s' but no attribute term set\n", whose, obj->ID, obj->term);
		return;
	}
	if (aterm != obj->term) {
		pcb_message(PCB_MSG_ERROR, CHK "%s %ld has mismatching pointer of ->term ('%s') and attribute term ('%s')\n", whose, obj->ID, obj->term, aterm);
		return;
	}

	if (s_intconn != NULL) {
		char *end;
		long intconn = strtol(s_intconn, &end, 10);
		if (*end == '\0') {
			if (intconn != obj->intconn) {
				pcb_message(PCB_MSG_ERROR, CHK "%s %ld has mismatching intconn: cached is %d, attribute is '%s'\n", whose, obj->ID, obj->intconn, s_intconn);
				return;
			}
		}
	}
}

static void chk_subc_cache(pcb_subc_t *subc)
{
	const char *arefdes = pcb_attribute_get(&subc->Attributes, "refdes");

	if (pcb_obj_id_invalid(arefdes))
		pcb_message(PCB_MSG_ERROR, CHK "subc %ld has refdes attribute '%s' with invalid characters\n", subc->ID, arefdes);

	if ((subc->BoundingBox.X2 < 0) || (subc->BoundingBox.Y2 < 0))
		pcb_message(PCB_MSG_ERROR, CHK "subc %ld is on negative coordinates; its bottom right corner is %$mm;%$mm\n", subc->ID, subc->BoundingBox.X2, subc->BoundingBox.Y2);

	if ((subc->BoundingBox.X1 > PCB->hidlib.size_x) || (subc->BoundingBox.Y1 > PCB->hidlib.size_y))
		pcb_message(PCB_MSG_ERROR, CHK "subc %ld is olost beyond board extents; its top left corner is %$mm;%$mm\n", subc->ID, subc->BoundingBox.X1, subc->BoundingBox.Y1);

	if ((arefdes == NULL) && (subc->refdes == NULL))
		return;
	if (subc->refdes == NULL) {
		pcb_message(PCB_MSG_ERROR, CHK "subc %ld has refdes attribute '%s' but no ->refdes set\n", subc->ID, arefdes);
		return;
	}
	if (arefdes == NULL) {
		pcb_message(PCB_MSG_ERROR, CHK "subc %ld has ->refdes '%s' but no attribute refdes set\n", subc->ID, subc->refdes);
		return;
	}
	if (arefdes != subc->refdes) {
		pcb_message(PCB_MSG_ERROR, CHK "subc %ld has mismatching pointer of ->refdes ('%s') and attribute refdes ('%s')\n", subc->ID, subc->refdes, arefdes);
		return;
	}
}


static void chk_subc(const char *whose, pcb_subc_t *subc)
{
	int n;
	pcb_pstk_t *ps;
	pcb_coord_t dummy;
	double dummy2;

	chk_layers("subc", subc->data, PCB_PARENT_SUBC, subc, 0);
	chk_subc_cache(subc);

	if (pcb_subc_get_origin(subc, &dummy, &dummy) != 0)
		pcb_message(PCB_MSG_ERROR, CHK "%s %ld: can not determine subc origin\n", whose, subc->ID);
	if (pcb_subc_get_rotation(subc, &dummy2) != 0)
		pcb_message(PCB_MSG_ERROR, CHK "%s %ld: can not determine subc rotation\n", whose, subc->ID);
	if (pcb_subc_get_side(subc, &dummy) != 0)
		pcb_message(PCB_MSG_ERROR, CHK "%s %ld: can not determine subc side\n", whose, subc->ID);

	/* check term chaches */
	for(ps = padstacklist_first(&subc->data->padstack); ps != NULL; ps = padstacklist_next(ps))
		chk_term("padstack", (pcb_any_obj_t *)ps);

	for(n = 0; n < subc->data->LayerN; n++) {
		pcb_layer_t *ly = &subc->data->Layer[n];
		pcb_line_t *lin;
		pcb_arc_t *arc;
		pcb_text_t *txt;
		pcb_poly_t *pol;

		if (!ly->is_bound)
			pcb_message(PCB_MSG_ERROR, CHK "%ld subc layer %ld is not a bound layer\n", subc->ID, n);

		for(lin = linelist_first(&ly->Line); lin != NULL; lin = linelist_next(lin))
			chk_term("line", (pcb_any_obj_t *)lin);

		for(arc = arclist_first(&ly->Arc); arc != NULL; arc = arclist_next(arc))
			chk_term("arc", (pcb_any_obj_t *)arc);

		for(txt = textlist_first(&ly->Text); txt != NULL; txt = textlist_next(txt))
			chk_term("text", (pcb_any_obj_t *)txt);

		for(pol = polylist_first(&ly->Polygon); pol != NULL; pol = polylist_next(pol))
			chk_term("polygon", (pcb_any_obj_t *)pol);
	}
}

/* Check layers and objects: walk the tree top->down and check ->parent
   references from any node */
static void chk_layers(const char *whose, pcb_data_t *data, pcb_parenttype_t pt, void *parent, int name_chk)
{
	pcb_layer_id_t n;
	long id_chk_cnt = 0;
	htip_entry_t *e;

	if (data->parent_type != pt)
		pcb_message(PCB_MSG_ERROR, CHK "%s data: parent type broken (%d != %d)\n", whose, data->parent_type, pt);
	else if (data->parent.any != parent)
		pcb_message(PCB_MSG_ERROR, CHK "%s data: parent broken (%p != %p)\n", whose, data->parent, parent);


	for(n = 0; n < data->LayerN; n++) {
		pcb_line_t *lin;
		pcb_text_t *txt;
		pcb_arc_t *arc;
		pcb_poly_t *poly;
		pcb_layergrp_t *grp;
	
		/* check layers */
		if (data->Layer[n].parent.data != data)
			pcb_message(PCB_MSG_ERROR, CHK "%s layer %ld/%s parent broken (%p != %p)\n", whose, n, data->Layer[n].name, data->Layer[n].parent, data);
		if (name_chk && ((data->Layer[n].name == NULL) || (*data->Layer[n].name == '\0')))
			pcb_message(PCB_MSG_ERROR, CHK "%s layer %ld has invalid name\n", whose, n);
		check_type(&data->Layer[n], PCB_OBJ_LAYER);
		check_parent("layer", (&data->Layer[n]), PCB_PARENT_DATA, data);
		chk_attr("layer", &data->Layer[n]);

		if (!data->Layer[n].is_bound) {
			grp = pcb_get_layergrp(data->parent.board, data->Layer[n].meta.real.grp);
			if (grp != NULL) {
				int i, found = 0;
				for(i = 0; i < grp->len; i++) {
					if (grp->lid[i] == n) {
						found = 1;
						break;
					}
				}
				if (!found)
					pcb_message(PCB_MSG_ERROR, CHK "%s layer %ld is linked to group %ld but the group does not link back to the layer\n", whose, n, data->Layer[n].meta.real.grp);
			}
			else
				pcb_message(PCB_MSG_ERROR, CHK "%s layer %ld is linked to non-existing group %ld\n", whose, n, data->Layer[n].meta.real.grp);
		}

		if (data->Layer[n].is_bound) {
			if ((data->Layer[n].meta.bound.type & PCB_LYT_BOUNDARY) && (data->Layer[n].meta.bound.type & PCB_LYT_ANYWHERE))
				pcb_message(PCB_MSG_ERROR, CHK "%s layer %ld/%s is a non-global boundary (bound layer)\n", whose, n, data->Layer[n].name);
		}

		/* check layer objects */
		for(lin = linelist_first(&data->Layer[n].Line); lin != NULL; lin = linelist_next(lin)) {
			check_parent("line", lin, PCB_PARENT_LAYER, &data->Layer[n]);
			check_obj_id("line", data, lin);
			check_type(lin, PCB_OBJ_LINE);
			chk_attr("line", lin);
		}

		for(txt = textlist_first(&data->Layer[n].Text); txt != NULL; txt = textlist_next(txt)) {
			check_parent("text", txt, PCB_PARENT_LAYER, &data->Layer[n]);
			check_obj_id("text", data, txt);
			check_type(txt, PCB_OBJ_TEXT);
			chk_attr("text", txt);
		}

		for(poly = polylist_first(&data->Layer[n].Polygon); poly != NULL; poly = polylist_next(poly)) {
			check_parent("polygon", poly, PCB_PARENT_LAYER, &data->Layer[n]);
			check_obj_id("polygon", data, poly);
			check_type(poly, PCB_OBJ_POLY);
			chk_attr("polygon", poly);
		}

		for(arc = arclist_first(&data->Layer[n].Arc); arc != NULL; arc = arclist_next(arc)) {
			check_parent("arc", arc, PCB_PARENT_LAYER, &data->Layer[n]);
			check_obj_id("arc", data, arc);
			check_type(arc, PCB_OBJ_ARC);
			chk_attr("arc", arc);
		}
	}

	/* check global objects */
	{
		pcb_subc_t *subc;
		pcb_pstk_t *ps;
		pcb_rat_t *rat;

		for(ps = padstacklist_first(&data->padstack); ps != NULL; ps = padstacklist_next(ps)) {
			check_parent("padstack", ps, PCB_PARENT_DATA, data);
			check_obj_id("padstack", data, ps);
			check_type(ps, PCB_OBJ_PSTK);
			chk_attr("padstack", ps);
			chk_term("padstack", (pcb_any_obj_t *)ps);
		}

		for(subc = pcb_subclist_first(&data->subc); subc != NULL; subc = pcb_subclist_next(subc)) {
			check_parent("subc", subc, PCB_PARENT_DATA, data);
			check_obj_id("subc", data, subc);
			check_type(subc, PCB_OBJ_SUBC);
			chk_subc(whose, subc);
			chk_attr("subc", subc);
		}

		/* check rat line objects */
		for(rat = ratlist_first(&data->Rat); rat != NULL; rat = ratlist_next(rat)) {
			check_parent("rat", rat, PCB_PARENT_DATA, data);
			check_obj_id("rat", data, rat);
			check_type(rat, PCB_OBJ_RAT);
			chk_attr("rat", rat);
		}
	}

	/* Safe check for the other way around: if the hash contains more entries
	   than the objects we checked above, we have some garbage left; the check
	   is safe because it is not needed to dereference the garbage */
	for(e = htip_first(&data->id2obj); e; e = htip_next(&data->id2obj, e)) {
		id_chk_cnt--;
	}
	if (id_chk_cnt != 0)
		pcb_message(PCB_MSG_ERROR, CHK "id hash contains %ld excess IDs in %s\n", id_chk_cnt, whose);
}

static void chk_layergrps(pcb_board_t *pcb)
{
	pcb_layergrp_id_t n;
	const char *whose = "board";

	for(n = 0; n < pcb->LayerGroups.len; n++) {
		pcb_layergrp_t *grp = &pcb->LayerGroups.grp[n];
		int i, i2;

		check_parent("layer_group", grp, PCB_PARENT_BOARD, pcb);
		check_type(grp, PCB_OBJ_LAYERGRP);
		if ((grp->ltype & PCB_LYT_BOUNDARY) && (grp->ltype & PCB_LYT_ANYWHERE))
			pcb_message(PCB_MSG_ERROR, CHK "layer group %ld/%s is a non-global boundary\n", n, grp->name);

		for(i = 0; i < grp->len; i++) {
			pcb_layer_t *ly;

			for(i2 = 0; i2 < i; i2++)
				if (grp->lid[i] == grp->lid[i2])
					pcb_message(PCB_MSG_ERROR, CHK "layer group %ld/%s has duplicate layer entry: %ld\n", n, grp->name, (long)grp->lid[i]);

			ly = pcb_get_layer(pcb->Data, grp->lid[i]);
			if (ly != NULL) {
				if (ly->meta.real.grp != n)
					pcb_message(PCB_MSG_ERROR, CHK "layer group %ld/%s conains layer %ld/%s but it doesn't link back to the group but links to %ld instead \n", n, grp->name, (long)grp->lid[i], ly->name, ly->meta.real.grp);
			}
			else
				pcb_message(PCB_MSG_ERROR, CHK "layer group %ld/%s contains invalid layer entry: %ld\n", n, grp->name, (long)grp->lid[i]);
		}
	}
}


void pcb_check_integrity(pcb_board_t *pcb)
{
	int n;

	chk_layergrps(pcb);
	chk_layers("board", pcb->Data, PCB_PARENT_BOARD, pcb, 1);

	for (n = 0; n < PCB_MAX_BUFFER; n++) {
		char bn[16];
		sprintf(bn, "buffer #%d", n);
		chk_layers(bn, pcb_buffers[n].Data, PCB_PARENT_INVALID, NULL, 0);
	}

	if (undo_check() != 0)
		pcb_message(PCB_MSG_ERROR, CHK "undo\n");
}
