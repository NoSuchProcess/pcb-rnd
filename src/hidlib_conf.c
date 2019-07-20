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

#include "conf.h"
#include "error.h"
#include "color.h"
#include "hidlib.h"
#include "hid.h"

#include "hidlib_conf.h"

#define PCB_MAX_GRID         PCB_MIL_TO_COORD(1000)

pcbhl_conf_t pcbhl_conf;

int pcb_hidlib_conf_init()
{
	int cnt = 0;

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(pcbhl_conf, field,isarray,type_name,cpath,cname,desc,flags);
#include "hidlib_conf_fields.h"

	return cnt;
}

/* sets cursor grid with respect to grid offset values */
void pcb_hidlib_set_grid(pcb_hidlib_t *hidlib, pcb_coord_t Grid, pcb_bool align, pcb_coord_t ox, pcb_coord_t oy)
{
	if (Grid >= 1 && Grid <= PCB_MAX_GRID) {
		if (align) {
			hidlib->grid_ox = ox % Grid;
			hidlib->grid_oy = oy % Grid;
		}
		hidlib->grid = Grid;
		conf_set_design("editor/grid", "%$mS", Grid);
		if (pcbhl_conf.editor.draw_grid)
			pcb_gui->invalidate_all(pcb_gui);
	}
}

void pcb_hidlib_set_unit(pcb_hidlib_t *hidlib, const pcb_unit_t *new_unit)
{
	if (new_unit != NULL && new_unit->allow != PCB_UNIT_NO_PRINT)
		conf_set(CFR_DESIGN, "editor/grid_unit", -1, new_unit->suffix, POL_OVERWRITE);
}
