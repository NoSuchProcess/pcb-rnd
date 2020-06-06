/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
#include "layer.h"
#include <librnd/core/actions.h>
#include "actions_pcb.h"
#include <librnd/core/plugins.h>
#include <librnd/core/misc_util.h>
#include "idpath.h"
#include "search.h"
#include "find.h"

static void append_type_bit(void *ctx, pcb_layer_type_t bit, const char *name, int class, const char *class_name)
{
	gds_t *s = ctx;
	if (s->used > 0)
		gds_append(s, ',');
	gds_append_str(s, name);
}

static const char pcb_acts_ReadGroup[] =
	"ReadGroup(length)\n"
	"ReadGroup(field, group, [init_invis|ltype|ltypestr|ltypehas|name|open|purpose|vis|length])\n"
	"ReadGroup(layerid, group, idx)\n"
	;
static const char pcb_acth_ReadGroup[] = "Length returns the number of groups on the current PCB. Field returns one of the fields of the group named in groupid. Layerid returns the integer layer ID (as interpreted within data) for the idxth layer of the group.";
static fgw_error_t pcb_act_ReadGroup(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	int cmd, fld;
	char *cmds, *flds, *target;
	rnd_layergrp_id_t gid;
	pcb_layergrp_t *grp;
	pcb_layer_type_t lyt;
	long idx;

	RND_ACT_CONVARG(1, FGW_STR, ReadGroup, cmds = argv[1].val.str);
	cmd = act_read_keywords_sphash(cmds);
	switch(cmd) {
		case act_read_keywords_length:
			res->type = FGW_LONG; res->val.nat_long = pcb->LayerGroups.len;
			return 0;
		case act_read_keywords_layerid:
			RND_ACT_CONVARG(2, FGW_LAYERGRPID, ReadGroup, gid = fgw_layergrpid(&argv[2]));
			grp = pcb_get_layergrp(pcb, gid);
			if (grp == NULL)
				return FGW_ERR_ARG_CONV;
			RND_ACT_CONVARG(3, FGW_LONG, ReadGroup, idx = argv[3].val.nat_long);
			if ((idx < 0) || (idx >= grp->len))
				return FGW_ERR_ARG_CONV;
			res->type = FGW_LONG; res->val.nat_long = grp->lid[idx];
			return 0;
		case act_read_keywords_field:
			RND_ACT_CONVARG(2, FGW_LAYERGRPID, ReadGroup, gid = fgw_layergrpid(&argv[2]));
			grp = pcb_get_layergrp(pcb, gid);
			if (grp == NULL)
				return FGW_ERR_ARG_CONV;
			RND_ACT_CONVARG(3, FGW_STR, ReadGroup, flds = argv[3].val.str);
			fld = act_read_keywords_sphash(flds);
			switch(fld) {
				case act_read_keywords_name:
					res->type = FGW_STR; res->val.str = grp->name;
					return 0;
				case act_read_keywords_purpose:
					res->type = FGW_STR; res->val.str = grp->purpose;
					return 0;
				case act_read_keywords_ltype:
					res->type = FGW_LONG; res->val.nat_long = grp->ltype;
					return 0;
				case act_read_keywords_ltype_anywhere:
					res->type = FGW_LONG; res->val.nat_long = grp->ltype & PCB_LYT_ANYWHERE;
					return 0;
				case act_read_keywords_ltype_anything:
					res->type = FGW_LONG; res->val.nat_long = grp->ltype & PCB_LYT_ANYTHING;
					return 0;
				case act_read_keywords_ltypestr:
					lyt = grp->ltype;
					ltypestr:;
					{
						gds_t tmp;
						gds_init(&tmp);
						pcb_layer_type_map(lyt, &tmp, append_type_bit);
						res->type = FGW_STR | FGW_DYN; res->val.str = tmp.array;
					}
					return 0;
				case act_read_keywords_ltypehas:
					RND_ACT_CONVARG(4, FGW_STR, ReadGroup, target = argv[4].val.str);
					lyt = pcb_layer_type_str2bit(target);
					if (lyt == 0) {
						rnd_message(RND_MSG_ERROR, "ReadGroup(): unknown layer type name: '%s'\n", target);
						return FGW_ERR_ARG_CONV;
					}
					RND_ACT_IRES(!!(grp->ltype & lyt));
					return 0;
				case act_read_keywords_ltypestr_anything:
					lyt = grp->ltype & PCB_LYT_ANYTHING;
					goto ltypestr;
				case act_read_keywords_ltypestr_anywhere:
					lyt = grp->ltype & PCB_LYT_ANYWHERE;
					goto ltypestr;
				case act_read_keywords_length:
					res->type = FGW_LONG; res->val.nat_long = grp->len;
					return 0;
				case act_read_keywords_vis:
					res->type = FGW_INT; res->val.nat_int = grp->vis;
					return 0;
				case act_read_keywords_init_invis:
					res->type = FGW_INT; res->val.nat_int = grp->init_invis;
					return 0;
				case act_read_keywords_open:
					res->type = FGW_INT; res->val.nat_int = grp->open;
					return 0;
				default:
					return FGW_ERR_ARG_CONV;
			}
		default:
			return FGW_ERR_ARG_CONV;
	}

	return FGW_ERR_ARG_CONV;
}
