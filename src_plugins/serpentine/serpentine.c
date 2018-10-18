/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Adrian Purser
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include "conf_core.h"

#include <math.h>

#include "crosshair.h"
#include "board.h"
#include "data.h"
#include "data_it.h"
#include "error.h"
#include "search.h"
#include "tool.h"
#include "rats.h"
#include "rtree.h"
#include "flag_str.h"
#include "macro.h"
#include "undo.h"
#include "find.h"
#include "draw.h"
#include "draw_wireframe.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "actions.h"
#include "serpentine_conf.h"
#include "compat_nls.h"
#include "layer.h"
#include "route.h"

/*#include <genregex/regex_sei.h>*/

conf_serpentine_t conf_serpentine;

static pcb_route_t serpentine_route;

static double
point_on_line(	pcb_coord_t X,	pcb_coord_t Y,
								pcb_coord_t X1,	pcb_coord_t Y1,
								pcb_coord_t X2,	pcb_coord_t Y2,
								pcb_coord_t * OutX,
								pcb_coord_t * OutY )
{
	const double abx = (double)X2 - (double)X1;
	const double aby = (double)Y2 - (double)Y1;
	const double apx = (double)X - (double)X1;
	const double apy = (double)Y - (double)Y1;
	const double ab2 = (abx*abx) + (aby*aby);
	const double apab = (apx * abx) + (apy * aby);
	const double t = apab / ab2;

	if(OutX != NULL) *OutX = X1 + (pcb_coord_t)(abx * t);
	if(OutY != NULL) *OutY = Y1 + (pcb_coord_t)(aby * t);

	return t;
}

int 
serpentine_calculate_route(	pcb_route_t * route,
														const pcb_line_t * line,
														const pcb_point_t * point1,
														const pcb_point_t * point2,
														const pcb_coord_t pitch )
{
	pcb_coord_t sx,sy,ex,ey;
	double angle_n,angle_l;
	pcb_coord_t amplitude = 15.0 * line->Thickness;
	const pcb_coord_t radius = pitch/2;
	int count = 0;
	double sind,cosd,sinn,cosn;

	pcb_point_t start,end;

	/* Project the start and end points onto the line */
	double t0 =	point_on_line(point1->X,point1->Y,line->Point1.X,line->Point1.Y,line->Point2.X,line->Point2.Y,&sx,&sy);
	double t1 =	point_on_line(point2->X,point2->Y,line->Point1.X,line->Point1.Y,line->Point2.X,line->Point2.Y,&ex,&ey);
	double nx = point2->X - ex;
	double ny = point2->Y - ey;
	double lx = ex-sx;
	double ly = ey-sy;
	double ld,lnx,lny;
	double nd,nnx,nny;
	double side;

	/* If the start or end point is outside of the line then abort the route creation */
	if(	(t0 == t1) || (t0 < 0.0) || (t0 > 1.0) ||	(t1 < 0.0) || (t1 > 1.0) )
		return -1;

	ld		= sqrt(lx*lx + ly*ly);
	lnx		= lx/ld;
	lny		= ly/ld;
	nd		= sqrt(nx*nx + ny*ny);
	count = ld/(pitch*2);
	
	if((count == 0) || (nd < 0.1))
		return -1;
		
	nnx = (double)nx/nd;
	nny = (double)ny/nd;
	
	/* If the end point is closest to the start of the line then swap the start
	 * and end points so that we are always going in the correct direction
	 */
	if(t1 < t0) {
		pcb_coord_t t;
		ex = sx;
		ey = sy;
		sx += count * pitch * 2 * lnx;
		sy += count * pitch * 2 * lny;
		lnx = -lnx;
		lny = -lny;
	}

	side = 0 - ((lnx * nny) - (lny * nnx));
	start.X = sx;
	start.Y = sy;
	end.X = ex;
	end.Y = ey;

	angle_n = (0.0-(atan2(ny, nx) / PCB_M180));
	angle_l = (0.0-(atan2(ly, lx) / PCB_M180));

	/* Calculate Amplitude */
	amplitude = nd;
	if(amplitude < (2.0*radius))
		amplitude = radius * 2;

	/* Create the route */
	if(count > 0) {
		int i;
		const double lpx = pitch * lnx;
		const double lpy = pitch * lny;
		const double nax = (amplitude - (radius * 2.0)) * nnx;
		const double nay = (amplitude - (radius * 2.0)) * nny;
		const pcb_coord_t slx = start.X + (radius * lnx) + (radius * nnx);
		const pcb_coord_t sly = start.Y + (radius * lny) + (radius * nny);

		pcb_point_t startpos;
		pcb_point_t endpos;

		route->start_point	= start;
		route->thickness		= line->Thickness;
		route->clearance		= line->Clearance;
		route->start_layer	= 
		route->end_layer		= pcb_layer_id(PCB->Data,line->parent.layer);
		route->PCB					= PCB;
		route->flags				= line->Flags;

		/* Add a line from the start of the original line to the start of the serpentine. */ 
		pcb_route_add_line(route,&line->Point1,&start,route->start_layer);
		
		/* Add a quarter arc to lead into the serpentine. */
		startpos.X = start.X + (nnx * radius);
		startpos.Y = start.Y + (nny * radius);
		pcb_route_add_arc(route,&startpos,angle_n/*+180*/,90*side,radius,route->start_layer);

		/* Each cycle of the loop will add one phase of the serpentine.  The last 
		 * cycle will finish with a quarter arc to lead back into the line.
		 */
		for(i=0;i<count;++i) {
			const pcb_coord_t of = (pitch * 2 * i)+pitch;
			const pcb_coord_t px = start.X + (lnx * of);
			const pcb_coord_t py = start.Y + (lny * of);

			/* First Line */
			startpos.X = slx + (lpx * i * 2);
			startpos.Y = sly + (lpy * i * 2);
			endpos.X = startpos.X + nax;
			endpos.Y = startpos.Y + nay;
			pcb_route_add_line(route,&startpos,&endpos,route->start_layer);

			/* First Arc */
			startpos.X = px + (nnx * (amplitude-radius));
			startpos.Y = py + (nny * (amplitude-radius));
			pcb_route_add_arc(route,&startpos,angle_n + (side*270),(-180)*side,radius,route->start_layer);

			/* Second Line */
			startpos.X = slx + nax + (lpx * ((i * 2)+1));
			startpos.Y = sly + nay + (lpy * ((i * 2)+1));
			endpos.X = startpos.X - nax;
			endpos.Y = startpos.Y - nay;
			pcb_route_add_line(route,&endpos,&startpos,route->start_layer);
			
			/* Second Arc */
			startpos.X = px + (nnx * radius) + (lnx * pitch);
			startpos.Y = py + (nny * radius) + (lny * pitch);
			pcb_route_add_arc(route,&startpos,angle_n + (side*270),(i+1 == count ? 90.0 : 180.0)*side,radius,route->start_layer);
		}

		/* Add a line from the end of the serpentine to the end of the original line. */ 
		pcb_route_add_line(route,&route->end_point,&line->Point2,route->start_layer);
	}
	
	return 0;
}


/*** Serpentine Tool ***/

static void tool_serpentine_init(void)
{
	pcb_route_init(&serpentine_route);


}

static void tool_serpentine_uninit(void)
{
  pcb_notify_crosshair_change(pcb_false);
  pcb_crosshair.AttachedObject.Type = PCB_OBJ_VOID;
  pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
  pcb_notify_crosshair_change(pcb_true);
  pcb_route_destroy(&serpentine_route);
}


static void tool_serpentine_notify_mode(void)
{
	int type;
	pcb_any_obj_t *term_obj;
	
	switch (pcb_crosshair.AttachedObject.State) {
	case PCB_CH_STATE_FIRST:
		pcb_crosshair.AttachedObject.Type = pcb_search_screen(pcb_tool_note.X, pcb_tool_note.Y, PCB_OBJ_LINE, 
						&pcb_crosshair.AttachedObject.Ptr1, &pcb_crosshair.AttachedObject.Ptr2, &pcb_crosshair.AttachedObject.Ptr3);
		/* TODO: check if an object is on the current layer */
		if (pcb_crosshair.AttachedObject.Type == PCB_OBJ_LINE) {
			pcb_line_t * p_line = (pcb_line_t *)pcb_crosshair.AttachedObject.Ptr2;
			
			double t = point_on_line(	pcb_tool_note.X,pcb_tool_note.Y,
																p_line->Point1.X,p_line->Point1.Y,
																p_line->Point2.X,p_line->Point2.Y,
																&pcb_crosshair.AttachedObject.X,
																&pcb_crosshair.AttachedObject.Y );

			if((t >= 0.0) && (t <= 1.0)) {
				pcb_route_reset(&serpentine_route);
				pcb_crosshair.AttachedObject.State = PCB_CH_STATE_SECOND;
			}
		}
		else
			pcb_message(PCB_MSG_WARNING, _("Serpentines can be only drawn onto a line\n"));
		break;

	case PCB_CH_STATE_SECOND:
		pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
		break;

	case PCB_CH_STATE_THIRD:
		break;

	default:		/* all following points */
		break;
	}
}

static void tool_serpentine_adjust_attached_objects(void)
{
		pcb_crosshair.AttachedObject.tx = pcb_crosshair.X;
		pcb_crosshair.AttachedObject.ty = pcb_crosshair.Y;
}

static void tool_serpentine_draw_attached(void)
{
	switch (pcb_crosshair.AttachedObject.State) {
		case PCB_CH_STATE_SECOND:
			{
				pcb_line_t * p_line = (pcb_line_t *)pcb_crosshair.AttachedObject.Ptr2;
				pcb_point_t point1;
				pcb_point_t point2;
				point1.X = pcb_crosshair.AttachedObject.X;
				point1.Y = pcb_crosshair.AttachedObject.Y;
				point2.X = pcb_crosshair.AttachedObject.tx; 
				point2.Y = pcb_crosshair.AttachedObject.ty;

				pcb_route_reset(&serpentine_route);
				if(serpentine_calculate_route(&serpentine_route,p_line, &point1, &point2, 5 * p_line->Thickness ) == 0) {
					pcb_route_draw(&serpentine_route, pcb_crosshair.GC);
					pcb_route_reset(&serpentine_route);
				}
			}
			break;

		default : 
			break;
	}
}

pcb_bool tool_serpentine_undo_act()
{
	/* don't allow undo in the middle of an operation */
	if (pcb_crosshair.AttachedObject.State != PCB_CH_STATE_FIRST)
		return pcb_false;
	return pcb_true;
}

static pcb_tool_t tool_serpentine = {
	"serpentine", NULL, 100,
	tool_serpentine_init,
	tool_serpentine_uninit,
	tool_serpentine_notify_mode,
	NULL,
	tool_serpentine_adjust_attached_objects,
	tool_serpentine_draw_attached,
	tool_serpentine_undo_act,
	NULL,
	
	pcb_false
};



/*** Actions ***/

static const char pcb_acts_serpentine[] = "serpentine()";
static const char pcb_acth_serpentine[] = "Tool for drawing serpentines";
fgw_error_t pcb_act_serpentine(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_tool_select_by_name("serpentine");
	PCB_ACT_IRES(0);
	return 0;
}


pcb_action_t serpentine_action_list[] = {
  {"Serpentine", pcb_act_serpentine, pcb_acth_serpentine, pcb_acts_serpentine}
};

static const char *serpentine_cookie = "serpentine plugin";

PCB_REGISTER_ACTIONS(serpentine_action_list, serpentine_cookie)

int pplg_check_ver_serpentine(int ver_needed) { return 0; }

void pplg_uninit_serpentine(void)
{
	pcb_remove_actions_by_cookie(serpentine_cookie);
	pcb_tool_unreg_by_cookie(serpentine_cookie);
	conf_unreg_fields("plugins/serpentine/");
}

#include "dolists.h"
int pplg_init_serpentine(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(serpentine_action_list, serpentine_cookie)
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_serpentine, field,isarray,type_name,cpath,cname,desc,flags);
#include "serpentine_conf_fields.h"

	pcb_tool_reg(&tool_serpentine, serpentine_cookie);

	return 0;
}

