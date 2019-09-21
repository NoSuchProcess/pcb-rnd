/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
 *
 *  This module, dialogs, was written and is Copyright (C) 2017 by Tibor Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include "board.h"
#include "actions.h"
#include "plugins.h"
#include "misc_util.h"
#include "idpath.h"
#include "search.h"
#include "find.h"

static const char pcb_acts_IsPointOnLine[] = "IsPointOnLine(x, y, r, idpath)";
static const char pcb_acth_IsPointOnLine[] = "Returns 1 if point x;y with radius r is on the line addressed by idpath, 0 else.";
static fgw_error_t pcb_act_IsPointOnLine(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_coord_t x, y, r;
	pcb_idpath_t *idp;
	pcb_any_obj_t *obj;

	PCB_ACT_CONVARG(1, FGW_COORD, IsPointOnLine, x = fgw_coord(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_COORD, IsPointOnLine, y = fgw_coord(&argv[2]));
	PCB_ACT_CONVARG(3, FGW_COORD, IsPointOnLine, r = fgw_coord(&argv[3]));
	PCB_ACT_CONVARG(4, FGW_IDPATH, IsPointOnLine, idp = fgw_idpath(&argv[4]));
	if ((idp == NULL) || !fgw_ptr_in_domain(&pcb_fgw, &argv[4], PCB_PTR_DOMAIN_IDPATH))
		return FGW_ERR_PTR_DOMAIN;
	obj = pcb_idpath2obj(PCB, idp);
	if ((obj == NULL) || (obj->type != PCB_OBJ_LINE))
		return FGW_ERR_ARG_CONV;

	PCB_ACT_IRES(pcb_is_point_on_line(x, y, r, (pcb_line_t *)obj));
	return 0;
}


static const char pcb_acts_IsPointOnArc[] = "IsPointOnArc(x, y, r, idpath)";
static const char pcb_acth_IsPointOnArc[] = "Returns 1 if point x;y with radius r is on the arc addressed by idpath, 0 else.";
static fgw_error_t pcb_act_IsPointOnArc(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_coord_t x, y, r;
	pcb_idpath_t *idp;
	pcb_any_obj_t *obj;

	PCB_ACT_CONVARG(1, FGW_COORD, IsPointOnArc, x = fgw_coord(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_COORD, IsPointOnArc, y = fgw_coord(&argv[2]));
	PCB_ACT_CONVARG(3, FGW_COORD, IsPointOnArc, r = fgw_coord(&argv[3]));
	PCB_ACT_CONVARG(4, FGW_IDPATH, IsPointOnArc, idp = fgw_idpath(&argv[4]));
	if ((idp == NULL) || !fgw_ptr_in_domain(&pcb_fgw, &argv[4], PCB_PTR_DOMAIN_IDPATH))
		return FGW_ERR_PTR_DOMAIN;
	obj = pcb_idpath2obj(PCB, idp);
	if ((obj == NULL) || (obj->type != PCB_OBJ_ARC))
		return FGW_ERR_ARG_CONV;

	PCB_ACT_IRES(pcb_is_point_on_arc(x, y, r, (pcb_arc_t *)obj));
	return 0;
}


static const char pcb_acts_IntersectObjObj[] = "IntersectObjObj(idpath, idpath)";
static const char pcb_acth_IntersectObjObj[] = "Returns 1 if point x;y with radius r is on the arc addressed by idpath, 0 else.";
static fgw_error_t pcb_act_IntersectObjObj(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_idpath_t *idp1, *idp2;
	pcb_any_obj_t *obj1, *obj2;

	PCB_ACT_CONVARG(1, FGW_IDPATH, IntersectObjObj, idp1 = fgw_idpath(&argv[1]));
	PCB_ACT_CONVARG(2, FGW_IDPATH, IntersectObjObj, idp2 = fgw_idpath(&argv[2]));
	if ((idp1 == NULL) || !fgw_ptr_in_domain(&pcb_fgw, &argv[1], PCB_PTR_DOMAIN_IDPATH))
		return FGW_ERR_PTR_DOMAIN;
	if ((idp2 == NULL) || !fgw_ptr_in_domain(&pcb_fgw, &argv[2], PCB_PTR_DOMAIN_IDPATH))
		return FGW_ERR_PTR_DOMAIN;
	obj1 = pcb_idpath2obj(PCB, idp1);
	obj2 = pcb_idpath2obj(PCB, idp2);
	if ((obj1 == NULL) || ((obj1->type & PCB_OBJ_CLASS_REAL) == 0) || (obj2 == NULL) || ((obj2->type & PCB_OBJ_CLASS_REAL) == 0))
		return FGW_ERR_ARG_CONV;

	PCB_ACT_IRES(pcb_intersect_obj_obj(obj1, obj2));
	return 0;
}

